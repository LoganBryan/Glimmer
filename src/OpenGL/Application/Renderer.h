#pragma once
#ifndef RENDERER_H
#define RENDERER_H
#define GLM_ENABLE_EXPERIMENTAL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <vector>
#include <string>

#include <variant>

#include "Shader.h"
#include "ComputeShader.h"
#include "FPSCounter.h"

#include <OpenGL/Model/glTF/ResourceCache.h>
#include <OpenGL/Model/glTF/InstanceManager.h>
#include <OpenGL/Model/glTF/SceneRenderer.h>


struct alignas(16) LightData
{
	GLuint type; // x = 0, 1, 2, 3 (Dir, Point, Spot, Area) 
	glm::vec3 _padding; 			
	glm::vec4 _padding_2; 	

	glm::vec4 color; // w - intensity

	glm::vec4 position; // xyz - position (used for point, spot and area) 
	glm::vec4 direction; // xyz - direction (used for dir and spot)

	glm::vec4 cutOff; // Used for spot. x is inner and y is outer

	glm::vec4 attenuation; // x - constant, y - linear, z - quadratic, w - radius
	glm::vec4 axisU; // Area light. xyz edge U
	glm::vec4 axisV; // Area light. xyz edge V
};

struct alignas(16) Cluster
{
	glm::vec4 minPoint;
	glm::vec4 maxPoint;
	unsigned int count;
	unsigned int lightIndices[100];
};

class Renderer
{
public:
	Renderer(GLFWwindow* window);
	~Renderer();

	void Init();
	void Render(float width, float height);

	//void UpdateGrabbedObject(int objectIndex, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& grabOffset);

	// TODO: replace with a function to retrieve and update the closest object
	//void UpdatePrimaryObject(glm::vec3 position, glm::quat rotation, glm::vec3 scale);

	void SetupClusterSSBO();
	void CullLights();

	void TestJointTransformLeftGlove();
private:
	GLFWwindow* window;
	FPSCounter fpsCounter;

	std::shared_ptr<std::vector<MeshGPU>> meshGPU;
	std::vector<GLuint> materialUBOs;
	std::shared_ptr<Skin> skinHandle;

	InstanceManager instanceManager;
	SceneRenderer sceneRenderer;
	std::vector<SceneObject> sceneObjs;

	std::vector<LightData> lightsWorld;
	std::vector<LightData> lightsView;

	Shader skyboxShader;
	unsigned int skyboxTexture;
	unsigned int skyboxVAO, skyboxVBO;

	GLuint lightSSBO;

	GLuint lightCount{ 0 };
	GLuint maxLights{ 1024 };

	Shader mainShader;

	float timeOfDay = 0.1f;

	std::filesystem::path gltfFile;

	const unsigned int localSize = 128;
	const unsigned int gridSizeX = 12;
	const unsigned int gridSizeY = 12;
	const unsigned int gridSizeZ = 24;
	const unsigned int clusterCount = gridSizeX * gridSizeY * gridSizeZ;
	GLuint clusterSSBO;
	ComputeShader clusterShader;
	ComputeShader cullLightShader;
};

#endif // !RENDERER_H