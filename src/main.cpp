#include "OpenXR/OpenGLExt/OpenXRManager_OpenGL.h"
#include "OpenGL/Application/Application.h"

//int main()
//{
//	Application application(1920, 1080, "Glimmer");
//	if (!application.Init())
//	{
//		printf("Application failed to initialize!");
//		return -1;
//	}
//
//	OpenXRManager_OpenGL xrApplication(application.GetWindow());
//	if (!xrApplication.Init({"Glimmer", "Glimmer XR Engine", 1, 1}))
//	{
//		printf("XR Application failed to initialize!");
//	}
//
//	xrApplication.Run();
//	xrApplication.Destroy();
//
//	return 0;
//}

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