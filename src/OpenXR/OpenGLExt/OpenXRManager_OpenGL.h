#pragma once
#include "OpenXR/OpenXRManager.h"

#include "OpenXRInstance_OpenGL.h"
#include "OpenXRSession_OpenGL.h"
#include "OpenXR/SwapchainManager.h"
#include "OpenGLSwapchainTraits.h"
#include "OpenGL/Application/Renderer.h"

#include <GLFW/glfw3.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unknwn.h>

class OpenXRManager_OpenGL : public OpenXRManager
{
public:
	inline OpenXRManager_OpenGL(GLFWwindow* window) : mWindow(window)
	{
		mHWND = glfwGetWin32Window(mWindow);
		mHDC = GetDC(mHWND);
		mGLRC = glfwGetWGLContext(mWindow);
	}
	inline ~OpenXRManager_OpenGL()
	{
		delete mRenderer;
		delete mSession;
		delete mSwapchainManager;
		delete mInstance;
	}

	bool Init(XRApplicationInfo applicationInfo) override;
	void Run() override;
	void Destroy() override;

private:
	GLFWwindow* mWindow = nullptr;
	HWND mHWND = nullptr;
	HDC mHDC = nullptr;
	HGLRC mGLRC = nullptr;
	HGLRC sharedContext;

	OpenXRInstance_OpenGL* mInstance = nullptr;
	XRSession_OpenGL* mSession = nullptr;
	SwapchainManager<XrSwapchainImageOpenGLKHR, OpenGLSwapchainTraits>* mSwapchainManager = nullptr;
	Renderer* mRenderer = nullptr;

	std::mutex mGLMutex;

	bool calledDestroy{ false };
};

