#include "Renderer.h"
#include "GeometryData.h"
#include "GUIHandler.h"
#include "Utils.h"
#include "Camera.h"
#include <stb_image.h>

std::vector<std::string> skyboxFaces = {
	"assets/textures/skybox/right.jpg",
	"assets/textures/skybox/left.jpg",
	"assets/textures/skybox/top.jpg",
	"assets/textures/skybox/bottom.jpg",
	"assets/textures/skybox/front.jpg",
	"assets/textures/skybox/back.jpg"
};

Renderer::Renderer(GLFWwindow* window) : window(window), skyboxTexture(0), skyboxVAO(0), skyboxVBO(0)
{
}

Renderer::~Renderer()
{
	// Delete buffers, shaders etc...

	skyboxShader.Delete();
	glDeleteTextures(1, &skyboxTexture);
	skyboxTexture = 0;
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteBuffers(1, &skyboxVBO);
}

void Renderer::Init()
{
	// Init shaders, load models, setup buffers etc
	skyboxShader.Load("shaders/skybox.vert", "shaders/skybox.frag");
	mainShader.Load("shaders/shader.vert", "shaders/shader.frag");

	// Load skybox textures
	skyboxTexture = Utils::GenerateCubemap(skyboxFaces);

	Viewer viewer;
	//GltfLoader gltf2Object;
	//GltfLoader gltf3Object;
	auto gltfFile = std::filesystem::path("assets/models/helmet/DamagedHelmet.gltf");
	//auto gltfFile = std::filesystem::path("assets/models/flightHelm/FlightHelmet.gltf");
	glfwSetWindowUserPointer(window, &viewer);

	// Skybox object
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, GeometryData::skyboxVerticesSize, &GeometryData::skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	skyboxShader.Use();
	skyboxShader.SetInt("skybox", 0);

	gltfObject.LoadModel(gltfFile, mainShader);


	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_STENCIL_TEST);

	glDisable(GL_BLEND);
}

void Renderer::Render(float width, float height)
{
	glClearColor(0.25f, 0.25f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	const float aspect = static_cast<float>(width) / static_cast<float>(height);
	const float nearPlane = 0.1f;
	const float farPlane = 100.0f;

	glm::mat4 model = glm::mat4(1.0f);
	CameraMatrices camMatrices = Camera::GetInstance()->GetMVP(aspect, nearPlane, farPlane, model);

	// gLTF object
	mainShader.Use();
	glStencilFunc(GL_ALWAYS, 1, 0xFF); // All fragments pass stencil test
	glStencilMask(0xFF); // Enable writing to stencil buffer

	mainShader.SetMatrix4("projection", camMatrices.projection);
	mainShader.SetMatrix4("view", camMatrices.view);

	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	mainShader.SetMatrix4("model", model);

	gltfObject.DrawModel();

	// Skybox - Drawn last
	glDepthFunc(GL_LEQUAL);

	skyboxShader.Use();
	skyboxShader.SetMatrix4("view", camMatrices.viewNormal);
	skyboxShader.SetMatrix4("projection", camMatrices.projection);

	// Draw skybox cube
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

	GUIHandler* gui = GUIHandler::GetInstance();
	
	gui->NewFrame();
	gui->BeginFrame("Test Window", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking, ImVec2(854, 480));
	ImGui::Text("Test!");
	gui->EndFrame();

	gui->Render();
}
