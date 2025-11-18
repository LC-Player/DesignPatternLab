#include "Workspace.h"

#include <filesystem>
#include <fstream>

#include "Outputer.h"
#include "TreeDrawer.h"

#include "nlohmann/json.hpp"

Workspace::Workspace(const std::string& workspaceData)
    : CommandExecutor()
{
    m_CurrentEditor = -1;
    m_LogMode = LogMode::NoLog;
    m_Running = true;
    if (workspaceData.find_first_not_of("\n\t ") != std::string::npos) {
        try {
            nlohmann::json inData = nlohmann::json::parse(workspaceData);
            DeserializeJson(inData);
        } catch (const std::exception&) {}
    }
    m_Logger = CreateScope<Logger>("data/.workspace.log");
    RegisterCommandHandlingStrategies();
}

void Workspace::Handle(const Command& command)
{
    bool success = false;
    if (m_Dispatcher.Dispatch(this, command)) {
        success = true;
    } else {
        Outputer::ErrorLn(command) << "Command not handled in workspace.";
    }
    if (success && m_LogMode == LogMode::WithLog) {
        m_Logger->Log(command);
    }
}

void Workspace::CreateEditorByFilePath(const std::string& fp) {
    const Ref<Editor> editor = CreateRef<Editor>(fp);
    m_Editors.push_back(editor);
}

void Workspace::RegisterCommandHandlingStrategies() {
    m_Dispatcher.Register(Command::Type::Load, &Workspace::HandleLoad);
    m_Dispatcher.Register(Command::Type::Save, &Workspace::HandleSave);
    m_Dispatcher.Register(Command::Type::Init, &Workspace::HandleInit);
    m_Dispatcher.Register(Command::Type::Close, &Workspace::HandleClose);
    m_Dispatcher.Register(Command::Type::Edit, &Workspace::HandleEdit);
    m_Dispatcher.Register(Command::Type::EditorList, &Workspace::HandleEditorList);
    m_Dispatcher.Register(Command::Type::DirTree, &Workspace::HandleDirTree);
    m_Dispatcher.Register(Command::Type::LogOn, &Workspace::HandleLogOn);
    m_Dispatcher.Register(Command::Type::LogOff, &Workspace::HandleLogOff);
    m_Dispatcher.Register(Command::Type::LogShow, &Workspace::HandleLogShow);
    m_Dispatcher.Register(Command::Type::Exit, &Workspace::HandleExit);
}

bool Workspace::HandleLoad(const Command& command)
{
    auto fp = command.GetArgs()[0];
    if (auto existing = GetEditorIndexByPath(fp); existing >= 0){
        m_CurrentEditor = existing;
        return false;
    }
    try {
        CreateEditorByFilePath(fp);
    } catch (const std::exception& e) {
        Outputer::ErrorLn(command) << "Failed to create editor: " << e.what();
        return false;
    }

    m_CurrentEditor = static_cast<int>(m_Editors.size()) - 1;
    UpdateLogMode(GetCurrentEditor()->GetLogMode());

    return true;
}

bool Workspace::HandleSave(const Command& command){
    auto& args = command.GetArgs();
    Ref<Editor> target;
    if (args.empty()){ // save current editor
        target = GetCurrentEditor();
        if (target == nullptr){
            Outputer::ErrorLn(command) << "No current editor";
            return false;
        }
        target->Save();
        return true;
    }
    const auto fp = command.GetArgs()[0];
    if (fp == "all"){
        for (const auto& editor : m_Editors){
            editor->Save();
        }
    }
    target = GetEditorByPath(fp);
    if (target == nullptr){
        Outputer::ErrorLn(command) << "File `" << fp << "` not found in workspace";
        return false;
    }
    target->Save();
    GetCurrentEditor()->UpdateTime();

    return true;
}

bool Workspace::HandleInit(const Command& command){
    auto& args = command.GetArgs();
    const auto& fp = args[0];
    if (GetEditorIndexByPath(fp) >= 0){
        Outputer::ErrorLn(command) << "File `" << args[0] << "` found in workspace";
        return false;
    }
    LogMode logMode = LogMode::None;
    if (args.size() >= 2 && args[1] == "with-log") {
        logMode = LogMode::WithLog;
    }
    Ref<Editor> editor;
    try {
        editor = CreateRef<Editor>(fp, logMode);
    } catch (const std::exception& e) {
        Outputer::ErrorLn(command) << "Failed to create editor: " << e.what();
        return false;
    }
    m_CurrentEditor = static_cast<int>(m_Editors.size());
    m_Editors.push_back(editor);
    GetCurrentEditor()->UpdateTime();
    UpdateLogMode(GetCurrentEditor()->GetLogMode());

    return true;
}
bool Workspace::HandleClose(const Command& command){
    if (!command.GetArgs().empty()){
        // user specified file
        auto fp = command.GetArgs()[0];
        int editorIndex = GetEditorIndexByPath(fp);
        if (editorIndex >= 0){
            m_Editors[editorIndex]->AskSaving();
            m_Editors.erase(m_Editors.begin() + editorIndex);
            m_CurrentEditor = GetLastEditorIndex();
        } else{
            Outputer::ErrorLn(command) << "File `" << fp << "` not found in workspace";
            return false;
        }
    }

    if (m_CurrentEditor < 0 || m_CurrentEditor >= m_Editors.size()){
        Outputer::ErrorLn(command) << "No current editor";
        return false;
    }

    m_Editors[m_CurrentEditor]->AskSaving();
    Outputer::InfoLn() << "File closed: " << m_Editors[m_CurrentEditor]->GetFilePath();
    m_Editors.erase(m_Editors.begin() + m_CurrentEditor);
    m_CurrentEditor = GetLastEditorIndex();
    if (GetCurrentEditor()) {
        UpdateLogMode(GetCurrentEditor()->GetLogMode());
    }

    return true;

}

bool Workspace::HandleEdit(const Command& command){
    auto to = GetEditorIndexByPath(command.GetArgs()[0]);
    if (to < 0){
        Outputer::ErrorLn(command) << "No such editor for file - " << command.GetArgs()[0];
        return false;
    }
    m_CurrentEditor = to;
    GetCurrentEditor()->UpdateTime();
    UpdateLogMode(GetCurrentEditor()->GetLogMode());

    return true;
}
bool Workspace::HandleEditorList(const Command& command) {
    for (int i = 0; i < m_Editors.size(); i++){
        if (i == m_CurrentEditor){
            Outputer::Out() << ">";
        } else {
            Outputer::Out() << " ";
        }
        Outputer::Out() << ' ' << m_Editors[i]->GetFilePath();
        if (m_Editors[i]->IsModified()){
            Outputer::Out() << '*';
        }
        Outputer::Out() << '\n';
    }

    return true;
}
bool Workspace::HandleDirTree(const Command& command) {
    std::string fp;
    if (!command.GetArgs().empty()) {
        fp = command.GetArgs()[0];
    } else {
        fp = std::filesystem::current_path().string();
    }
    Outputer::Out() << fp << std::endl;
    DrawDirTree(fp, "");

    return true;
}
bool Workspace::HandleLogOn(const Command& command) {
    Ref<Editor> targetEditor;
    LogMode logMode = LogMode::WithLog;
    if (command.GetArgs().empty()) {
        targetEditor = GetCurrentEditor();
    } else {
        targetEditor = GetEditorByPath(command.GetArgs()[0]);
    }
    if (!targetEditor) {
        Outputer::ErrorLn(command) << "No such editor";
        return false;
    }
    targetEditor->SetLogMode(logMode);
    m_LogMode = m_CurrentEditor < 0 ? logMode : GetCurrentEditor()->GetLogMode();

    return true;
}

bool Workspace::HandleLogOff(const Command& command) {
    Ref<Editor> targetEditor;
    LogMode logMode = LogMode::NoLog;
    if (command.GetArgs().empty()) {
        targetEditor = GetCurrentEditor();
    } else {
        targetEditor = GetEditorByPath(command.GetArgs()[0]);
    }
    if (!targetEditor) {
        Outputer::ErrorLn(command) << "No such editor";
        return false;
    }
    targetEditor->SetLogMode(logMode);
    m_LogMode = m_CurrentEditor < 0 ? logMode : GetCurrentEditor()->GetLogMode();

    return true;
}

bool Workspace::HandleLogShow(const Command& command) {
    Ref<Editor> targetEditor;

    if (command.GetArgs().empty()) {
        targetEditor = GetCurrentEditor();
    } else {
        targetEditor = GetEditorByPath(command.GetArgs()[0]);
    }
    if (!targetEditor) {
        Outputer::ErrorLn(command) << "No such editor";
        return false;
    }
    this->m_Logger->Show();
    targetEditor->GetLogger()->Show();

    return true;
}
bool Workspace::HandleExit(const Command& command) {
    for (const auto& editor : m_Editors) {
        if (editor->IsModified()) {
            editor->AskSaving();
        }
    }
    ExportState();
    m_Running = false;

    return true;
}

/**
 * @return -1 when no editors
 */
int Workspace::GetLastEditorIndex() const{
    long long tp = 0;
    int index = -1;
    for (int i = 0; i < m_Editors.size(); i++){
        const auto current = m_Editors[i]->GetLastTime().time_since_epoch().count();
        if (current > tp){
            tp = current;
            index = i;
        }
    }
    return index;
}

void Workspace::ExportState() const {
    nlohmann::json statusJson = SerializeJson();
    const auto jsonString = statusJson.dump(4);
    if (!std::filesystem::exists("data")) {
        std::filesystem::create_directory("data");
    }
    std::ofstream outFile("data/.editor_workspace");
    if (!outFile.is_open()) {
        Outputer::InfoLn() << "Failed to update workspace file";
        return;
    }
    outFile << jsonString;
    outFile.close();
}

nlohmann::json Workspace::SerializeJson() const {
    nlohmann::json j;
    j["log_mode"] = m_LogMode;
    j["current_editor"] = m_CurrentEditor;
    for (const auto& editor : m_Editors) {
        nlohmann::json editorJson;
        editorJson["path"] = editor->GetFilePath();
        editorJson["modified"] = editor->IsModified();
        editorJson["log_mode"] = editor->GetLogMode();
        j["editors"].push_back(editorJson);
    }
    return j;
}

void Workspace::DeserializeJson(const nlohmann::json& j) {
    m_Editors.clear();
    m_CurrentEditor = -1;
    if (j.contains("log_mode")) {
        m_LogMode = j["log_mode"].get<LogMode>();
    } else {
        m_LogMode = LogMode::None;
    }
    if (j.contains("current_editor")) {
        m_CurrentEditor = j["current_editor"].get<int>();
    }
    if (!j.contains("editors") || j["editors"].empty() || !j["editors"].is_array()) {
        return;
    }
    for (const auto& editorJson : j["editors"]) {
        if (!editorJson.contains("path") || !editorJson["path"].is_string()) {
            Outputer::InfoLn() << "Invalid editor configure: missing token `path`";
            continue;
        }
        if (!editorJson.contains("log_mode") || !editorJson["log_mode"].is_number()) {
            Outputer::InfoLn() << "Invalid editor configure: missing token `log_mode`";
            continue;
        }
        if (!editorJson.contains("modified") || !editorJson["modified"].is_boolean()) {
            Outputer::InfoLn() << "Invalid editor configure: missing token `modified`";
            continue;
        }
        CreateEditorByFilePath(editorJson["path"].get<std::string>());
        auto current = m_Editors.back();
        current->SetLogMode(editorJson["log_mode"].get<LogMode>());
        current->SetModified(editorJson["modified"].get<bool>());
        Outputer::InfoLn() << "Loaded: " << current->GetFilePath();
    }
}

Ref<Editor> Workspace::GetEditorByPath(const std::string& path) const
{
    for (const auto& editor : m_Editors) {
        if (editor->GetFilePath() == path) { return editor; }
    }
    return nullptr;
}

int Workspace::GetEditorIndexByPath(const std::string& path) const {
    for (int i = 0; i < m_Editors.size(); i++){
        if (m_Editors[i]->GetFilePath() == path) { return i; }
    }
    return -1;
}

void Workspace::UpdateLogMode(const LogMode logMode) {
    switch (logMode) {
    case LogMode::None:
        return;
    default:
        this->m_LogMode = logMode;
    }
}
//
// const std::unordered_map<Command::Type, Workspace::CommandStrategy> Workspace::s_HandlerMethods = {
//     {Command::Type::Load,        &Workspace::HandleLoad},
//     {Command::Type::Save,        &Workspace::HandleSave},
//     {Command::Type::Init,        &Workspace::HandleInit},
//     {Command::Type::Close,       &Workspace::HandleClose},
//     {Command::Type::Edit,        &Workspace::HandleEdit},
//     {Command::Type::EditorList,  &Workspace::HandleEditorList},
//     {Command::Type::DirTree,     &Workspace::HandleDirTree},
//     {Command::Type::LogOn,       &Workspace::HandleLogOn},
//     {Command::Type::LogOff,      &Workspace::HandleLogOff},
//     {Command::Type::LogShow,     &Workspace::HandleLogShow},
//     {Command::Type::Exit,        &Workspace::HandleExit},
// };