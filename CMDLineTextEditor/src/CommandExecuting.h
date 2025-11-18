#pragma once

#include <Command.h>

enum class CommandExecuting {
    None,
    Workspace, Editor
};

inline CommandExecuting GetExecutorFromType(Command::Type type) {
    if (type == Command::Type::None) {
        return CommandExecuting::None;
    }
    return (Command::Type::WorkspaceCommandBegin < type && type < Command::Type::WorkspaceCommandEnd) ? CommandExecuting::Workspace :
        ((Command::Type::EditorCommandBegin < type && type < Command::Type::EditorCommandEnd) ? CommandExecuting::Editor :
            CommandExecuting::None);
}


// inherited by Workspace and Editor
class CommandExecutor
{
public:
    virtual ~CommandExecutor() = default;
    virtual void Handle(const Command& command) = 0;
protected:
    virtual void RegisterCommandHandlingStrategies() = 0;
};

template<typename T>
class CommandDispatcher {
public:
    using Strategy = bool (T::*)(const Command&);
    void Register(Command::Type type, Strategy strategy) {
        m_Strategies[type] = strategy;
    }
    bool Dispatch(T* obj, const Command& command) {
        auto it = m_Strategies.find(command.GetType());
        if (it == m_Strategies.end()) {
            return false;
        }
        return (obj->*(it->second))(command);
    }
private:
    std::unordered_map<Command::Type, Strategy> m_Strategies;

};