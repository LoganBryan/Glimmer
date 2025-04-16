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

	Shader skyboxShader;
	unsigned int skyboxTexture;
	unsigned int skyboxVAO, skyboxVBO;

	Shader mainShader;
};

#endif // !RENDERER_H