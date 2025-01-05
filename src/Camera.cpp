#include "Camera.h"

Camera::Camera(glm::vec3 initialPosition): cameraPos(initialPosition)
{
	cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	up = glm::vec3(0.0f, 1.0f, 0.0f);
	worldUp = up;

	cameraDirection = glm::normalize(cameraPos - cameraTarget);
	cameraRight = glm::normalize(glm::cross(up, cameraDirection));
	cameraUp = glm::cross(cameraDirection, cameraRight);

}

glm::mat4 Camera::Update()
{
	double time = glfwGetTime();

	const float radi = 5.0f;
	float cameraX = sin(time) * radi;
	float cameraZ = cos(time) * radi;

	float currentFrame = time;
	delta = currentFrame - lastFrame;
	lastFrame = currentFrame;

	//return glm::lookAt(glm::vec3(cameraX, 0.0f, cameraZ), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void Camera::HandleCameraInput(GLFWwindow* window)
{
	const float cameraSpeed = 2.5f * delta;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		cameraPos += cameraSpeed * cameraFront;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		cameraPos -= cameraSpeed * cameraFront;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}

}
