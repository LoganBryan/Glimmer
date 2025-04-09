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
#include <openxr/openxr.h>

class Renderer
{
public:
	Renderer(GLFWwindow* window);
	~Renderer();

	void Init();
	void Render(float width, float height);
	void RenderEye(glm::mat4& view, glm::mat4& projection, GLuint framebuffer, int width, int height);
	void RenderHands(const std::array<XrPosef, 2>& handPoses, const std::array<XrActionStatePose, 2>& handStates);

	// TODO: replace with a function to retrieve and update the closest object
	void UpdatePrimaryObject(glm::vec3 position, glm::quat rotation, glm::vec3 scale);

private:
	GLFWwindow* window;

	Shader skyboxShader;
	unsigned int skyboxTexture;
	unsigned int skyboxVAO, skyboxVBO;

	Shader mainShader;
	GltfLoader gltfObject;
	GltfLoader handObjects[2];
};

#endif // !RENDERER_H