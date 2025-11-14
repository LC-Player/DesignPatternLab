#include "Workspace.h"

#include <filesystem>
#include <fstream>

#include "Outputer.h"
#include "TreeDrawer.h"

#include "nlohmann/json.hpp"

Workspace::Workspace(const std::string& workspaceData)
    : CommandHandler()
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
}

void Workspace::Handle(const Command& command)
{
    m_CurrentCommand = command;
    m_Success = false;
    switch (command.GetType())
    {
    case Command::Type::Load:
        HandleLoad();
        break;
    case Command::Type::Save:
        HandleSave();
        break;
    case Command::Type::Init:
        HandleInit();
        break;
    case Command::Type::Close:
        HandleClose();
        break;
    case Command::Type::Edit:
        HandleEdit();
        break;
    case Command::Type::EditorList:
        HandleEditorList();
        break;
    case Command::Type::DirTree:
        HandleDirTree();
        break;
    case Command::Type::Exit:
        HandleExit();
        break;
    case Command::Type::LogOn:
        HandleLogChange(LogMode::WithLog);
        break;
    case Command::Type::LogOff:
        HandleLogChange(LogMode::NoLog);
        break;
    case Command::Type::LogShow:
        HandleLogShow();
        break;
    default:
        Outputer::ErrorLn(command) << "Command not handled in Workspace::Handle";
        return;
    }
    if (m_Success && m_LogMode == LogMode::WithLog) {
        m_Logger->Log(command);
    }
}

void Workspace::CreateEditorByFilePath(std::vector<std::string>::value_type fp) {
    const Ref<Editor> editor = CreateRef<Editor>(fp);
    m_Editors.push_back(editor);
}

void Workspace::HandleLoad()
{
    auto fp = m_CurrentCommand.GetArgs()[0];
    if (auto existing = GetEditorIndexByPath(fp); existing >= 0){
        m_CurrentEditor = existing;
        return;
    }
    try {
        CreateEditorByFilePath(fp);
    } catch (const std::exception& e) {
        Outputer::ErrorLn(m_CurrentCommand) << "Failed to create editor: " << e.what();
        return;
    }

    m_CurrentEditor = static_cast<int>(m_Editors.size()) - 1;
    UpdateLogMode(GetCurrentEditor()->GetLogMode());
    m_Success = true;
}

void Workspace::HandleSave(){
    auto& args = m_CurrentCommand.GetArgs();
    Ref<Editor> target;
    if (args.empty()){
        target = GetCurrentEditor();
        if (target == nullptr){
            Outputer::ErrorLn(m_CurrentCommand) << "No current editor";
            return;
        }
        target->Save();
        return;
    }
    const auto fp = m_CurrentCommand.GetArgs()[0];
    if (fp == "all"){
        for (const auto& editor : m_Editors){
            editor->Save();
        }
    }
    target = GetEditorByPath(fp);
    if (target == nullptr){
        Outputer::ErrorLn(m_CurrentCommand) << "File `" << fp << "` not found in workspace";
        return;
    }
    target->Save();
    GetCurrentEditor()->UpdateTime();
    m_Success = true;
}
void Workspace::HandleInit(){
    auto& args = m_CurrentCommand.GetArgs();
    const auto fp = args[0];
    if (GetEditorIndexByPath(fp) >= 0){
        Outputer::ErrorLn(m_CurrentCommand) << "File `" << args[0] << "` found in workspace";
        return;
    }
    LogMode logMode = LogMode::None;
    if (args.size() >= 2 && args[1] == "with-log") {
        logMode = LogMode::WithLog;
    }
    Ref<Editor> editor;
    try {
        editor = CreateRef<Editor>(fp, logMode);
    } catch (const std::exception& e) {
        Outputer::ErrorLn(m_CurrentCommand) << "Failed to create editor: " << e.what();
        return;
    }
    m_CurrentEditor = static_cast<int>(m_Editors.size());
    m_Editors.push_back(editor);
    GetCurrentEditor()->UpdateTime();
    UpdateLogMode(GetCurrentEditor()->GetLogMode());
    m_Success = true;
}

void Workspace::HandleClose(){
    if (!m_CurrentCommand.GetArgs().empty()){
        // user specified file
        auto fp = m_CurrentCommand.GetArgs()[0];
        int editorIndex = GetEditorIndexByPath(fp);
        if (editorIndex >= 0){
            m_Editors[editorIndex]->AskSaving();
            m_Editors.erase(m_Editors.begin() + editorIndex);
            m_CurrentEditor = GetLastEditorIndex();
        } else{
            Outputer::ErrorLn(m_CurrentCommand) << "File `" << fp << "` not found in workspace";
        }
        return;
    }

    if (m_CurrentEditor < 0 || m_CurrentEditor >= m_Editors.size()){
        Outputer::ErrorLn(m_CurrentCommand) << "No current editor";
        return;
    }

    m_Editors[m_CurrentEditor]->AskSaving();
    Outputer::InfoLn() << "File closed: " << m_Editors[m_CurrentEditor]->GetFilePath();
    m_Editors.erase(m_Editors.begin() + m_CurrentEditor);
    m_CurrentEditor = GetLastEditorIndex();
    if (GetCurrentEditor()) {
        UpdateLogMode(GetCurrentEditor()->GetLogMode());
    }
    m_Success = true;

}
void Workspace::HandleEdit(){
    auto to = GetEditorIndexByPath(m_CurrentCommand.GetArgs()[0]);
    if (to < 0){
        Outputer::ErrorLn(m_CurrentCommand) << "No such editor for file - " << m_CurrentCommand.GetArgs()[0];
        return;
    }
    m_CurrentEditor = to;
    GetCurrentEditor()->UpdateTime();
    UpdateLogMode(GetCurrentEditor()->GetLogMode());
    m_Success = true;
}
void Workspace::HandleEditorList() {
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
    m_Success = true;
}
void Workspace::HandleDirTree() {
    std::string fp;
    if (!m_CurrentCommand.GetArgs().empty()) {
        fp = m_CurrentCommand.GetArgs()[0];
    } else {
        fp = std::filesystem::current_path().string();
    }
    Outputer::Out() << fp << std::endl;
    DrawDirTree(fp, "");
    m_Success = true;
}

void Workspace::HandleLogChange(LogMode logMode) {
    Ref<Editor> targetEditor;

    if (m_CurrentCommand.GetArgs().empty()) {
        targetEditor = GetCurrentEditor();
    } else {
        targetEditor = GetEditorByPath(m_CurrentCommand.GetArgs()[0]);
    }
    if (!targetEditor) {
        Outputer::ErrorLn(m_CurrentCommand) << "No such editor";
        return;
    }
    targetEditor->SetLogMode(logMode);
    m_LogMode = m_CurrentEditor < 0 ? logMode : GetCurrentEditor()->GetLogMode();
    m_Success = true;
}
void Workspace::HandleLogShow() {
    Ref<Editor> targetEditor;

    if (m_CurrentCommand.GetArgs().empty()) {
        targetEditor = GetCurrentEditor();
    } else {
        targetEditor = GetEditorByPath(m_CurrentCommand.GetArgs()[0]);
    }
    if (!targetEditor) {
        Outputer::ErrorLn(m_CurrentCommand) << "No such editor";
        return;
    }
    this->m_Logger->Show();
    targetEditor->GetLogger()->Show();
    m_Success = true;
}

void Workspace::HandleExit() {
    for (const auto& editor : m_Editors) {
        if (editor->IsModified()) {
            editor->AskSaving();
        }
    }
    ExportState();
    m_Running = false;
    m_Success = true;
}

void Workspace::ExportState() const {
    nlohmann::json statusJson = SerializeJson();
    const auto jsonString = statusJson.dump(4);
    if (!std::filesystem::exists("data")) {
        std::filesystem::create_directory("data");
    }
    std::ofstream outFile("data/.editor_workspace");
    if (!outFile.is_open()) {
        Outputer::ErrorLn(m_CurrentCommand) << "Failed to update workspace file";
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
