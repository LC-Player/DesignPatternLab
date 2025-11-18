// Workspace.h

#pragma once
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "Core.h"
#include "Command.h"
#include "Logging.h"
#include "Editor.h"
#include "CommandExecuting.h"


class Workspace : public CommandExecutor{
public:
	explicit Workspace(const std::string& workspaceData);
	~Workspace() override = default;

	Ref<Editor> GetCurrentEditor() {
		if (m_CurrentEditor < 0 || m_CurrentEditor >= m_Editors.size())
			return nullptr;
		return m_Editors[m_CurrentEditor];
	}
	int GetCurrentEditorIndex() const { return m_CurrentEditor; }
	LogMode GetLogMode() const { return m_LogMode; }

	void Handle(const Command& command) override;
	void CreateEditorByFilePath(const std::string& fp);

	bool GetRunning() const { return m_Running; }
protected:
	void RegisterCommandHandlingStrategies() override;
private:
	bool HandleLoad       (const Command& command);
	bool HandleSave       (const Command& command);
	bool HandleInit       (const Command& command);
	bool HandleClose      (const Command& command);
	bool HandleEdit       (const Command& command);
	bool HandleEditorList (const Command& command);
	bool HandleDirTree    (const Command& command);
	bool HandleLogOn      (const Command& command);
	bool HandleLogOff     (const Command& command);
	bool HandleLogShow    (const Command& command);
	bool HandleExit       (const Command& command);

	int GetLastEditorIndex() const;

	void ExportState() const ;
	nlohmann::json SerializeJson() const;
	void DeserializeJson(const nlohmann::json& j);
	Ref<Editor> GetEditorByPath(const std::string& path) const;
	int GetEditorIndexByPath(const std::string& path) const;
	void UpdateLogMode(LogMode logMode);

private:
	// using CommandStrategy = bool (Workspace::*)(const Command&);
	// static const std::unordered_map<Command::Type, CommandStrategy> s_HandlerMethods;
	CommandDispatcher<Workspace> m_Dispatcher;
	int m_CurrentEditor;
	std::vector<Ref<Editor>> m_Editors;
	bool m_Running = true;
	LogMode m_LogMode;
	Scope<Logger> m_Logger;
};

