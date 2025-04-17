#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <vector>
#include <string>

#include "Shader.h"
#include "OpenGL/Model/GltfLoader.h"

struct SceneObject
{
	GltfLoader model;
	Transform transform;
	glm::vec3 grabOffset;
	glm::quat grabRotationOffset;
	bool isGrabbed = false;
	int grabbedByHand = -1; // -1 none, 0 left, 1 right
};

struct alignas(16) LightData
{
	GLuint type; // x = 0, 1, 2, 3 (Dir, Point, Spot, Area)  // 4 bytes, offset 16
	GLuint _padding_1[3];									 // 12 bytes, offset 4
	glm::vec3 _padding_2;									 // 12 bytes, offset 32
	float _padding_3;									 // 12 bytes, offset 28

	glm::vec4 color; // w - intensity

	glm::vec4 position; // xyz - position (used for point, spot and area) 
	glm::vec4 direction; // xyz - direction (used for dir and spot)

	glm::vec4 cutOff; // Used for spot. x is inner and y is outer

	glm::vec4 attenuation; // x - constant, y - linear, z - quadratic
	glm::vec4 axisU; // Area light. xyz edge U
	glm::vec4 axisV; // Area light. xyz edge V
};

class Renderer
{
public:
	Renderer(GLFWwindow* window);
	~Renderer();

	void Init();
	void Render(float width, float height);

	void AddObject(const std::filesystem::path& modelPath);
	//void UpdateGrabbedObject(int objectIndex, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& grabOffset);

	// TODO: replace with a function to retrieve and update the closest object
	//void UpdatePrimaryObject(glm::vec3 position, glm::quat rotation, glm::vec3 scale);

private:
	GLFWwindow* window;

	std::vector<std::unique_ptr<SceneObject>> sceneObjects; // TODO: should update this to have multiple containers, grabbable objects, scene objects and player objects (hands, body etc)
	std::vector<LightData> lightsWorld;
	std::vector<LightData> lightsView;

	Shader skyboxShader;
	unsigned int skyboxTexture;
	unsigned int skyboxVAO, skyboxVBO;

	GLuint lightSSBO;

	GLuint maxLights{ 100 };
	GLuint lightCount{ 0 };

	Shader mainShader;

	float timeOfDay = 0.1f;
};

#endif // !RENDERER_H