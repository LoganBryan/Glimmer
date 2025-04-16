#pragma once
#ifndef GUIHANDLER_H
#define GUIHANDLER_H

#include <mutex>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

class GUIHandler
{
public:
	GUIHandler(GUIHandler& other) = delete;
	void operator=(const GUIHandler&) = delete;

	static GUIHandler* GetInstance();

	// Needs to be called once
	static void Init(GLFWwindow* window, const char* glslVersion = "#version 460");

	static void Shutdown();

	void NewFrame();
	void Render();

	void BeginFrame(const char* windowName, ImGuiWindowFlags flags = 0, const ImVec2& windowSize = ImVec2(200, 200));
	inline const void EndFrame() { ImGui::End(); }

protected:
	GUIHandler(): mGLSLVersion(nullptr), mWindow(nullptr) {}
	~GUIHandler() {}

private:
	static GUIHandler* pInstance;
	static std::mutex mutex_;

	GLFWwindow* mWindow;
	const char* mGLSLVersion;
};

#endif // !GUIHANDLER_H