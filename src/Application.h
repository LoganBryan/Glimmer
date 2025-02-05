#pragma once
#ifndef APPLICATION_H
#define APPLICATION_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Application
{
public:
	Application(int width, int height, const char* title);
	~Application();

	// Window, Context, ImGui etc..
	bool Init();

	void Run();
private:
	void ProcessInput();
	void SetupFramebuffers();
	void SetupCallbacks();

	inline void CalculateDelta() 
	{ 
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
	}

	GLFWwindow* window;
	int width, height;
	const char* title;

	float lastFrame;
	float deltaTime;
};

#endif // !APPLICATION_H

