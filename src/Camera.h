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

	inline const glm::vec3 GetFront() const { return cameraFront; }
	inline const glm::mat4 GetViewMatrix() const { return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); }
	inline const glm::vec3 GetCameraPosition() const { return cameraPos; }

	inline const void RecalculateRightUp()
	{ 
		cameraFront = glm::normalize(glm::cross(cameraFront, worldUp));
		up = glm::normalize(glm::cross(cameraRight, cameraFront));
	}

	// Temp for now, shouldn't be exposed publicly
	glm::vec3 cameraPos;
	glm::vec3 cameraFront;

private:
	glm::vec3 worldUp;
	glm::vec3 up;

	//glm::vec3 cameraPos;
	glm::vec3 cameraTarget;
	glm::vec3 cameraDirection;

	glm::vec3 cameraRight;
	//glm::vec3 cameraFront;
	glm::vec3 cameraUp;

	// TODO: When counter class is updated, remove these and use that class
	float delta = 0.0f;
	float lastFrame = 0.0f;
};

#endif // !CAMERA_H