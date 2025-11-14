#include "Command.h"
#include "Outputer.h"

std::unordered_map<std::string, Command::Type> Command::s_VerbToType = {
	{"load", Command::Type::Load},
	{"save", Command::Type::Save},
	{"init", Command::Type::Init},
	{"close", Command::Type::Close},
	{"edit", Command::Type::Edit},
	{"editor-list", Command::Type::EditorList},
	{"dir-tree", Command::Type::DirTree},
	{"undo", Command::Type::Undo},
	{"redo", Command::Type::Redo},
	{"exit", Command::Type::Exit},

	{"append", Command::Type::Append},
	{"insert", Command::Type::Insert},
	{"delete", Command::Type::Delete},
	{"replace", Command::Type::Replace},
	{"show", Command::Type::Show},

	{"log-on", Command::Type::LogOn},
	{"log-off", Command::Type::LogOff},
	{"log-show", Command::Type::LogShow}
};

void Command::ParseArguments(const std::string& cmdText, size_t verbEnd) {
	// parse arguments
	size_t argStart = cmdText.find_first_not_of(' ', verbEnd), argEnd;
	while (argStart != std::string::npos) {
		std::string currentArg;
		if (cmdText[argStart] == '"') {
			argEnd = cmdText.find_first_of('"', argStart + 1);
			if (argEnd == std::string::npos) {
				currentArg = cmdText.substr(argStart + 1);
			} else {
				currentArg = cmdText.substr(argStart + 1, argEnd - argStart - 1);
				argStart = argEnd + 1;
			}
		} else {
			argEnd = cmdText.find_first_of(' ', argStart);
			if (argEnd == std::string::npos) {
				currentArg = cmdText.substr(argStart);
			}
			else {
				currentArg = cmdText.substr(argStart, argEnd - argStart);
				argStart = argEnd + 1;
			}
		}

		m_Args.emplace_back(currentArg);
		if (argEnd == std::string::npos) {
			break;
		}
		if (argStart != std::string::npos) {
			argStart = cmdText.find_first_not_of(' ', argEnd + 1);
		}
	}
}

Command::Command(const std::string& cmdText)
{
	size_t verbStart = cmdText.find_first_not_of(' ');
	size_t verbEnd = cmdText.find_first_of(' ', verbStart);
	if (verbStart == std::string::npos) {
		m_Verb = "";
	}
	else if (verbEnd == std::string::npos) {
		m_Verb = cmdText.substr(verbStart);
	}
	else {
		m_Verb = cmdText.substr(verbStart, verbEnd);
	}
	if (s_VerbToType.find(m_Verb) == s_VerbToType.end()) {
		m_Type = Type::None;
		m_Handler = HandlerType::None;
		return;
	}
	m_Type = s_VerbToType[m_Verb];
	m_Handler = GetHandlerFromType(m_Type);

	ParseArguments(cmdText, verbEnd);

	m_Line = cmdText;

	m_Time = std::chrono::system_clock::now();
}

bool Command::Validate() const
{
	if (m_Type == Type::None) {
		Outputer::ErrorLn(*this) << "unknown command";
		return false;
	}

	if (!ValidateArgNums()) {
		Outputer::ErrorLn(*this) << "too many or too few arguments";
		return false;
	}
	return true;
}

bool Command::ValidateArgNums() const
{
	switch (m_Type)
	{
	case Type::Undo: // 0
	case Type::Redo: // 0
	case Type::Exit: // 0
	case Type::EditorList: // 0
		return (m_Args.size() == 0); // NOLINT(*-container-size-empty)
	case Type::Load: // 1
	case Type::Edit: // 1
	case Type::Append: // 1
		return (m_Args.size() == 1);
	case Type::Insert: // 2
	case Type::Delete: // 2
		return (m_Args.size() == 2);
	case Type::Replace: // 3
		return (m_Args.size() == 3);
	case Type::Save: // 0 1
	case Type::Close: // 0 1
	case Type::DirTree: // 0 1
	case Type::Show: // 0 1
	case Type::LogOn: // 0 1
	case Type::LogOff: // 0 1
	case Type::LogShow: // 0 1
		return (m_Args.size() <= 1);
	case Type::Init: // 1 2
		return (m_Args.size() == 1 || m_Args.size() == 2);
	default:
		return false;
	}
}
