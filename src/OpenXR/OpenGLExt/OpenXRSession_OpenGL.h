#pragma once

#include "OpenXRInstance_OpenGL.h"
#include "OpenXR/SwapchainManager.h"
#include "OpenGL/Application/Renderer.h"
#include "OpenXR/OpenXRSession.h"

#include "OpenXR/SwapchainManager.h"
#include "OpenXR/OpenGLExt/OpenGLSwapchainTraits.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

using SwapchainManager_OpenGL = SwapchainManager<XrSwapchainImageOpenGLKHR, OpenGLSwapchainTraits>;

class XRSession_OpenGL : public OpenXRSession
{
public:
	XRSession_OpenGL(OpenXRInstance_OpenGL* oXrInstance, HDC hdc, HGLRC glrc, GLFWwindow* window) :
		mXrInstanceManager(oXrInstance), mSwapchainManager(nullptr), m_hdc(hdc), m_glrc(glrc), mRenderer(nullptr), mWindow(window) {}

	inline glm::mat4 XrPoseToGLMMatrix(const XrPosef& pose)
	{
		glm::quat rotation = glm::quat(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
		glm::vec3 position = glm::vec3(pose.position.x, pose.position.y, pose.position.z);
		glm::mat4 orientationMat = glm::mat4_cast(rotation);
		glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), position);
		return glm::inverse(translationMat * orientationMat);
	}

	inline glm::mat4 CreateProjectionMatrix(XrFovf fov, float nearZ, float farZ)
	{
		float left = tanf(fov.angleLeft) * nearZ;
		float right = tanf(fov.angleRight) * nearZ;
		float bottom = tanf(fov.angleDown) * nearZ;
		float top = tanf(fov.angleUp) * nearZ;

		glm::mat4 proj(0.0f);
		proj[0][0] = 2 * nearZ / (right - left);
		proj[2][0] = (right + left) / (right - left);
		proj[1][1] = 2 * nearZ / (top - bottom);
		proj[2][1] = (top + bottom) / (top - bottom);
		proj[2][2] = -(farZ + nearZ) / (farZ - nearZ);
		proj[3][2] = -2 * farZ * nearZ / (farZ - nearZ);
		proj[2][3] = -1.0f;

		return proj;
	}

	inline glm::mat4 GetLeftEyeViewMatrix()
	{
		if (!mLastFrameViews.empty())
		{
			return XrPoseToGLMMatrix(mLastFrameViews[0].pose);
		}
		return glm::mat4(1.0f);
	}

	void CreateSession() override;
	inline void DestroySession() override {
		if (mSession != XR_NULL_HANDLE)
			xrDestroySession(mSession);
	}
	XrSession GetSession() override { return mSession; }
	void PollSystemEvents() override;
	void PollEvents() override;
	void CreateReferenceSpace() override;
	void DestroyReferenceSpace() override;
	bool RenderLayer(RenderLayerInfo& renderlayerInfo) override;
	void RenderDesktopWindow() override;
	void RenderFrame() override;

	inline void SetSwapchainManager(SwapchainManager<XrSwapchainImageOpenGLKHR, OpenGLSwapchainTraits>* swapchainManager) { mSwapchainManager = swapchainManager; }
	inline void SetRenderer(Renderer* renderer) { mRenderer = renderer; }
	inline bool IsApplicationRunning() { return mApplicationRunning; }
	inline bool IsSessionRunning() { return mSessionRunning; }
private:
	HDC m_hdc;
	HGLRC m_glrc;

	OpenXRInstance_OpenGL* mXrInstanceManager;
	SwapchainManager<XrSwapchainImageOpenGLKHR, OpenGLSwapchainTraits>* mSwapchainManager;
	Renderer* mRenderer;
	GLFWwindow* mWindow;

	XrSpace mLocalSpace = XR_NULL_HANDLE;
	XrSession mSession = XR_NULL_HANDLE;

	std::vector<XrView> mLastFrameViews;
	std::mutex mGLMutex;
	std::atomic<bool> mSessionRunning{ false };
	std::atomic<bool> mApplicationRunning{ true };

	XrSessionState mSessionState = XR_SESSION_STATE_UNKNOWN;
};

