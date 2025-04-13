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
	// Interactions
	void CreateActionSet(std::string actionSetName, std::string readableName, int priority) override;
	void SuggestBindings() override;
	void RecordCurrentBindings() override;

	void CreateActionPoses() override;
	void AttachActionSet() override;

	void PollActions(XrTime predictedTime) override;

	glm::vec3 GetHandPosition(int handIndex) const;
	glm::quat GetHandRotation(int handIndex) const;
	void ObjectInteraction() override;
	void CreateHandTrackers() override 
	{
		for (int i = 0; i < 2; i++)
		{
			Hand& hand = mHands[i];
			XrHandTrackerCreateInfoEXT xrHandTrackerCreateInfo = { XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
			xrHandTrackerCreateInfo.hand = i == 0 ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
			xrHandTrackerCreateInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;

			mXrInstanceManager->xrCreateHandTrackerEXT(mSession, &xrHandTrackerCreateInfo, &hand.mHandTracker);
		}
	}
	void TrackHands(XrTime predictedTime) override;
public:
	XRSession_OpenGL(OpenXRInstance_OpenGL* oXrInstance, HDC hdc, HGLRC glrc, GLFWwindow* window) :
		mXrInstanceManager(oXrInstance), mSwapchainManager(nullptr), m_hdc(hdc), m_glrc(glrc), mRenderer(nullptr), mWindow(window) {}

	// This is mainly used for the camera
	inline glm::mat4 XrPoseToGLMMatrix(const XrPosef& pose)
	{
		glm::quat rotation = glm::quat(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
		glm::vec3 position = glm::vec3(pose.position.x, pose.position.y, pose.position.z);
		glm::mat4 orientationMat = glm::mat4_cast(rotation);
		glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), position);
		return glm::inverse(translationMat * orientationMat);
	}

	// Non-inversed pose to glm used for bone transforms
	inline glm::mat4 XrPoseToGLMMatrixBone(const XrPosef& pose)
	{
		glm::quat rotation = glm::quat(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
		glm::vec3 position = glm::vec3(pose.position.x, pose.position.y, pose.position.z);
		glm::mat4 orientationMat = glm::mat4_cast(rotation);
		glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), position);

		return translationMat * orientationMat;
	}

	inline std::array<XrPosef, 2> GetHandTrackingWristPoses()
	{
		std::array<XrPosef, 2> wristPoses{};
		for (int i = 0; i < 2; i++)
		{
			if (mXrInstanceManager->handTrackingSystemProperties.supportsHandTracking)
				wristPoses[i] = mHands[i].mJointLocations[XR_HAND_JOINT_WRIST_EXT].pose;
			else
				wristPoses[i] = { {0,0,0,1}, {0,0,0} };
		}

		return wristPoses;
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
	XrSession GetSession() override { return mSession; }
	void PollSystemEvents() override;
	void PollEvents() override;
	bool RenderLayer(RenderLayerInfo& renderlayerInfo) override;
	void RenderDesktopWindow() override;
	void RenderFrame() override;

	inline void SetSwapchainManager(SwapchainManager<XrSwapchainImageOpenGLKHR, OpenGLSwapchainTraits>* swapchainManager) { mSwapchainManager = swapchainManager; }
	inline void SetRenderer(Renderer* renderer) { mRenderer = renderer; }

private:
	HDC m_hdc;
	HGLRC m_glrc;

	OpenXRInstance_OpenGL* mXrInstanceManager;
	SwapchainManager<XrSwapchainImageOpenGLKHR, OpenGLSwapchainTraits>* mSwapchainManager;
	Renderer* mRenderer;
	GLFWwindow* mWindow;
};

