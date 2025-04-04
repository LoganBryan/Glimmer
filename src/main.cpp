#include "Application/Application.h"
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

// TODO: everything here needs to be re-implemented (currently just missing callbacks)
//#define TINYGLTF_IMPLEMENTATION
//
//#include <stdio.h>
//#include <string>
//#include <sstream>
//#include <format>
//#include <vector>
//#include <map>
//#include <memory>
//#include <chrono>
//
//#include <glad/glad.h>
//#include <GLFW/glfw3.h>
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//
//#include <imgui.h>
//#include <imgui_impl_glfw.h>
//#include <imgui_impl_opengl3.h>
//
//#include "stb_image.h"
//
//#include <fastgltf/core.hpp>
//#include <fastgltf/types.hpp>
//#include <fastgltf/tools.hpp>
//
//#include "Shader.h"
//#include "Camera.h"
//#include "FPSCounter.h"
//#include "GltfLoader.h"
//
//#define PI 3.14159265358979323846
//#define TWO_PI 6.2831855
//
//// TODO: General code cleanup
//
//// TEMP
//float vertices[] = {
//	// Pos          // Normal           // TexCoord
//	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
//	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
//	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
//	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
//	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
//	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
//
//	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
//	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
//	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
//	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
//	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
//	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
//
//	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
//	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
//	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
//	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
//	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
//	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
//
//	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
//	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
//	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
//	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
//	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
//	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
//
//	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
//	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
//	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
//	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
//	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
//	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
//
//	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
//	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
//	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
//	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
//	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
//	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
//};
//
//float skyboxVertices[] = {
//	// positions          
//	-1.0f,  1.0f, -1.0f,
//	-1.0f, -1.0f, -1.0f,
//	 1.0f, -1.0f, -1.0f,
//	 1.0f, -1.0f, -1.0f,
//	 1.0f,  1.0f, -1.0f,
//	-1.0f,  1.0f, -1.0f,
//
//	-1.0f, -1.0f,  1.0f,
//	-1.0f, -1.0f, -1.0f,
//	-1.0f,  1.0f, -1.0f,
//	-1.0f,  1.0f, -1.0f,
//	-1.0f,  1.0f,  1.0f,
//	-1.0f, -1.0f,  1.0f,
//
//	 1.0f, -1.0f, -1.0f,
//	 1.0f, -1.0f,  1.0f,
//	 1.0f,  1.0f,  1.0f,
//	 1.0f,  1.0f,  1.0f,
//	 1.0f,  1.0f, -1.0f,
//	 1.0f, -1.0f, -1.0f,
//
//	-1.0f, -1.0f,  1.0f,
//	-1.0f,  1.0f,  1.0f,
//	 1.0f,  1.0f,  1.0f,
//	 1.0f,  1.0f,  1.0f,
//	 1.0f, -1.0f,  1.0f,
//	-1.0f, -1.0f,  1.0f,
//
//	-1.0f,  1.0f, -1.0f,
//	 1.0f,  1.0f, -1.0f,
//	 1.0f,  1.0f,  1.0f,
//	 1.0f,  1.0f,  1.0f,
//	-1.0f,  1.0f,  1.0f,
//	-1.0f,  1.0f, -1.0f,
//
//	-1.0f, -1.0f, -1.0f,
//	-1.0f, -1.0f,  1.0f,
//	 1.0f, -1.0f, -1.0f,
//	 1.0f, -1.0f, -1.0f,
//	-1.0f, -1.0f,  1.0f,
//	 1.0f, -1.0f,  1.0f
//};
//
//float quadVertices[] = { // vertex for a quad to fill the screen
//	// positions   // texCoords
//	-1.0f,  1.0f,  0.0f, 1.0f,
//	-1.0f, -1.0f,  0.0f, 0.0f,
//	 1.0f, -1.0f,  1.0f, 0.0f,
//
//	-1.0f,  1.0f,  0.0f, 1.0f,
//	 1.0f, -1.0f,  1.0f, 0.0f,
//	 1.0f,  1.0f,  1.0f, 1.0f
//};
//
//std::vector<glm::vec3> staticLightPositions = {
//	glm::vec3(1.0f,  1.0f,  1.0f),
//	glm::vec3(2.3f, -3.3f, -4.0f),
//	glm::vec3(-4.0f,  2.0f, -12.0f),
//	glm::vec3(0.5f,  0.8f, -3.0f)
//};
//
//std::vector<glm::vec3> lightPositions = {
//	glm::vec3(1.0f,  1.0f,  1.0f),
//	glm::vec3(2.3f, -3.3f, -4.0f),
//	glm::vec3(-4.0f,  2.0f, -12.0f),
//	glm::vec3(0.0f,  0.0f, -3.0f)
//};
//
//std::vector<std::string> skyboxFaces = {
//	"assets/textures/skybox/right.jpg",
//	"assets/textures/skybox/left.jpg",
//	"assets/textures/skybox/top.jpg",
//	"assets/textures/skybox/bottom.jpg",
//	"assets/textures/skybox/front.jpg",
//	"assets/textures/skybox/back.jpg"
//};
//
//// Todo: Needs moving to it's own class
//void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
//void ProcessInput(GLFWwindow* window);
//void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
//unsigned int LoadTexture(char const* path);
//
//// This will eventually be in the camera class v
//void MouseCallback(GLFWwindow* window, double xpos, double ypos); 
//void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
//
//void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLenum id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
//
//GLenum polyMode = GL_FILL;
//
//int width = 1920;
//int height = 1080;
//int nChannels;
//
//unsigned int textureOne, textureTwo;
//std::string imagePath = "assets/textures/boxTex.png";
//std::string image2Path = "assets/textures/boxSpecular.png";
//std::string image3Path = "assets/textures/boxEmission.png";
//std::string uvPath = "assets/textures/uvmap.jpg";
//
//std::string testModel = "assets/models/flightHelm/FlightHelmet.gltf";
//
//bool firstMouseInput = true;
//float lastMouseX = 400, lastMouseY = 400;
//float yaw = -90.0f;
//float pitch = 0.0f;
//float fov = 90.0f;
//
//Camera mainCamera(glm::vec3(0.0f, 0.0f, 3.0f));
//
//glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
//
//unsigned int GenerateCubemap(std::vector<std::string> faces) 
//{
//	unsigned int textureID;
//	glGenTextures(1, &textureID);
//	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
//
//	int width, height, nChannels;
//	unsigned char* data;
//
//	for (unsigned int i = 0; i < faces.size(); i++)
//	{
//		data = stbi_load(faces[i].c_str(), &width, &height, &nChannels, 0);
//		if (data)
//		{
//			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
//		}
//		else
//		{
//			printf("Cubemap texture failed to load at %s", faces[i].c_str());
//		}
//		stbi_image_free(data);
//	}
//
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//
//	return textureID;
//}
//
//int main()
//{
//	glfwInit();
//	glfwWindowHint(GLFW_SAMPLES, 8);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//
//	// Create window context
//	GLFWwindow* window = glfwCreateWindow(width, height, "Glimmer", NULL, NULL);
//	if (window == NULL)
//	{
//		printf("Failed to create window!");
//		glfwTerminate();
//		return -1;
//	}
//	glfwMakeContextCurrent(window);
//	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
//
//	// Initialize OGL Context
//	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
//	{
//		printf("Failed to initialize GLAD!");
//		return -1;
//	}
//
//	Viewer viewer;
//	GltfLoader gltfObject;
//	//GltfLoader gltf2Object;
//	//GltfLoader gltf3Object;
//	auto gltfFile = std::filesystem::path("assets/models/helmet/DamagedHelmet.gltf");
//	//auto gltfFile = std::filesystem::path("assets/models/flightHelm/FlightHelmet.gltf");
//	glfwSetWindowUserPointer(window, &viewer);
//
//	IMGUI_CHECKVERSION();
//	ImGui::CreateContext();
//	ImGui::StyleColorsDark();
//
//	ImGuiIO& io = ImGui::GetIO(); (void)io;
//	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; right now this being enabled breaks the mouse wrapping code..g kjsej g
//	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
//
//	ImGuiStyle& style = ImGui::GetStyle();
//	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
//	{
//		style.WindowRounding = 0.0f;
//		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
//	}
//
//	ImGui_ImplGlfw_InitForOpenGL(window, true);
//	ImGui_ImplOpenGL3_Init("#version 460");
//
//	//glViewport(0, 0, 800, 600);
//	std::cout << glGetString(GL_RENDERER) << "\n" << glGetString(GL_VERSION) << std::endl;
//
//	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//	glfwSetKeyCallback(window, KeyCallback);
//	//glfwSetCursorPosCallback(window, MouseCallback);
//	glfwSetScrollCallback(window, ScrollCallback);
//
//	// Setup shaders
//	Shader mainShader("shaders/shader.vert", "shaders/shader.frag");
//	Shader lightSourceShader("shaders/lightFullBright.vert", "shaders/lightFullBright.frag");
//	Shader outlineShader("shaders/shader.vert", "shaders/singleColor.frag");
//	Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");
//	Shader framebufferShader("shaders/framebuffer.vert", "shaders/framebuffer.frag");
//
//	framebufferShader.Use();
//	framebufferShader.SetInt("screenTexture", 5);
//
//	// Setup framebuffer object
//	unsigned int fbo;
//	glGenFramebuffers(1, &fbo);
//	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//
//	// Create texture attachment
//	unsigned int textureCBuffer;
//	glGenTextures(1, &textureCBuffer);
//	glBindTexture(GL_TEXTURE_2D, textureCBuffer);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureCBuffer, 0); // Attach
//	//glBindTexture(GL_TEXTURE_2D, 0);
//
//	// Setup renderbuffer object attachment
//	unsigned int rbo;
//	glGenRenderbuffers(1, &rbo);
//	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
//	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
//	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // Attach
//	//glBindRenderbuffer(GL_RENDERBUFFER, 0);
//
//	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
//		printf("Framebuffer %d is incomplete!\n", fbo);
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//
//	// Screen quad
//	unsigned int screenQuadVAO, screenQuadVBO;
//	glGenVertexArrays(1, &screenQuadVAO);
//	glGenBuffers(1, &screenQuadVBO);
//	glBindVertexArray(screenQuadVAO);
//	glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
//	glEnableVertexAttribArray(0);
//	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
//	glEnableVertexAttribArray(1);
//	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
//
//	// Object for light source
//	unsigned int VBO;
//	glGenBuffers(1, &VBO);
//
//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//
//	unsigned int lightVAO;
//	glGenVertexArrays(1, &lightVAO);
//	glBindVertexArray(lightVAO);
//
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
//	glEnableVertexAttribArray(0);
//
//	// Unbind
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	glBindVertexArray(0);
//
//	// Load skybox textures
//	unsigned int skyboxTexture = GenerateCubemap(skyboxFaces);
//
//	// Skybox object
//	unsigned int skyboxVAO, skyboxVBO;
//	glGenVertexArrays(1, &skyboxVAO);
//	glGenBuffers(1, &skyboxVBO);
//	glBindVertexArray(skyboxVAO);
//	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
//	glEnableVertexAttribArray(0);
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
//
//	skyboxShader.Use();
//	skyboxShader.SetInt("skybox", 0);
//
//	gltfObject.LoadModel(gltfFile, mainShader);
//	//gltf2Object.LoadModel(gltfFile, mainShader);
//	//gltf3Object.LoadModel(gltfFile, mainShader);
//
//	FPSCounter fpsCounter;
//	glEnable(GL_DEPTH_TEST);
//	glDepthFunc(GL_LESS);
//
//	glEnable(GL_CULL_FACE);
//	glCullFace(GL_BACK);
//
//	glEnable(GL_STENCIL_TEST);
//
//	glDisable(GL_BLEND);
//
//	// Main loop
//	while (!glfwWindowShouldClose(window))
//	{
//		// Input process
//		ProcessInput(window);
//		//mainCamera.HandleCameraInput(window);
//
//		fpsCounter.Update();
//		std::stringstream windowTitle;
//		windowTitle << "Glimmer [ " << fpsCounter.GetFPS() << " fps ]";
//		glfwSetWindowTitle(window, windowTitle.str().c_str());
//
//		//ImGui_ImplOpenGL3_NewFrame();
//		//ImGui_ImplGlfw_NewFrame();
//		//ImGui::NewFrame();
//
//		//ImGui::ShowDemoWindow();
//
//		// Render
//		glClearColor(0.25f, 0.25f, 0.4f, 1.0f);
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
//		glStencilFunc(GL_EQUAL, 1, 0xFF);
//		glClearColor(0.25f, 0.25f, 0.8f, 1.0f);
//
//		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
//
//		glPolygonMode(GL_FRONT_AND_BACK, polyMode);
//
//		glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 100.0f);
//		glm::mat4 view = mainCamera.Update();
//		glm::mat4 model = glm::mat4(1.0f);
//
//		glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Draw to framebuffer
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//		glEnable(GL_DEPTH_TEST);
//
//		// gLTF object
//		mainShader.Use();
//		glStencilFunc(GL_ALWAYS, 1, 0xFF); // All fragments pass stencil test
//		glStencilMask(0xFF); // Enable writing to stencil buffer
//
//		mainShader.SetMatrix4("projection", projection);
//		mainShader.SetMatrix4("view", view);
//
//		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
//		mainShader.SetMatrix4("model", model);
//
//		gltfObject.DrawModel();
//
//
//		//model = glm::translate(model, glm::vec3(2.5f, 0.0f, 0.0f));
//		//mainShader.SetMatrix4("model", model);
//		//gltf2Object.DrawModel();
//
//		//model = glm::translate(model, glm::vec3(-5.0f, 0.0f, 0.0f));
//		//mainShader.SetMatrix4("model", model);
//		//gltf3Object.DrawModel();
//
//
//
//		//DrawModel(exModel);
//
//		//DrawModel(vertElementbuffers, exModel);
//
//		//// gLTF object -- Scaled
//		//glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
//		//glStencilMask(0x00);
//		//glDisable(GL_DEPTH_TEST);
//
//		//outlineShader.Use();
//
//		//outlineShader.SetMatrix4("projection", projection);
//		//outlineShader.SetMatrix4("view", view);
//
//		////glm::mat4 model = glm::mat4(1.0f);
//		//model = glm::scale(model, glm::vec3(1.1, 1.1, 1.1));
//		//outlineShader.SetMatrix4("model", model);
//
//		//DrawModel(vertElementbuffers, exModel);
//
//		//if (!asset.scenes.empty() && sceneIndex < asset.scenes.size())
//		//{
//		//	fastgltf::iterateSceneNodes(asset, sceneIndex, fastgltf::math::fmat4x4(), [&](fastgltf::Node& node, fastgltf::math::fmat4x4 matrix)
//		//		{
//		//			if (node.meshIndex.has_value()) 
//		//				DrawGLTFMesh(&viewer, *node.meshIndex, matrix);
//		//		});
//		//}
//
//		//glStencilMask(0xFF);
//		//glStencilFunc(GL_ALWAYS, 1, 0xFF);
//		//glEnable(GL_DEPTH_TEST);
//
//		// Light object
//		lightSourceShader.Use();
//		lightSourceShader.SetMatrix4("projection", projection);
//		lightSourceShader.SetMatrix4("view", view);
//
//		glBindVertexArray(lightVAO);
//
//		//TODO: Should eventually be changed to allow adding/ removing pointlights, but this should be simple to change		
//
//		model = glm::translate(model, lightPositions[0]);
//		model = glm::scale(model, glm::vec3(0.3f));
//		lightSourceShader.SetMatrix4("model", model);
//
//		lightPositions[0].x = sin((float)glfwGetTime() + 6) * staticLightPositions[0].x * 2.5;
//		lightPositions[0].y = sin((float)glfwGetTime() + 4) * staticLightPositions[0].y * 6;
//		lightPositions[0].z = sin((float)glfwGetTime() + 2);
//
//		glDisable(GL_CULL_FACE);
//		glDrawArrays(GL_TRIANGLES, 0, 36);
//		glEnable(GL_CULL_FACE);
//
//		// Skybox - Drawn last
//		glDepthFunc(GL_LEQUAL);
//		skyboxShader.Use();
//		view = glm::mat4(glm::mat3(mainCamera.GetViewMatrix()));
//
//		skyboxShader.SetMatrix4("view", view);
//		skyboxShader.SetMatrix4("projection", projection);
//
//		// Draw skybox cube
//		glBindVertexArray(skyboxVAO);
//		glActiveTexture(GL_TEXTURE5);
//		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
//		glDrawArrays(GL_TRIANGLES, 0, 36);
//		glBindVertexArray(0);
//		glDepthFunc(GL_LESS);
//
//		glBindFramebuffer(GL_FRAMEBUFFER, 0); // Revert to default framebuffer
//
//		//Render frame buffer
//		framebufferShader.Use();
//		glBindVertexArray(screenQuadVAO);
//		glDisable(GL_DEPTH_TEST);
//		glActiveTexture(GL_TEXTURE5);
//		glBindTexture(GL_TEXTURE_2D, textureCBuffer);
//		glDrawArrays(GL_TRIANGLES, 0, 6);
//
//		// Start ImGui frame
//		ImGui_ImplOpenGL3_NewFrame();
//		ImGui_ImplGlfw_NewFrame();
//		ImGui::NewFrame();
//
//		static ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking;
//		ImGui::Begin("Viewport", (bool*)false, flags); // TODO: Recreate on resize
//		ImGui::SetWindowSize(ImVec2(854, 480));
//
//		ImVec2 viewportPos = ImGui::GetCursorScreenPos();
//		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
//
//		// TODO Should use quaternions for camera hhhhhhh
//		static bool isWarping = false;
//		if (ImGui::IsWindowHovered())
//		{
//			if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
//			{
//				double currentMouseX, currentMouseY;
//				glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
//
//				if (firstMouseInput)
//				{
//					lastMouseX = currentMouseX;
//					lastMouseY = currentMouseY;
//					firstMouseInput = false;
//				}
//
//				if (!isWarping)
//				{
//					float xOffset = static_cast<float>(currentMouseX - lastMouseX);
//					float yOffset = static_cast<float>(lastMouseY - currentMouseY);
//
//					const float speed = 0.25f;
//					xOffset *= speed;
//					yOffset *= speed;
//
//
//					yaw += xOffset;
//					pitch += yOffset;
//
//					pitch = glm::clamp(pitch, -89.0f, 89.0f);
//
//					if (pitch > 89.0f) pitch = 89.0f;
//					else if (pitch < -89.0f) pitch = -89.0f;
//
//					glm::vec3 direction;
//					direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
//					direction.y = sin(glm::radians(pitch));
//					direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
//					mainCamera.SetFront(glm::normalize(direction));
//				}
//
//				isWarping = false;
//
//				lastMouseX = currentMouseX;
//				lastMouseY = currentMouseY;
//
//				// Handle edge wrapping
//				if (currentMouseX <= viewportPos.x)
//				{
//					glfwSetCursorPos(window, viewportPos.x + viewportSize.x - 1, currentMouseY);
//					lastMouseX = viewportPos.x + viewportSize.x - 1;
//					isWarping = true;
//				}
//				else if (currentMouseX >= viewportPos.x + viewportSize.x - 1)
//				{
//					glfwSetCursorPos(window, viewportPos.x + 1, currentMouseY);
//					lastMouseX = viewportPos.x + 1;
//					isWarping = true;
//				}
//
//				if (currentMouseY <= viewportPos.y)
//				{
//					glfwSetCursorPos(window, currentMouseX, viewportPos.y + viewportSize.y - 1);
//					lastMouseY = viewportPos.y + viewportSize.y - 1;
//					isWarping = true;
//				}
//				else if (currentMouseY >= viewportPos.y + viewportSize.y - 1)
//				{
//					glfwSetCursorPos(window, currentMouseX, viewportPos.y + 1);
//					lastMouseY = viewportPos.y + 1;
//					isWarping = true;
//				}
//			}
//			else
//			{
//				firstMouseInput = true;
//			}
//
//			//ImGui::SetMouseCursor(ImGuiMouseCursor_None);
//
//			const float cameraSpeed = 2.5f * io.DeltaTime;
//
//			if (ImGui::IsKeyDown(ImGuiKey_W) || (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS))
//			{
//				mainCamera.cameraPos += cameraSpeed * mainCamera.cameraFront;
//			}
//			if (ImGui::IsKeyDown(ImGuiKey_S) || (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS))
//			{
//				mainCamera.cameraPos -= cameraSpeed * mainCamera.cameraFront;
//			}
//			if (ImGui::IsKeyDown(ImGuiKey_A) || (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS))
//			{
//				mainCamera.cameraPos -= glm::normalize(glm::cross(mainCamera.cameraFront, mainCamera.cameraUp)) * cameraSpeed;
//			}
//			if (ImGui::IsKeyDown(ImGuiKey_D) || (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS))
//			{
//				mainCamera.cameraPos += glm::normalize(glm::cross(mainCamera.cameraFront, mainCamera.cameraUp)) * cameraSpeed;
//			}
//		}
//		else
//		{
//			firstMouseInput = true;
//		}
//
//
//		ImGui::Image((ImTextureID)(intptr_t)textureCBuffer, viewportSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
//
//		ImGui::End();
//
//		// Render ImGui
//		ImGui::Render();
//		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//
//		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
//		{
//			GLFWwindow* glfwCurrentContext = glfwGetCurrentContext();
//			ImGui::UpdatePlatformWindows();
//			ImGui::RenderPlatformWindowsDefault();
//			glfwMakeContextCurrent(glfwCurrentContext);
//		}
//
//		// Call events + swap buffers
//		glfwSwapInterval(0);
//		glfwSwapBuffers(window);
//		glfwPollEvents();
//	}
//
//	// Deallocate
//	//glDeleteVertexArrays(1, &vertElementbuffers.first);
//	glDeleteVertexArrays(1, &skyboxVAO);
//	glDeleteVertexArrays(1, &screenQuadVAO);
//	glDeleteBuffers(1, &skyboxVBO);
//	glDeleteBuffers(1, &screenQuadVBO);
//	glDeleteRenderbuffers(1, &rbo);
//	glDeleteFramebuffers(1, &fbo);
//	glDeleteVertexArrays(1, &lightVAO);
//	glDeleteBuffers(1, &VBO);
//
//	ImGui_ImplOpenGL3_Shutdown();
//	ImGui_ImplGlfw_Shutdown();
//	ImGui::DestroyContext();
//
//	for (auto& mesh : viewer.meshes)
//	{
//		glDeleteBuffers(1, &mesh.drawBuffer);
//
//		for (auto& primitive : mesh.primitives)
//		{
//			glDeleteVertexArrays(1, &primitive.vertexArray);
//			glDeleteBuffers(1, &primitive.indexBuffer);
//			glDeleteBuffers(1, &primitive.vertexBuffer);
//		}
//	}
//
//	//CleanupBuffers(vertElementbuffers, exModel);
//	mainShader.Delete();
//
//	glfwDestroyWindow(window);
//	glfwTerminate();
//	return 0;
//}
//
//void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
//{
//	glViewport(0, 0, width, height);
//}
//
//void ProcessInput(GLFWwindow* window)
//{
//	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//	{
//		glfwSetWindowShouldClose(window, true);
//	}
//
//}
//
//void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods)
//{
//	if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
//	{
//		polyMode = polyMode == GL_FILL ? GL_LINE : GL_FILL;
//	}
//}
//
//// Pretty rough function, but for now it's fine
//unsigned int LoadTexture(char const* path)
//{
//	unsigned int textureID;
//	glGenTextures(1, &textureID);
//
//	int width, height, nComponents;
//	unsigned char* data = stbi_load(path, &width, &height, &nComponents, 0);
//
//	if (data)
//	{
//		GLenum format{};
//		if (nComponents == 1)
//		{
//			format = GL_RED;
//		}
//		else if (nComponents == 3)
//		{
//			format = GL_RGB;
//		}
//		else if (nComponents == 4)
//		{
//			format = GL_RGBA;
//		}
//
//		glBindTexture(GL_TEXTURE_2D, textureID);
//		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
//		glGenerateMipmap(GL_TEXTURE_2D);
//
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//
//		stbi_image_free(data);
//	}
//	else
//	{
//		printf("Failed to load texture at path: %s!", path);
//		stbi_image_free(data);
//	}
//
//	return textureID;
//}
//
//void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
//{
//	fov -= (float)yoffset;
//	if (fov < 1.0f)
//		fov = 1.0f;
//	if (fov > 90.0f)
//		fov = 90.0f;
//}
//
//void MessageCallback(GLenum source, GLenum type, GLenum id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
//{
//	std::cerr << "OGL Debug\n";
//	printf("Source: 0x%x | Type: 0x%x | ID: %u\n Severity: 0x%x\n%s\n\n", source, type, id, severity, message);
//}
