// Command.h

#pragma once
#include <chrono>
#include <ctime>
#include <string>
#include <vector>
#include <unordered_map>

enum class HandlerType {
	None,
	Workspace, Editor
};
class Command {
public:
	enum class Type {
		None,
		WorkspaceCommandBegin, Load, Save, Init, Close, Edit,
		EditorList, DirTree, Exit, LogOn, LogOff, LogShow, WorkspaceCommandEnd,
		EditorCommandBegin, Append, Insert, Delete, Replace, Show, Undo, Redo, EditorCommandEnd,
	};

	Command() = default;
	Command(const std::string& cmdText) ;
	~Command() = default;

	bool Validate() const;

	const std::string& GetVerb() const				{ return m_Verb; }
	const std::string& GetLine() const				{ return m_Line; }
	const std::vector<std::string>& GetArgs() const { return m_Args; }
	const Type& GetType() const                      { return m_Type; }

	const HandlerType& GetHandler() const		    { return m_Handler; }
	std::chrono::time_point<std::chrono::system_clock> GetTime() const { return m_Time; }

	static std::unordered_map<std::string, Command::Type> s_VerbToType;
private:
	void ParseArguments(const std::string& cmdText, size_t verbEnd);
	bool ValidateArgNums() const;

	Type m_Type = Type::None;
	HandlerType m_Handler = HandlerType::None;
	std::string m_Line;
	std::string m_Verb;
	std::vector<std::string> m_Args;
	std::chrono::time_point<std::chrono::system_clock> m_Time;

};

static HandlerType GetHandlerFromType(Command::Type type) {
	if (type == Command::Type::None) {
		return HandlerType::None;
	}
	return (Command::Type::WorkspaceCommandBegin < type && type < Command::Type::WorkspaceCommandEnd) ? HandlerType::Workspace :
		((Command::Type::EditorCommandBegin < type && type < Command::Type::EditorCommandEnd) ? HandlerType::Editor :
			HandlerType::None);
}

// inherited by Workspace, Editor, and Logger
class CommandHandler
{
public:
	virtual ~CommandHandler() = default;
	virtual void Handle(const Command& command) = 0;
	Command m_CurrentCommand;
};