#pragma once
#ifndef CAMERA_H
#define CAMERA_H

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
	static Camera& Get()
	{
		static Camera instance;
		return instance;
	}

	Camera(const Camera&) = delete;
	Camera(Camera&&) = delete;
	Camera& operator=(const Camera&) = delete;
	Camera& operator=(Camera&&) = delete;

	void SetViewport(float width, float height)
	{
		aspectRatio = (width > 0 && height > 0) ? (width / height) : 1.0f;
		RecomputeProjection();
	}

	void SetClippingPlanes(float nearPlane, float farPlane)
	{
		this->nearPlane = nearPlane;
		this->farPlane = farPlane;
		RecomputeProjection();
	}

	std::pair<float, float> GetClippingPlanes() { return std::make_pair(nearPlane, farPlane); }

	void SetFOV(float degrees)
	{
		fov = degrees;
		RecomputeProjection();
	}

	void SetPosition(const glm::vec3& pos) 
	{ 
		position = pos; 
		RecomputeView(); 
	}
	void SetOrientation(float yaw, float pitch)
	{
		this->yaw = yaw;
		this->pitch = pitch;
		RecomputeView();
	}

	void ProcessKeyboard(GLFWwindow* window, float deltaTime);
	void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
	void ProcessMouseScroll(float yOffset);

	const glm::mat4& GetProjection() const { return projection; }
	const glm::mat4& GetView() const { return view; }
	glm::mat4 GetMVP(const glm::mat4& model) const { return projection * view * model; }
	CameraMatrices GetCameraMatrix(const glm::mat4& model)
	{
		CameraMatrices matrices;

		matrices.projection = GetProjection();
		matrices.view = GetView();
		matrices.viewNormal = glm::mat4(glm::mat3(GetView()));
		matrices.model = model;
		matrices.mvp = matrices.projection * matrices.view * matrices.model;

		return matrices;
	}

private:
	Camera();
	~Camera() = default;

	void RecomputeView();
	void RecomputeProjection();

	glm::vec3 position{ 0.0f, 0.0f, 3.0f };
	glm::vec3 front{ 0.0f, 0.0f, -1.0f };
	glm::vec3 up{ 0.0f, 1.0f, 0.0f };
	glm::vec3 right{ 1.0f, 0.0f, 0.0f };
	glm::vec3 worldUp{ 0.0f, 1.0f, 0.0f };

	float yaw = -90.0f;
	float pitch = 0.0f;
	float fov = 90.0f;
	float aspectRatio = 4.0f / 3.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

	float movementSpeed = 2.5f;
	float mouseSensitivity = 0.1f;

	glm::mat4 view{ 1.0f };
	glm::mat4 projection{ 1.0f };
};
#endif // !CAMERA_H
