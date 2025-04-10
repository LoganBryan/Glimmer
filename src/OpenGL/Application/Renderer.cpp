#include "Renderer.h"
#include "OpenGL/Model/GeometryData.h"
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
	//mainShader.Load("shaders/xrShader.vert", "shaders/xrShader.frag");

	// Load skybox textures
	skyboxTexture = Utils::GenerateCubemapCompressed(skyboxFaces);

	Viewer viewer;
	//GltfLoader gltf2Object;
	//GltfLoader gltf3Object;
	auto gltfFile = std::filesystem::path("assets/models/helmet/DamagedHelmet.gltf");
	auto controller = std::filesystem::path("assets/models/cube/Cube.gltf");
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

	//gltfObject.LoadModel(gltfFile, mainShader);
	//gltfObject.GetTransform().position = glm::vec3(0.0f, 1.0f, -0.5f);
	//gltfObject.GetTransform().rotation = glm::vec3(1.5f, 0.0f, 0.0f);
	//gltfObject.GetTransform().scale = glm::vec3(0.25f);

	// TODO: Should probably seperate scene setup from renderer, and then extend addobject function (to add a unique id, DisplayName, set transform matrix etc)
	AddObject(gltfFile);
	AddObject(gltfFile);
	AddObject(gltfFile);

	sceneObjects[0]->transform.position = glm::vec3(0.0f, 1.0f, -0.5f);
	sceneObjects[0]->transform.rotation = glm::vec3(1.5f, 0.0f, 0.0f);
	sceneObjects[0]->transform.scale = glm::vec3(0.25f);

	sceneObjects[1]->transform.position = glm::vec3(-2.0f, 1.0f, -0.5f);
	sceneObjects[1]->transform.rotation = glm::vec3(1.5f, 0.0f, 0.0f);
	sceneObjects[1]->transform.scale = glm::vec3(0.5f);

	sceneObjects[2]->transform.position = glm::vec3(2.0f, 1.0f, -0.5f);
	sceneObjects[2]->transform.rotation = glm::vec3(1.5f, 0.0f, 0.0f);
	sceneObjects[2]->transform.scale = glm::vec3(0.75f);

	handObjects[0].LoadModel(controller, mainShader);
	handObjects[1].LoadModel(controller, mainShader);

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

	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	//mainShader.SetMatrix4("model", model);

	//gltfObject.DrawModel();

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

void Renderer::RenderEye(glm::mat4& view, glm::mat4& projection, GLuint framebuffer, int width, int height)
{
	if (framebuffer == 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			// Handle context loss
			glfwMakeContextCurrent(window);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	}

	glViewport(0, 0, width, height);

	glClearColor(0.25f, 0.25f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	mainShader.Use();
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);

	mainShader.SetMatrix4("projection", projection);
	mainShader.SetMatrix4("view", view);

	// Render object(s)
	//glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	//glm::mat4 model = gltfObject.GetTransform().GetMatrix();
	//model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
	//mainShader.SetMatrix4("model", model);
	//gltfObject.DrawModel();

	// TODO: BATCH RENDERING!!!
	for (auto& obj : sceneObjects)
	{
		glm::mat4 model = obj->transform.GetMatrix();
		mainShader.SetMatrix4("model", model);
		obj->model.DrawModel();
	}

	// Render skybox
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	skyboxShader.Use();
	glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
	skyboxShader.SetMatrix4("view", skyboxView);
	skyboxShader.SetMatrix4("projection", projection);

	// Draw skybox cube
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);

	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
}

void Renderer::RenderHands(const std::array<XrPosef, 2>& handPoses, const std::array<XrActionStatePose, 2>& handStates)
{
	glDisable(GL_STENCIL_TEST);

	mainShader.Use();
	for (int i = 0; i < 2; i++)
	{
		if (handStates[i].isActive)
		{
			glm::vec3 handPos = glm::vec3(
				handPoses[i].position.x,
				handPoses[i].position.y,
				handPoses[i].position.z
			);
			glm::quat handRot = glm::quat(
				handPoses[i].orientation.w,
				handPoses[i].orientation.x,
				handPoses[i].orientation.y,
				handPoses[i].orientation.z
			);

			handObjects[i].GetTransform().position = handPos;
			handObjects[i].GetTransform().rotation = handRot;
			handObjects[i].GetTransform().scale = glm::vec3(0.1f);

			glm::mat4 model = handObjects[i].GetTransform().GetMatrix();
			mainShader.SetMatrix4("model", model);
			
			handObjects[i].DrawModel();
		}
	}

	glEnable(GL_STENCIL_TEST);
}

void Renderer::AddObject(const std::filesystem::path& modelPath)
{
	auto newObject = std::make_unique<SceneObject>();
	newObject->model.LoadModel(modelPath, mainShader);  // TODO: might eventually support changing shader
	newObject->transform.position = glm::vec3(0, 0, 0);

	sceneObjects.emplace_back(std::move(newObject));
}

//void Renderer::UpdateGrabbedObject(int objectIndex, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& grabOffset)
//{
//	if (objectIndex >= 0 && objectIndex < sceneObjects.size())
//	{
//		sceneObjects[objectIndex].transform.position = position + grabOffset;
//		sceneObjects[objectIndex].transform.rotation = rotation;
//	}
//}

//void Renderer::UpdatePrimaryObject(glm::vec3 position, glm::quat rotation, glm::vec3 scale)
//{
//	scale.x = scale.x <= 0.0f ? gltfObject.GetTransform().scale.x : scale.x;
//	scale.y = scale.y <= 0.0f ? gltfObject.GetTransform().scale.y : scale.y;
//	scale.z = scale.z <= 0.0f ? gltfObject.GetTransform().scale.z : scale.z;
//
//	gltfObject.GetTransform().position = position;
//	gltfObject.GetTransform().rotation = rotation;
//	gltfObject.GetTransform().scale = scale;
//}
