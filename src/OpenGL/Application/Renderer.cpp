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

	gltfFile = std::filesystem::path("assets/models/helmet/DamagedHelmet.gltf");
	auto leftGlove = std::filesystem::path("assets/models/steamvr_glove/vr_glove_left_model.glb");
	auto flight = std::filesystem::path("assets/models/flightHelm/flight.glb");
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

	// TODO: this should be in it's own function, these are only called once on creation then cached for later
	auto& cache = ResourceCache::Get();

	std::shared_ptr<fastgltf::Asset> assetPtr = nullptr;
	size_t firstPrimMatIndex = 0;
	try
	{
		cache.GetOrLoadMeshes(gltfFile);
		cache.LoadMaterialsFromAsset(gltfFile);
		cache.GetOrLoadSkin(gltfFile);

		assetPtr = cache.GetParsedAsset(gltfFile);
		if (assetPtr && !assetPtr->meshes.empty() && !assetPtr->meshes[0].primitives.empty())
		{
			firstPrimMatIndex = assetPtr->meshes[0].primitives[0].materialIndex.value_or(0);
		}
		else
		{
			std::cerr << "Warning! Could not determine material index from glTF: " << gltfFile << ". Using default!" << std::endl;
			firstPrimMatIndex = 0;
		}
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << "Error pre loading resources for " << gltfFile << "\n what: " << e.what() << std::endl;
		return;
	}

	std::shared_ptr<fastgltf::Asset> assetPtrGlove = nullptr;
	size_t firstPrimMatIndexGlove = 0;
	try
	{
		cache.GetOrLoadMeshes(leftGlove);
		cache.LoadMaterialsFromAsset(leftGlove);
		cache.GetOrLoadSkin(leftGlove);

		assetPtrGlove = cache.GetParsedAsset(leftGlove);
		if (assetPtrGlove && !assetPtrGlove->meshes.empty() && !assetPtrGlove->meshes[0].primitives.empty())
		{
			firstPrimMatIndexGlove = assetPtrGlove->meshes[0].primitives[0].materialIndex.value_or(0);
		}
		else
		{
			std::cerr << "Warning! Could not determine material index from glTF: " << leftGlove << ". Using default!" << std::endl;
			firstPrimMatIndexGlove = 0;
		}
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << "Error pre loading resources for " << leftGlove << "\n what: " << e.what() << std::endl;
		return;
	}

	glm::mat4 nodeTransform = glm::mat4(1.0f);

	if (!assetPtr->nodes.empty())
	{
		auto& node = assetPtr->nodes[0];

		nodeTransform = std::visit([](auto&& arg) -> glm::mat4
			{
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<T, fastgltf::math::fmat4x4>)
				{
					return glm::make_mat4(arg.data());
				}
				else if constexpr (std::is_same_v<T, fastgltf::TRS>)
				{
					glm::mat4 transMat = glm::translate(glm::mat4(1.0f), glm::make_vec3(arg.translation.data()));
					glm::quat rotQuat = glm::make_quat(arg.rotation.data());
					glm::mat4 rotMat = glm::mat4_cast(rotQuat);
					glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::make_vec3(arg.scale.data()));

					return transMat * rotMat * scaleMat;
				}
				else
				{
					return glm::mat4(1.0f);
				}
			}, node.transform);
	}

	glm::vec3 start = glm::vec3(0.0f);

	for (int i = 0; i < 100; i++)
	{
		SceneObject sceneObj;
		sceneObj.meshPath = gltfFile.string();

		glm::mat4 instTrans = glm::translate(glm::mat4(1.0f), start);
		glm::mat4 instRot = glm::mat4_cast(glm::quat(1, 0, 0, 0));
		glm::mat4 instScale = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
		glm::mat4 instMat = instTrans * instRot * instScale;

		glm::mat4 finalMatrix = instMat * nodeTransform;

		glm::vec3 decompScale;
		glm::quat decompRot;
		glm::vec3 decompPos;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(finalMatrix, decompScale, decompRot, decompPos, skew, perspective);

		sceneObj.transform.position = decompPos;
		sceneObj.transform.rotation = decompRot;
		sceneObj.transform.scale = decompScale;

		sceneObj.gltfMaterialIndex = firstPrimMatIndex;

		sceneObjs.push_back(sceneObj);

		start += glm::vec3(2.0f, 0.0f, 0.0f);
	}

	//std::sort(sceneObjs.begin(), sceneObjs.end(), [](const SceneObject& a, const SceneObject& b)
	//	{
	//		return a.meshPath < b.meshPath;
	//	});

	for (int i = 0; i < 2; i++)
	{
		SceneObject sceneObj;
		sceneObj.meshPath = leftGlove.string();
		sceneObj.transform.position = start;
		sceneObj.transform.rotation = glm::quat(1, 0, 0, 0);
		sceneObj.transform.scale = glm::vec3(1.0f);

		sceneObj.gltfMaterialIndex = firstPrimMatIndexGlove;

		sceneObjs.push_back(sceneObj);

		start += glm::vec3(2.0f, 0.0f, 0.0f);
	}

	std::sort(sceneObjs.begin(), sceneObjs.end(), [](const SceneObject& a, const SceneObject& b)
		{
			return a.meshPath < b.meshPath;
		});

	instanceManager.SetSceneObjects(sceneObjs);


	mainShader.Use();

	viewer.uvOffsetUniform = glGetUniformLocation(mainShader.GetID(), "uvOffset");
	viewer.uvScaleUniform = glGetUniformLocation(mainShader.GetID(), "uvScale");
	viewer.uvRotationUniform = glGetUniformLocation(mainShader.GetID(), "uvRotation");

	glUniform2f(viewer.uvOffsetUniform, 0, 0);
	glUniform2f(viewer.uvScaleUniform, 1.0f, 1.0f);
	glUniform1f(viewer.uvRotationUniform, 0);

	uint64_t placeholderHandle = cache.texManager.GetPlaceholderHandle();
	glm::uvec2 placeholder_uvec2;
	placeholder_uvec2.x = static_cast<uint32_t>(placeholderHandle & 0xFFFFFFFFULL);
	placeholder_uvec2.y = static_cast<uint32_t>(placeholderHandle >> 32);
	glUniform2ui(glGetUniformLocation(mainShader.GetID(), "u_placeholderTextureHandle"), placeholder_uvec2.x, placeholder_uvec2.y);

	//for (int i = 0; i < 500; i++)
	//{
	//	float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	//	float g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	//	float b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	//	glm::vec4 newCol = glm::vec4(r, g, b, 1.0f);

	//	float x = -5.0f + static_cast<float>(rand()) / (RAND_MAX / (10.0f));
	//	float y = 0.0f + static_cast<float>(rand()) / (RAND_MAX / (50.0f));
	//	float z = -5.0f + static_cast<float>(rand()) / (RAND_MAX / (10.0f));
	//	glm::vec4 newPosition = glm::vec4(x, y, z, 1.0f);

	//	LightData manyPointLights = {};
	//	manyPointLights.type = 1;
	//	manyPointLights.color = newCol;
	//	manyPointLights.position = newPosition;
	//	manyPointLights.attenuation = glm::vec4(1.0f, 0.007f, 0.0002f, 0.0f);

	//	lightsWorld.emplace_back(manyPointLights);
	//}

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
	mainShader.SetFloat("environmentIntensity", 0.0);

	mainShader.SetMatrix4("projection", camMatrices.projection);
	mainShader.SetMatrix4("view", camMatrices.view);

	auto& cache = ResourceCache::Get();
	cache.skinManager.Upload(model);
	sceneRenderer.Draw(mainShader, instanceManager, cache, cache.matManager, cache.skinManager, skyboxTexture);

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

void Renderer::TestJointTransformLeftGlove()
{
	auto& cache = ResourceCache::Get();

	glm::mat4 indexMatrix = glm::mat4(1.0f);
	indexMatrix = glm::rotate(indexMatrix, glm::radians(35.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::mat4 middleMatrix = glm::mat4(1.0f);
	middleMatrix = glm::rotate(middleMatrix, glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::mat4 ringMatrix = glm::mat4(1.0f);
	ringMatrix = glm::rotate(ringMatrix, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	cache.skinManager.UpdateJointTransform("finger_index_meta_l", indexMatrix);
	cache.skinManager.UpdateJointTransform("finger_index_0_l", indexMatrix);
	cache.skinManager.UpdateJointTransform("finger_index_1_l", indexMatrix);
	cache.skinManager.UpdateJointTransform("finger_index_2_l", indexMatrix);
	cache.skinManager.UpdateJointTransform("finger_index_l_end", indexMatrix);

	cache.skinManager.UpdateJointTransform("finger_middle_meta_l", middleMatrix);
	cache.skinManager.UpdateJointTransform("finger_middle_0_l", middleMatrix);
	cache.skinManager.UpdateJointTransform("finger_middle_1_l", middleMatrix);
	cache.skinManager.UpdateJointTransform("finger_middle_2_l", middleMatrix);
	cache.skinManager.UpdateJointTransform("finger_middle_l_end", middleMatrix);

	cache.skinManager.UpdateJointTransform("finger_ring_meta_l", ringMatrix);
	cache.skinManager.UpdateJointTransform("finger_ring_0_l", ringMatrix);
	cache.skinManager.UpdateJointTransform("finger_ring_1_l", ringMatrix);
	cache.skinManager.UpdateJointTransform("finger_ring_2_l", ringMatrix);
	cache.skinManager.UpdateJointTransform("finger_ring_l_end", ringMatrix);
}
