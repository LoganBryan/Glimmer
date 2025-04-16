#include "OpenGL/Application/Application.h"
#include <stdio.h>

int main()
{

	Application application(1920, 1080, "Glimmer");
	if (!application.Init())
	{
		printf("Application failed to initialize!");
		return -1;
	}
	application.Run();

	return 0;
}