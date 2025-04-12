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
	void RenderEye(glm::mat4& view, glm::mat4& projection, GLuint framebuffer, int width, int height);
	void RenderHands(const std::array<XrPosef, 2>& handPoses, const std::array<XrActionStatePose, 2>& handStates);

	void AddObject(const std::filesystem::path& modelPath);
	//void UpdateGrabbedObject(int objectIndex, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& grabOffset);

	// TODO: replace with a function to retrieve and update the closest object
	//void UpdatePrimaryObject(glm::vec3 position, glm::quat rotation, glm::vec3 scale);

	inline std::vector<std::unique_ptr<SceneObject>>& GetObjects() { return sceneObjects; }

    // TODO: This is just temp for visualizing
    struct LineVertex {
        XrVector3f position;
        XrVector3f color;
    };

    inline void RenderLine(const XrVector3f& start, const XrVector3f& end, const XrVector3f& color) {

		lineShader.Use();
		glDisable(GL_DEPTH_TEST);

        LineVertex vertices[2] = {
            {start, color},
            {end, color}
        };

        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, color));
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_LINES, 0, 2);

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);

		glEnable(GL_DEPTH_TEST);
		glBindVertexArray(0);
    }

private:
	GLFWwindow* window;

	std::vector<std::unique_ptr<SceneObject>> sceneObjects; // TODO: should update this to have multiple containers, grabbable objects, scene objects and player objects (hands, body etc)
	GLuint simpleObjectShader;

	Shader skyboxShader;
	unsigned int skyboxTexture;
	unsigned int skyboxVAO, skyboxVBO;

	Shader mainShader;
	//GltfLoader gltfObject;
	GltfLoader handObjects[2];

	Shader lineShader;
};

#endif // !RENDERER_H