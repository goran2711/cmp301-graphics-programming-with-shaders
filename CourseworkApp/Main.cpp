// Main.cpp
#include "../DXFramework/System.h"
#include "CourseworkApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	CourseworkApp* app = new CourseworkApp();
	System* system;

	// Create the system object.
	system = new System(app);

	// Initialize and run the system object.
	system->run();

	// Shutdown and release the system object.
	delete system;
	system = 0;

	return 0;
}