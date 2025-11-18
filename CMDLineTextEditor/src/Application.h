#pragma once
#include "Components/Workspace.h"
#include "Core.h"
#include "CommandExecuting.h"

class Application {
public:
	Application();
	~Application();
	void Run();

private:
	Scope<Workspace> m_Workspace;
};
