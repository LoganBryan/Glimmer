#include "Camera.h"
#include <algorithm>

void Camera::ProcessKeyboard(GLFWwindow* window, float deltaTime)
{
	float velocity = movementSpeed * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		position += front * velocity;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		position -= front * velocity;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		position -= glm::normalize(glm::cross(front, up)) * velocity;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		position += glm::normalize(glm::cross(front, up)) * velocity;
	}

	RecomputeView();
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
}

void Camera::ProcessMouseScroll(float yOffset)
{
}

Camera::Camera()
{
	RecomputeView();
	RecomputeProjection();
}

void Camera::RecomputeView()
{
	glm::vec3 f;
	f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	f.y = sin(glm::radians(pitch));
	f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	front = glm::normalize(f);
	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));

	view = glm::lookAt(position, position + front, up);
}

void Camera::RecomputeProjection()
{
	projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}