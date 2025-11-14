// Editor.h

#pragma once
#include <chrono>
#include <stack>
#include <string>
#include <vector>

#include "Command.h"
#include "Core.h"
#include "Logging.h"

std::pair<int, int> ParseRange(const std::string& range);

std::vector<std::string> ParseLineBreaks(const std::string& line, const std::string& seperator = "\\n");

enum class LogMode{
    None,
    WithLog,
    NoLog,
};

struct EditorData {
    bool modified = false;
    std::vector<std::string> lines;
    LogMode logMode = LogMode::None;
};

class Editor : public CommandHandler {
public:
    Editor();
    explicit Editor(const std::string& filePathText, LogMode logMode = LogMode::None);
    void Handle(const Command& command) override;

    void Save();
    void UpdateTime();
    void AskSaving();

    const std::string& GetFilePath() { return m_FilePath; }
    const std::chrono::time_point<std::chrono::system_clock>& GetLastTime() const { return m_LastTime; }
    LogMode GetLogMode() const { return m_Data.logMode; }
    const Ref<Logger>& GetLogger() const { return m_Logger; }
    void SetLogMode(LogMode m) { m_Data.logMode = m; }
    bool IsModified() const { return m_Data.modified; }
    void SetModified(bool m) { m_Data.modified = m; }
    const std::vector<std::string>& GetLines() { return m_Data.lines; };

private:
    bool HandleShow() const;
    bool HandleAppend();
    bool HandleInsert();
    bool HandleDelete();
    bool HandleReplace();
    bool HandleUndo();
    bool HandleRedo();
    friend class EditorModificationScope;
    bool GetAndValidateLineColRange(int& lineIndex, int& col) const;
    void Insert(int lineIndex, int col, const std::vector<std::string>::value_type& raw);
    Scope<EditorData> CreateDataSnapshot();

private:
    std::stack<Scope<EditorData>> m_UndoStack;
    std::stack<Scope<EditorData>> m_RedoStack;
    std::string m_FilePath;
    EditorData m_Data;
    std::chrono::time_point<std::chrono::system_clock> m_LastTime;
    Ref<Logger> m_Logger;
};

class EditorModificationScope {
public:
    explicit EditorModificationScope(Editor* editor)
        : m_Editor(editor), m_Snapshot(editor->CreateDataSnapshot()) {}
    ~EditorModificationScope() {
        m_Editor->m_UndoStack = {};
        m_Editor->m_Data.modified = true;
        m_Editor->UpdateTime();
        m_Editor->m_UndoStack.push(std::move(m_Snapshot));
    }
private:
    Editor* m_Editor;
    Scope<EditorData> m_Snapshot;
};

#define MODIFICATION_SCOPE auto _editorModificationScope = EditorModificationScope(this)