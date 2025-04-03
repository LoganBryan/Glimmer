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
#include "GltfLoader.h"

class Renderer
{
public:
	Renderer(GLFWwindow* window);
	~Renderer();

	void Init();
	void Render(float width, float height);

private:
	GLFWwindow* window;

	Shader skyboxShader;
	unsigned int skyboxTexture;
	unsigned int skyboxVAO, skyboxVBO;

	Shader mainShader;
	GltfLoader gltfObject;
};

#endif // !RENDERER_H