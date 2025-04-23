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

	glDeleteBuffers(1, &lightSSBO);
}

void Renderer::Init()
{
	// Init shaders, load models, setup buffers etc
	skyboxShader.Load("shaders/skybox.vert", "shaders/skybox.frag");
	mainShader.Load("shaders/shader.vert", "shaders/shader.frag");

	SetupClusterSSBO();
	clusterShader.Load("shaders/cluster.comp");
	cullLightShader.Load("shaders/cullLight.comp");

	// Load skybox textures
	skyboxTexture = Utils::GenerateCubemapCompressed(skyboxFaces);

	Viewer viewer;

	auto gltfFile = std::filesystem::path("assets/models/helmet/DamagedHelmet.glb");
	auto leftGlove = std::filesystem::path("assets/models/steamvr_glove/vr_glove_left_model.glb");
	auto rightGlove = std::filesystem::path("assets/models/steamvr_glove/vr_glove_right_model.glb");
	auto duck = std::filesystem::path("assets/models/Duck/duck.glb");
	auto cesiumMan = std::filesystem::path("assets/models/man/CesiumMan.glb");
	auto sponza = std::filesystem::path("assets/models/sponza/Sponza.gltf");

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

	// TODO: Should probably seperate scene setup from renderer, and then extend addobject function (to add a unique id, DisplayName, set transform matrix etc)
	// Loading should also not be handled inside of renderer!
	AddObject(gltfFile);
	AddObject(gltfFile);
	AddObject(gltfFile);
	AddObject(gltfFile);
	AddObject(gltfFile);
	AddObject(gltfFile);
	AddObject(gltfFile);
	AddObject(gltfFile);

	glm::vec3 lastPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	for (auto& obj : sceneObjects)
	{
		obj->transform.position = lastPosition;
		obj->transform.rotation = glm::quat(1, 0, 0, 0);
		obj->transform.scale = glm::vec3(1.0f);

		lastPosition = glm::vec3(lastPosition.x + 2.0f, 0.0f, 0.0f);
	}

	// Light objects
	LightData pointLight = {};
	pointLight.type = 1;
	pointLight.color = glm::vec4(1.0f, 0.0f, 0.0f, 5.0f);
	pointLight.position = glm::vec4(10.0f, 4.0f, 0.0f, 1.0f);
	pointLight.attenuation = glm::vec4(1.0f, 0.007f, 0.0002f, 10.0f);

	LightData spotLight = {};
	spotLight.type = 2;
	spotLight.color = glm::vec4(0.0f, 0.0f, 1.0f, 30.0f);
	spotLight.position = glm::vec4(5.0f, 4.0f, 0.0f, 1.0f);
	spotLight.direction = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
	spotLight.cutOff = glm::vec4(glm::cos(glm::radians(5.0f)), glm::cos(glm::radians(40.0f)), 0.0f, 0.0f);
	spotLight.attenuation = glm::vec4(1.0f, 0.007f, 0.0002f, 5.0f);

	LightData areaLight = {};
	areaLight.type = 3;
	areaLight.color = glm::vec4(0.0f, 1.0f, 0.0f, 10.0f);
	areaLight.position = glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
	areaLight.axisU = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	areaLight.axisV = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
	areaLight.attenuation = glm::vec4(0.0f, 0.0f, 0.0f, 2.0f);

	//lightsWorld.push_back(pointLight);
	//lightsWorld.push_back(spotLight);
	//lightsWorld.push_back(areaLight);

	for (int i = 0; i < 500; i++)
	{
		float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		float g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		float b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		glm::vec4 newCol = glm::vec4(r, g, b, 1.0f);

		float x = -5.0f + static_cast<float>(rand()) / (RAND_MAX / (10.0f));
		float y = 0.0f + static_cast<float>(rand()) / (RAND_MAX / (50.0f));
		float z = -5.0f + static_cast<float>(rand()) / (RAND_MAX / (10.0f));
		glm::vec4 newPosition = glm::vec4(x, y, z, 1.0f);

		LightData manyPointLights = {};
		manyPointLights.type = 1;
		manyPointLights.color = newCol;
		manyPointLights.position = newPosition;
		manyPointLights.attenuation = glm::vec4(1.0f, 0.007f, 0.0002f, 0.0f);

		lightsWorld.emplace_back(manyPointLights);
	}

	glGenBuffers(1, &lightSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
	GLsizei bufferSize = maxLights * sizeof(LightData) + sizeof(GLuint);
	glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightSSBO);

	std::cout << sizeof(LightData) << std::endl;
	std::cout << alignof(LightData) << std::endl;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_STENCIL_TEST);

	glDisable(GL_BLEND);
}

void Renderer::Render(float width, float height)
{
	fpsCounter.Update();

	glClearColor(0.25f, 0.25f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	const float aspect = static_cast<float>(width) / static_cast<float>(height);
	const float nearPlane = 0.1f;
	const float farPlane = 100.0f;

	glm::mat4 model = glm::mat4(1.0f);
	CameraMatrices camMatrices = Camera::Get().GetCameraMatrix(model);
	glm::mat4 viewMatrix = Camera::Get().GetView();
	glm::mat4 projMatrix = Camera::Get().GetProjection();

	Camera::Get().SetViewport(width, height);
	Camera::Get().SetClippingPlanes(nearPlane, farPlane);

	// Lights
	// Transform world space to view space
	lightCount = static_cast<GLuint>(lightsWorld.size());

	lightsView = lightsWorld;
	for (size_t i = 0; i < lightsView.size(); i++)
	{
		lightsView[i].position = viewMatrix * lightsWorld[i].position;
		lightsView[i].direction = viewMatrix * glm::vec4(glm::vec3(lightsWorld[i].direction), 0.0f);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(LightData) * lightCount, lightsView.data());
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, maxLights * sizeof(LightData), sizeof(GLuint), &lightCount);

	//clusterShader.Use();
	//clusterShader.SetFloat("zNear", nearPlane);
	//clusterShader.SetFloat("zFar", farPlane);
	//clusterShader.SetVec3("gridSize", gridSizeX, gridSizeY, gridSizeZ);
	//clusterShader.SetVec2("screenDim", (GLuint)width, (GLuint)height);
	//glm::mat4 inverseProj = glm::inverse(projMatrix);
	//clusterShader.SetMatrix4("inverseProj", inverseProj);
	//glDispatchCompute(gridSizeX, gridSizeY, gridSizeZ);

	//cullLightShader.Use();
	//cullLightShader.SetMatrix4("viewMatrix", viewMatrix);
	//GLuint groups = (clusterCount + localSize - 1) / localSize;
	//glDispatchCompute(groups, 1, 1);

	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	CullLights();

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, clusterSSBO);

	// gLTF object
	mainShader.Use();
	glStencilFunc(GL_ALWAYS, 1, 0xFF); // All fragments pass stencil test
	glStencilMask(0xFF); // Enable writing to stencil buffer

	// Update sun
	const float dT = 0.016f;
	timeOfDay += dT * 0.01;

	timeOfDay = glm::fract(timeOfDay);

	//timeOfDay = 0.5f;

	float sunElevation = (timeOfDay * 2.0f - 0.5f) * glm::pi<float>();
	float sunHeight = glm::sin(sunElevation);

	glm::vec3 sunDir = glm::vec3(0.0f, glm::sin(sunElevation), glm::cos(sunElevation));
	glm::vec3 sunDirView = glm::mat3(viewMatrix) * sunDir;

	float sunIntensity = glm::smoothstep(-0.5f, 0.5f, sunHeight);
	sunIntensity = glm::clamp(sunIntensity, 0.0f, 1.0f);

	float environmentIntensity = glm::smoothstep(-0.866f, 0.866f, sunHeight);
	environmentIntensity = glm::mix(0.02f, 1.0f, environmentIntensity);

	mainShader.SetFloat("zNear", nearPlane);
	mainShader.SetFloat("zFar", farPlane);
	mainShader.SetVec3("gridSize", gridSizeX, gridSizeY, gridSizeZ);
	mainShader.SetVec2("screenDim", (GLuint)width, (GLuint)height);

	mainShader.SetVec3("sunDirection", sunDirView);
	mainShader.SetFloat("sunIntensity", sunIntensity);
	mainShader.SetFloat("environmentIntensity", environmentIntensity);

	mainShader.SetMatrix4("projection", camMatrices.projection);
	mainShader.SetMatrix4("view", camMatrices.view);

	for (auto& obj : sceneObjects)
	{
		glm::mat4 model = obj->transform.GetMatrix();

		obj->model.DrawModel(mainShader, model);
		obj->model.UpdateSkins(model);
	}

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
	ImGui::Text("FPS: %.2f", fpsCounter.GetFPS());
	ImGui::Text("Frame Time: %.4f ms", fpsCounter.GetFrameTime() * 1000.0f);
	gui->EndFrame();

	gui->Render();
}

void Renderer::AddObject(const std::filesystem::path& modelPath)
{
	auto newObject = std::make_unique<SceneObject>();
	newObject->model.LoadModel(modelPath, mainShader);  // TODO: might eventually support changing shader
	newObject->transform.position = glm::vec3(0, 0, 0);

	sceneObjects.emplace_back(std::move(newObject));
}

void Renderer::SetupClusterSSBO()
{
	glGenBuffers(1, &clusterSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, clusterSSBO);

	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Cluster) * clusterCount, nullptr, GL_STATIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, clusterSSBO);
}

void Renderer::CullLights()
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	std::pair<float, float> clipping = Camera::Get().GetClippingPlanes();
	glm::mat4 proj = Camera::Get().GetProjection();
	glm::mat4 view = Camera::Get().GetView();
	glm::mat4 inverseProj = glm::inverse(proj);

	// Build AABBs
	clusterShader.Use();
	clusterShader.SetFloat("zNear", clipping.first);
	clusterShader.SetFloat("zFar", clipping.second);
	clusterShader.SetMatrix4("inverseProj", inverseProj);
	clusterShader.SetVec3("gridSize", gridSizeX, gridSizeY, gridSizeZ);
	clusterShader.SetVec2("screenDim", width, height);

	clusterShader.Dispatch(gridSizeX, gridSizeY, gridSizeZ);

	// Cull Lights
	cullLightShader.Use();
	cullLightShader.SetMatrix4("viewMatrix", view);

	GLuint groups = (clusterCount + localSize - 1) / localSize;
	cullLightShader.Dispatch(groups, 1, 1);
}