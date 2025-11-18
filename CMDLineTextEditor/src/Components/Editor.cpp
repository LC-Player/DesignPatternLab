#include "Editor.h"

#include <cassert>
#include <filesystem>
#include <fstream>

#include "Outputer.h"

std::pair<int, int> ParseRange(const std::string& range) {
    const auto seperator = range.find(':');
    if (seperator == std::string::npos) {
        throw std::invalid_argument("Invalid range");
    }
    return {std::stoi(range.substr(0, seperator)), std::stoi(range.substr(seperator+1))};
}

std::vector<std::string> ParseLineBreaks(const std::string& line, const std::string& seperator) {
    size_t start = 0, end = 0;
    std::vector<std::string> result;
    while (true) {
        end = line.find(seperator, start);
        if (end == std::string::npos) {
            result.emplace_back(line.substr(start));
            return result;
        }
        result.emplace_back(line.substr(start, end - start));
        start = end + seperator.length();
    }
}

// const std::unordered_map<Command::Type, Editor::CommandStrategy> Editor::s_HandlerMethods = {
//     {Command::Type::Append,  &Editor::HandleAppend},
//     {Command::Type::Insert,  &Editor::HandleInsert},
//     {Command::Type::Delete,  &Editor::HandleDelete},
//     {Command::Type::Replace, &Editor::HandleReplace},
//     {Command::Type::Show,    &Editor::HandleShow},
//     {Command::Type::Undo,    &Editor::HandleUndo},
//     {Command::Type::Redo,    &Editor::HandleRedo},
// };

Editor::Editor() = default;

Editor::Editor(const std::string& filePathText, LogMode logMode)
    : m_FilePath(filePathText), m_LastTime(std::chrono::system_clock::now())
{
    std::filesystem::path fp(m_FilePath);
    if (!std::filesystem::exists(fp.parent_path())) {
        std::filesystem::create_directories(fp.parent_path());
    }
    if (!std::filesystem::is_directory(fp.parent_path())) {
        throw std::runtime_error("The parent directory of `" + filePathText + "` is an existing file");
    }
    m_Data.modified = false;
    std::ifstream in(filePathText);
    if (!in.is_open()) {
        std::ofstream out(filePathText);
        if (!out.is_open())
            throw std::runtime_error("Could not open file: " + filePathText);
        out.close();

        in.open(filePathText);;
        if (!in.is_open())
            throw std::runtime_error("Could not open newly created file: " + filePathText);
        m_Data.modified = true;
    }
    for (std::string line; std::getline(in, line); ) {
        m_Data.lines.push_back(line);
    }
    if (!m_Data.lines.empty()) {
        if (m_Data.lines[0] == "# log") {
            m_Data.logMode = LogMode::WithLog;
        }
    }
    if (logMode != LogMode::None) {
        m_Data.logMode = logMode;
    }
    auto loggingPath = fp.parent_path().string() + "/." + fp.filename().string() + ".log";
    m_Logger = CreateRef<Logger>(loggingPath);
    RegisterCommandHandlingStrategies();
}

void Editor::Handle(const Command& command)
{
    bool success = false;
    if (m_Dispatcher.Dispatch(this, command)) {
        success = true;
    } else {
        Outputer::ErrorLn(command) << "Command not handled in workspace.";
    }
    if (success && m_Data.logMode == LogMode::WithLog) {
        m_Logger->Log(command);
    }
}

void Editor::RegisterCommandHandlingStrategies() {
    m_Dispatcher.Register(Command::Type::Append, &Editor::HandleAppend);
    m_Dispatcher.Register(Command::Type::Insert, &Editor::HandleInsert);
    m_Dispatcher.Register(Command::Type::Delete, &Editor::HandleDelete);
    m_Dispatcher.Register(Command::Type::Replace, &Editor::HandleReplace);
    m_Dispatcher.Register(Command::Type::Show, &Editor::HandleShow);
    m_Dispatcher.Register(Command::Type::Undo, &Editor::HandleUndo);
    m_Dispatcher.Register(Command::Type::Redo, &Editor::HandleRedo);
}

bool Editor::HandleShow(const Command& command) {
    int from = 0, to = static_cast<int>(m_Data.lines.size()) - 1;
    if (!command.GetArgs().empty()) {
        try {
            auto rangeStr = command.GetArgs()[0];
            auto r = ParseRange(rangeStr);
            from = r.first - 1;
            to = r.second - 1;
        } catch (const std::exception&) {
            Outputer::ErrorLn(command) << "Invalid range format";
            return false;
        }

        if (from < 0 || to < 0 || from >= m_Data.lines.size() || to >= m_Data.lines.size()) {
            Outputer::ErrorLn(command) << "Range out of bounds";
            return false;
        }
        if (from > to) {
            std::swap(from, to);
        }
    }

    for (int i = std::max(0, from); i < m_Data.lines.size() && i <= to; i++) {
        Outputer::Out() << m_Data.lines[i] << '\n';
    }
    return true;
}

bool Editor::HandleAppend(const Command& command) {
    MODIFICATION_SCOPE;
    m_Data.lines.emplace_back(command.GetArgs()[0]);
    m_Data.modified = true;
    return true;
}

bool Editor::HandleInsert(const Command& command) {
    int lineIndex;
    int col;
    if (!GetAndValidateLineColRange(command, lineIndex, col)) return false;
    const auto& text = command.GetArgs()[1];
    MODIFICATION_SCOPE;
    Insert(lineIndex, col, text);
    return true;
}

bool Editor::HandleDelete(const Command& command) {
    int lineIndex;
    int col;
    if (!GetAndValidateLineColRange(command, lineIndex, col)) return false;
    auto& lineText = m_Data.lines[lineIndex];
    int len = 0;
    try {
        len = std::stoi(command.GetArgs()[1]);
    } catch (const std::exception&) {
        Outputer::ErrorLn(command) << "Invalid length";
        return false;
    }
    if (static_cast<int>(lineText.size()) - col - len < 0) {
        Outputer::ErrorLn(command) << "Deleted part beyond the end of the line";
        return false;
    }
    MODIFICATION_SCOPE;
    lineText.erase(lineText.begin() + col, lineText.begin() + col + len);
    return true;
}

bool Editor::HandleReplace(const Command& command) {
    int lineIndex;
    int col;
    if (!GetAndValidateLineColRange(command, lineIndex, col)) return false;
    auto& lineText = m_Data.lines[lineIndex];
    int len = 0;
    try {
        len = std::stoi(command.GetArgs()[1]);
    } catch (const std::exception&) {
        Outputer::ErrorLn(command) << "Invalid length";
        return false;
    }
    if (static_cast<int>(lineText.size()) - col - len < 0) {
        Outputer::ErrorLn(command) << "Replaced part beyond the end of the line";
        return false;
    }
    MODIFICATION_SCOPE;
    lineText.erase(lineText.begin() + col, lineText.begin() + col + len);
    Insert(lineIndex, col, command.GetArgs()[2]);
    return true;
}

bool Editor::HandleUndo(const Command& command) {
    if (m_UndoStack.empty()) {
        Outputer::ErrorLn(command) << "Nothing to undo";
        return false;
    }
    auto snapshot = CreateDataSnapshot();
    m_RedoStack.emplace(std::move(snapshot));
    this->m_Data = *m_UndoStack.top();
    m_UndoStack.pop();
    UpdateTime();
    return true;
}
bool Editor::HandleRedo(const Command& command) {
    if (m_RedoStack.empty()) {
        Outputer::ErrorLn(command) << "Nothing to redo";
        return false;
    }
    auto snapshot = CreateDataSnapshot();
    m_UndoStack.emplace(std::move(snapshot));
    this->m_Data = *m_RedoStack.top();
    m_RedoStack.pop();
    UpdateTime();
    return true;
}


bool Editor::GetAndValidateLineColRange(const Command& command, int& lineIndex, int& col) const {
    std::pair<int, int> range;
    try {
        range = ParseRange(command.GetArgs()[0]);
    } catch (const std::exception&) {
        Outputer::ErrorLn(command) << "Invalid range format";
        return false;
    }
    lineIndex = range.first - 1;
    col = range.second - 1;
    if (lineIndex < 0 || lineIndex >= m_Data.lines.size()) {
        Outputer::ErrorLn(command) << "Line out of bounds";
        return false;
    }
    auto& lineText = m_Data.lines[lineIndex];
    if (col < 0 || col > lineText.size()) {
        Outputer::ErrorLn(command) << "Column out of bounds";
        return false;
    }
    return true;
}

void Editor::Insert(int lineIndex, int col, const std::string& raw) {
    auto inserted = ParseLineBreaks(raw);
    const auto lineText = m_Data.lines[lineIndex];
    const auto prefix = lineText.substr(0, col), suffix = lineText.substr(col);
    assert(!inserted.empty());
    if (inserted.size() == 1) {
        m_Data.lines[lineIndex] = prefix + inserted[0] + suffix;
    }
    else {
        std::vector<std::string> lineReplacement;
        lineReplacement.reserve(inserted.size());
        for (int i = 0; i < inserted.size(); i++) {
            if (i == 0) {
                lineReplacement.emplace_back(prefix + inserted[i]);
            } else if (i == inserted.size() - 1) {
                lineReplacement.emplace_back(inserted[i] + suffix);
            } else {
                lineReplacement.emplace_back(inserted[i]);
            }
        }
        m_Data.lines.erase(m_Data.lines.begin() + lineIndex);
        m_Data.lines.insert(m_Data.lines.begin() + lineIndex, lineReplacement.begin(), lineReplacement.end());
    }
}

Scope<EditorData> Editor::CreateDataSnapshot() {
    return CreateScope<EditorData>(m_Data);
}

void Editor::Save() {
    std::ofstream out(m_FilePath);
    if (!out.is_open()) {
        throw std::runtime_error("Could not open file: " + m_FilePath);
    }
    for (const auto& line : m_Data.lines) {
        out << line << std::endl;
    }
    out.close();
    m_Data.modified = false;
    Outputer::InfoLn() << "File saved: " << m_FilePath;
}
void Editor::AskSaving(){
    if (m_Data.modified) {
        Outputer::InfoLn() << "File `" << m_FilePath << "` has been modified. Save before Closing?(y/n)";
        std::string input;
        std::getline(std::cin, input);
        if (input.at(0) != 'n' && input.at(0) != 'N') {
            Save();
        }
    }
}

void Editor::UpdateTime() {
    m_LastTime = std::chrono::system_clock::now();
}
