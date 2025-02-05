#include "Application.h"
#include "Renderer.h"
#include "Camera.h"
#include <stdio.h>
#include <iostream>

Application::Application(int width, int height, const char* title) : width(width), height(height), title(title), window(nullptr) 
{
}

Application::~Application()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

bool Application::Init()
{
	if (!glfwInit())
	{
		printf("Failed to initialize GLFW!\n");
		return false;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!window)
	{
		printf("Failed to create window!\n");
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD!\n");
		return false;
	}

	std::cout << glGetString(GL_RENDERER) << "\n" << glGetString(GL_VERSION) << std::endl;

	//TODO: setup here (callbacks, imgui context etc)


	return true;
}

void Application::ProcessInput()
{

}

void Application::SetupFramebuffers()
{

}

void Application::SetupCallbacks()
{

}

void Application::Run()
{
	Renderer renderer(window);
	renderer.Init();

	while (!glfwWindowShouldClose(window))
	{
		CalculateDelta();
		renderer.Render(width, height);

		Camera::Camera::GetInstance()->ProcessKeyboard(window, deltaTime);

		glfwSwapInterval(0);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

