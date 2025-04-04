#include "Camera.h"


Camera* instance = nullptr;
std::mutex mtx;

CameraMatrices Camera::GetMVP(float aspect, float nearPlane, float farPlane, const glm::mat4& model) const
{
	CameraMatrices matrices;

	matrices.projection = glm::perspective(glm::radians(zoom), aspect, nearPlane, farPlane);
	matrices.view = GetViewMatrix();
	matrices.viewNormal = glm::mat4(glm::mat3(GetViewMatrix()));
	matrices.model = model;
	matrices.mvp = matrices.projection * matrices.view * matrices.model;

	return matrices;
}

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
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
	xOffset *= mouseSensitivity;
	yOffset *= mouseSensitivity;

	yaw += xOffset;
	pitch += yOffset;

	if (constrainPitch)
	{
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}

	UpdateCameraVectors();
}

Camera::Camera() : position(0.0f, 0.0f, 3.0f), front(0.0f, 0.0f, -1.0f), up(0.0f, 1.0f, 0.0f), right(0.0f, 0.0f, 0.0f), worldUp(0.0f, 1.0f, 0.0f), yaw(-90.0f), pitch(0.0f), movementSpeed(2.5f), mouseSensitivity(0.1f), zoom(90.0f)
{
	cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	
	cameraDirection = glm::normalize(position - cameraTarget);
	right = glm::normalize(glm::cross(up, cameraDirection));
	up = glm::cross(cameraDirection, right);
}

void Camera::UpdateCameraVectors()
{
	glm::vec3 newFront;

	newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	newFront.y = sin(glm::radians(pitch));
	newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(newFront);

	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));
}

