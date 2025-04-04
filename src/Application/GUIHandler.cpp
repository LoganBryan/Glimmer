#include "GUIHandler.h"

GUIHandler* GUIHandler::pInstance{ nullptr };
std::mutex GUIHandler::mutex_;

GUIHandler* GUIHandler::GetInstance()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (pInstance == nullptr)
	{
		pInstance = new GUIHandler();
	}
	return pInstance;
}

void GUIHandler::Init(GLFWwindow* window, const char* glslVersion)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	
	ImGui::StyleColorsDark();
	
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	
	// Init GLFW and OGL backend
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glslVersion);
}

void GUIHandler::Shutdown()
{
	if (pInstance)
	{
		delete pInstance;
		pInstance = nullptr;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void GUIHandler::NewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void GUIHandler::Render()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* currentContext = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(currentContext);
	}
}

void GUIHandler::BeginFrame(const char* windowName, ImGuiWindowFlags flags, const ImVec2& windowSize)
{
	ImGui::Begin(windowName, nullptr, flags);
	ImGui::SetWindowSize(windowSize);
}

