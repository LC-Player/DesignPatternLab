// Command.h

#pragma once
#include <chrono>
#include <ctime>
#include <string>
#include <vector>
#include <unordered_map>

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
	const Type& GetType() const                     { return m_Type; }

	std::chrono::time_point<std::chrono::system_clock> GetTime() const { return m_Time; }

	static std::unordered_map<std::string, Command::Type> s_VerbToType;
private:
	void ParseArguments(const std::string& cmdText, size_t verbEnd);
	bool ValidateArgNums() const;

	Type m_Type = Type::None;
	std::string m_Line;
	std::string m_Verb;
	std::vector<std::string> m_Args;
	std::chrono::time_point<std::chrono::system_clock> m_Time;

};