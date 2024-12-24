#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>

// This class is pretty rough - will fix B1
class Camera
{
public:
	Camera(glm::vec3 initialPosition);
	glm::mat4 Update();

	void HandleCameraInput(GLFWwindow* window);

	inline const void SetFront(glm::vec3 newFront) { cameraFront = newFront; }
private:
	glm::vec3 up{ 0.0f, 1.0f, 0.0f };

	glm::vec3 cameraPos;
	glm::vec3 cameraTarget;
	glm::vec3 cameraDirection;

	glm::vec3 cameraRight;
	glm::vec3 cameraFront;
	glm::vec3 cameraUp;

	// TODO: When counter class is updated, remove these and use that class
	float delta = 0.0f;
	float lastFrame = 0.0f;
};

#endif // !CAMERA_H