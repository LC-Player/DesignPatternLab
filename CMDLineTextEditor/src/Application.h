#pragma once
#include "Workspace.h"
#include "Core.h"

class Application {
public:
	Application();
	~Application();
	void Run();

private:
	Scope<Workspace> m_Workspace;
};
