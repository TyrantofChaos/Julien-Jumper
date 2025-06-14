// handles everything
#include "Application.h"

// program entry point
int main()
{
	Application exampleSpaceGame;
	if (exampleSpaceGame.MyInit()) {
		if (exampleSpaceGame.MyRun()) {
			return exampleSpaceGame.MyShutdown() ? 0 : 1;
		}
	}
	return 1;
}