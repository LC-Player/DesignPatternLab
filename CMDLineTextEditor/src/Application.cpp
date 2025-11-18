// Application.cpp

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include "Application.h"

#include <cassert>

#include "Core.h"
#include "Components/Workspace.h"
#include "Command.h"
#include "Outputer.h"

Application::Application()
{
	std::ifstream in("data/.editor_workspace");
	std::stringstream buffer;
	if (in) {
		buffer << in.rdbuf();
	}
	m_Workspace = CreateScope<Workspace>(buffer.str());
}

Application::~Application()
{
	Outputer::InfoLn() << "Shutting down...";
}

void Application::Run()
{
	while (m_Workspace->GetRunning()) {
		std::string cmdText;
		std::getline(std::cin, cmdText);
		// empty string
		if (cmdText.find_first_not_of(" \n\t") == std::string::npos)
			continue;
		Command command(cmdText);
		// validate the command with error info
		if (!command.Validate())
			continue;
		// handle
		switch (GetExecutorFromType(command.GetType()))
		{
		case CommandExecuting::Workspace:
			assert(m_Workspace != nullptr);
			m_Workspace->Handle(command);
			break;
		case CommandExecuting::Editor: {
			const auto editor = m_Workspace->GetCurrentEditor();
			if (!editor) {
				Outputer::ErrorLn(command) << "no current editor";
				break;
			}
			editor->Handle(command);
			break;
		}
		default:
			throw std::runtime_error("Unknown command");
		}
	}
}
