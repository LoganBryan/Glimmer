#pragma once

#include <mutex>
#include <atomic>
#include <future>
#include <string>

class OpenXRSession
{
public:
	struct RenderLayerInfo
	{
		XrTime predictedDisplayTime;
		std::vector<XrCompositionLayerBaseHeader*> layers;
		XrCompositionLayerProjection layerProjection = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
		std::vector<XrCompositionLayerProjectionView> layerProjectionViews;
	};

public:
	// Interactions
	virtual void CreateActionSet(std::string actionSetName, std::string readableName, int priority) = 0;
	virtual void SuggestBindings() = 0;
	virtual void RecordCurrentBindings() = 0;

	virtual void CreateActionPoses() = 0;
	virtual void AttachActionSet() = 0;

	virtual void PollActions(XrTime predictedTime) = 0;
	virtual void ObjectInteraction() = 0;

public:
	virtual void CreateSession() = 0;
	inline virtual void DestroySession()
	{
		if (mSession != XR_NULL_HANDLE)
			xrDestroySession(mSession);
	}
	virtual XrSession GetSession() = 0;
	virtual void PollSystemEvents() = 0;
	virtual void PollEvents() = 0;
	inline virtual void CreateReferenceSpace()
	{
		// Create reference XrSpace, specifying a local space with identity pose for origin.
		XrReferenceSpaceCreateInfo referenceSpaceCI{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
		referenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
		referenceSpaceCI.poseInReferenceSpace = { {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} };
		xrCreateReferenceSpace(mSession, &referenceSpaceCI, &mLocalSpace);
	}
	inline virtual void DestroyReferenceSpace()
	{
		xrDestroySpace(mLocalSpace);
	}
	virtual bool RenderLayer(RenderLayerInfo& renderlayerInfo) = 0;
	virtual void RenderDesktopWindow() = 0;
	virtual void RenderFrame() = 0;

	inline virtual bool IsApplicationRunning() { return mApplicationRunning; }
	inline virtual bool IsSessionRunning() { return mSessionRunning; }
protected:
	XrSpace mLocalSpace = XR_NULL_HANDLE;
	XrSession mSession = XR_NULL_HANDLE;

	std::vector<XrView> mLastFrameViews;
	std::mutex mGLMutex;
	std::atomic<bool> mSessionRunning{ false };
	std::atomic<bool> mApplicationRunning{ true };

	XrSessionState mSessionState = XR_SESSION_STATE_UNKNOWN;

protected:
	// Interactions
	XrActionSet mActionSet;
	XrAction mGrabAction;

	XrActionStateFloat mGrabState[2] = { {XR_TYPE_ACTION_STATE_FLOAT}, {XR_TYPE_ACTION_STATE_FLOAT} };
	int mGrabbedObject[2] = {-1, -1};

	XrAction mBuzzAction; // Haptic output
	float mBuzz[2] = { 0, 0 }; // Current haptic output for each controller
	bool mGrabHapticTriggered[2] = { false, false };

	XrAction mPalmPoseAction; // Hand/ Controller pos and orientation
	XrPath mHandPaths[2] = { 0,0 };

	XrSpace mHandPoseSpace[2];
	XrActionStatePose mHandPoseState[2] = { {XR_TYPE_ACTION_STATE_POSE}, {XR_TYPE_ACTION_STATE_POSE} };

	float mViewHeight = 1.5f;
	XrPosef mHandPose[2] =
	{
		{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -mViewHeight}},
		{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -mViewHeight}} };
};
