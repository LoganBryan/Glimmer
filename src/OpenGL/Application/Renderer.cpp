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

void Renderer::Init(float width, float height)
{
	// Init shaders, load models, setup buffers etc
	skyboxShader.Load("shaders/skybox.vert", "shaders/skybox.frag");
	//mainShader.Load("shaders/shader.vert", "shaders/shader.frag");
	geometryShader.Load("shaders/geometry.vert", "shaders/geometry.frag");
	lightingShader.Load("shaders/lighting.vert", "shaders/lighting.frag");
	shadowVolumeShader.Load("shaders/shadowVolume.vert", "shaders/shadowVolume.geom", "shaders/shadowVolume.frag");

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
	AddObject(gltfFile);

	glm::vec3 lastPosition = glm::vec3(0.0f, -0.5f, 0.0f);
	float newX = 0.0f;
	float newZ = 0.0f;
	for (auto& obj : sceneObjects)
	{
		obj->transform.position = lastPosition;
		obj->transform.rotation = glm::quat(1, 0, 0, 0);
		obj->transform.scale = glm::vec3(1.0f);

		newX += 2.0f;
		if (newX >= 6.0f)
		{
			newX = 0.0f;
			newZ -= 2.0f;
		}

		lastPosition = glm::vec3(newX, -0.5f, newZ);
	}

	// Light objects
	glGenBuffers(1, &lightSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
	GLsizei bufferSize = sizeof(GLuint) + 12 + (sizeof(LightData) * maxLights);
	glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightSSBO);

	LightData pointLight = {};
	pointLight.type = 1;
	pointLight.color = glm::vec4(1.0f, 0.0f, 0.0f, 5.0f);
	pointLight.position = glm::vec4(10.0f, 4.0f, 0.0f, 1.0f);
	pointLight.attenuation = glm::vec4(1.0f, 0.007f, 0.0002f, 0.0f);

	LightData spotLight = {};
	spotLight.type = 2;
	spotLight.color = glm::vec4(0.0f, 0.0f, 1.0f, 30.0f);
	spotLight.position = glm::vec4(5.0f, 4.0f, 0.0f, 1.0f);
	spotLight.direction = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
	spotLight.cutOff = glm::vec4(glm::cos(glm::radians(5.0f)), glm::cos(glm::radians(40.0f)), 0.0f, 0.0f);
	spotLight.attenuation = glm::vec4(1.0f, 0.007f, 0.0002f, 0.0f);

	LightData areaLight = {};
	areaLight.type = 3;
	areaLight.color = glm::vec4(0.0f, 1.0f, 0.0f, 10.0f);
	areaLight.position = glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
	areaLight.axisU = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	areaLight.axisV = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

	//lightsWorld.push_back(pointLight);
	//lightsWorld.push_back(spotLight);
	//lightsWorld.push_back(areaLight);

	for (int i = 0; i < 50; i++)
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

	// Setup buffers
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	// Position
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	// Normal 
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	// Albedo + Metallic
	glGenTextures(1, &gAlbedoMetallic);
	glBindTexture(GL_TEXTURE_2D, gAlbedoMetallic);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoMetallic, 0);

	// Roughness + AO
	glGenTextures(1, &gRoughAO);
	glBindTexture(GL_TEXTURE_2D, gRoughAO);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gRoughAO, 0);

	// Emissive
	glGenTextures(1, &gEmissive);
	glBindTexture(GL_TEXTURE_2D, gEmissive);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gEmissive, 0);

	// Draw into attachments
	unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(5, attachments);

	// Create and share depth buffer
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "GBuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

	// Geometry Pass
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 model = glm::mat4(1.0f);
	CameraMatrices camMatrices = Camera::GetInstance()->GetMVP(aspect, nearPlane, farPlane, model);
	glm::mat4 viewMatrix = Camera::GetInstance()->GetViewMatrix();

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

	// gLTF object
	geometryShader.Use();
	glStencilFunc(GL_ALWAYS, 1, 0xFF); // All fragments pass stencil test
	glStencilMask(0xFF); // Enable writing to stencil buffer

	geometryShader.SetMatrix4("projection", camMatrices.projection);
	geometryShader.SetMatrix4("view", camMatrices.view);

	for (auto& obj : sceneObjects)
	{
		glm::mat4 model = obj->transform.GetMatrix();

		obj->model.DrawModel(geometryShader, model);
		obj->model.UpdateSkins(model);
	}

	// Shadow Volume Pass
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Don't need color
	glDepthMask(GL_FALSE);
	glStencilFunc(GL_ALWAYS, 0, 0xFFFF); // Always pass stencil
	// Front face fail = increment. Back face fail = decrement
	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
	glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);

	shadowVolumeShader.Use();

	// Place light far away along sun 
	glm::vec3 lightPosWorld = -sunDir * 10000.0f;
	// Transform into view space
	glm::vec4 lightPosView = viewMatrix * glm::vec4(lightPosWorld, 1.0f);

	glm::mat4 infProj = glm::infinitePerspective(Camera::GetInstance()->GetFOV(), aspect, nearPlane);

	shadowVolumeShader.SetVec4("LightPosition", lightPosView);
	shadowVolumeShader.SetMatrix4("ProjMatrix", infProj);

	shadowVolumeShader.SetMatrix4("projection", camMatrices.projection);
	shadowVolumeShader.SetMatrix4("view", camMatrices.view);

	for (auto& obj : sceneObjects)
	{
		glm::mat4 model = obj->transform.GetMatrix();

		obj->model.DrawShadowVolume(shadowVolumeShader, model);
		//obj->model.UpdateSkins(model);
	}

	// Restore
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	// Lighting Pass
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Transform world space to view space
	lightCount = static_cast<GLuint>(lightsWorld.size());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &lightCount);

	lightsView = lightsWorld;
	for (size_t i = 0; i < lightsView.size(); i++)
	{
		lightsView[i].position = viewMatrix * lightsWorld[i].position;
		lightsView[i].direction = viewMatrix * glm::vec4(glm::vec3(lightsWorld[i].direction), 0.0f);
	}
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 16, sizeof(LightData) * lightCount, lightsView.data());

	lightingShader.Use();

	// Bind GBuffer textures to sampler units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	lightingShader.SetInt("gPosition", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	lightingShader.SetInt("gNormal", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gAlbedoMetallic);
	lightingShader.SetInt("gAlbedoMetallic", 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gRoughAO);
	lightingShader.SetInt("gRoughAO", 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, gEmissive);
	lightingShader.SetInt("gEmissive", 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	lightingShader.SetInt("skybox", 5);

	lightingShader.SetVec3("sunDirection", sunDirView);
	lightingShader.SetFloat("sunIntensity", sunIntensity);
	lightingShader.SetFloat("environmentIntensity", environmentIntensity);

	glDisable(GL_DEPTH_TEST);
	RenderQuad();
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Render anything with forward past here

	//Skybox - Drawn last
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
	newObject->model.LoadModel(modelPath, geometryShader);  // TODO: might eventually support changing shader
	newObject->transform.position = glm::vec3(0, 0, 0);

	sceneObjects.emplace_back(std::move(newObject));
}

void Renderer::RenderQuad()
{
	static GLuint quadVAO = 0, quadVBO = 0;
	if (quadVAO == 0)
	{
		float quadVertices[] =
		{
			-1.0f,  1.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 1.0f, 0.0f,
		};
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
