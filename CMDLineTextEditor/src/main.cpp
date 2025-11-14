#include "Application.h"

int main(int argc, char** argv) {
	const auto application = new Application();
	application->Run();
	delete application;
	return 0;
}