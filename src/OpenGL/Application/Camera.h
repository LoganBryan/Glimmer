#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <mutex>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>



struct CameraMatrices
{
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model;
	glm::mat4 mvp;
	glm::mat4 viewNormal;
};


class Camera
{
public:
	// Don't allow cloning
	Camera(Camera& other) = delete;

	// Don't allow assigning
	void operator=(const Camera&) = delete;

	static inline Camera* GetInstance()
	{
		std::lock_guard<std::mutex> lock(mtx);
		if (instance == nullptr)
		{
			instance = new Camera();
		}
		return instance;
	}

	inline glm::mat4 GetViewMatrix() const { return glm::lookAt(position, position + front, up); }
	CameraMatrices GetMVP(float aspect, float nearPlane, float farPlane, const glm::mat4& model = glm::mat4(1.0f)) const;

	inline const void SetFront(glm::vec3 newFront) { front = newFront; }
	inline const glm::vec3& GetPosition() const { return position; }
	inline const glm::vec3& GetFront() const { return front; }
	inline const glm::vec3& GetUp() const { return up; }
	
	void ProcessKeyboard(GLFWwindow* window, float deltaTime);
	void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

	inline void SetMovementSpeed(float speed) { movementSpeed = speed; }
	inline float GetMovementSpeed() const { return movementSpeed; }

	inline void SetMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
	inline float GetMouseSensitivity() const { return mouseSensitivity; }

protected:
	Camera();
	inline ~Camera() {}

private:
	static inline Camera* instance;
	static inline std::mutex mtx;

	void UpdateCameraVectors();

	// Camera Attribs
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;

	glm::vec3 cameraTarget;
	glm::vec3 cameraDirection;

	float yaw;
	float pitch;

	float movementSpeed;
	float mouseSensitivity;
	float zoom;
};


#endif // !CAMERA_H
