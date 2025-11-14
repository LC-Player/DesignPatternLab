// Workspace.h

#pragma once
#include <string>
#include <vector>

#include "Command.h"
#include "Core.h"
#include "Editor.h"
#include "Logging.h"
#include "nlohmann/json.hpp"

class Workspace : public CommandHandler{
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
	void CreateEditorByFilePath(std::vector<std::string>::value_type fp);

	bool GetRunning() const { return m_Running; }
private:
	void HandleLoad();
	void HandleSave();
	void HandleInit();
	void HandleClose();
	void HandleEdit();
	void HandleEditorList();
	void HandleDirTree();
	void HandleLogChange(LogMode logMode);
	void HandleLogShow();
	int GetLastEditorIndex() const;

	void ExportState() const ;
	nlohmann::json SerializeJson() const;
	void DeserializeJson(const nlohmann::json& j);
	void HandleExit();
private:
	int m_CurrentEditor;
	Ref<Editor> GetEditorByPath(const std::string& path) const;
	int GetEditorIndexByPath(const std::string& path) const;
	void UpdateLogMode(LogMode logMode);
	std::vector<Ref<Editor>> m_Editors;
	bool m_Running = true;
	LogMode m_LogMode;
	Scope<Logger> m_Logger;
	bool m_Success = false;
};

