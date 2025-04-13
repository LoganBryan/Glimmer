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

	struct Hand
	{
		XrHandJointLocationEXT mJointLocations[XR_HAND_JOINT_COUNT_EXT];
		XrHandTrackerEXT mHandTracker = 0;
	};

	const std::array<int, XR_HAND_JOINT_COUNT_EXT> HAND_JOINT_PARENT_INDICES =
	{
		XR_HAND_JOINT_WRIST_EXT,
		-1,

		XR_HAND_JOINT_WRIST_EXT,
		XR_HAND_JOINT_THUMB_METACARPAL_EXT,
		XR_HAND_JOINT_THUMB_PROXIMAL_EXT,
		XR_HAND_JOINT_THUMB_DISTAL_EXT,

		XR_HAND_JOINT_WRIST_EXT,
		XR_HAND_JOINT_INDEX_METACARPAL_EXT,
		XR_HAND_JOINT_INDEX_PROXIMAL_EXT,
		XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT,
		XR_HAND_JOINT_INDEX_DISTAL_EXT,

		XR_HAND_JOINT_WRIST_EXT,
		XR_HAND_JOINT_MIDDLE_METACARPAL_EXT,
		XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT,
		XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT,
		XR_HAND_JOINT_MIDDLE_DISTAL_EXT,

		XR_HAND_JOINT_WRIST_EXT,
		XR_HAND_JOINT_RING_METACARPAL_EXT,
		XR_HAND_JOINT_RING_PROXIMAL_EXT,
		XR_HAND_JOINT_RING_INTERMEDIATE_EXT,
		XR_HAND_JOINT_RING_DISTAL_EXT,

		XR_HAND_JOINT_WRIST_EXT,
		XR_HAND_JOINT_LITTLE_METACARPAL_EXT,
		XR_HAND_JOINT_LITTLE_PROXIMAL_EXT,
		XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT,
		XR_HAND_JOINT_LITTLE_DISTAL_EXT
	};

	// TODO: This should support rigs that may be set up in a different way, maybe allowing the developer to map their rig?
	std::string GetBoneName(XrHandJointEXT joint, bool isLeftHand)
	{
		const std::string side = isLeftHand ? "_l" : "_r";
		const std::string fingerBase = "finger_";

		switch (joint)
		{
		case XR_HAND_JOINT_WRIST_EXT:
			return "wrist" + side;

			// Thumb
		case XR_HAND_JOINT_THUMB_METACARPAL_EXT:
			return fingerBase + "thumb_meta" + side;
		case XR_HAND_JOINT_THUMB_PROXIMAL_EXT:
			return fingerBase + "thumb_0" + side;
		case XR_HAND_JOINT_THUMB_DISTAL_EXT:
			return fingerBase + "thumb_1" + side;
		case XR_HAND_JOINT_THUMB_TIP_EXT:
			return fingerBase + "thumb" + side + "_end";

			// Index
		case XR_HAND_JOINT_INDEX_METACARPAL_EXT:
			return fingerBase + "index_meta" + side;
		case XR_HAND_JOINT_INDEX_PROXIMAL_EXT:
			return fingerBase + "index_0" + side;
		case XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT:
			return fingerBase + "index_1" + side;
		case XR_HAND_JOINT_INDEX_DISTAL_EXT:
			return fingerBase + "index_2" + side;
		case XR_HAND_JOINT_INDEX_TIP_EXT:
			return fingerBase + "index" + side + "_end";

			// Middle
		case XR_HAND_JOINT_MIDDLE_METACARPAL_EXT:
			return fingerBase + "middle_meta" + side;
		case XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT:
			return fingerBase + "middle_0" + side;
		case XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT:
			return fingerBase + "middle_1" + side;
		case XR_HAND_JOINT_MIDDLE_DISTAL_EXT:
			return fingerBase + "middle_2" + side;
		case XR_HAND_JOINT_MIDDLE_TIP_EXT:
			return fingerBase + "middle" + side + "_end";

			// Ring
		case XR_HAND_JOINT_RING_METACARPAL_EXT:
			return fingerBase + "ring_meta" + side;
		case XR_HAND_JOINT_RING_PROXIMAL_EXT:
			return fingerBase + "ring_0" + side;
		case XR_HAND_JOINT_RING_INTERMEDIATE_EXT:
			return fingerBase + "ring_1" + side;
		case XR_HAND_JOINT_RING_DISTAL_EXT:
			return fingerBase + "ring_2" + side;
		case XR_HAND_JOINT_RING_TIP_EXT:
			return fingerBase + "ring" + side + "_end";

			// Pinky
		case XR_HAND_JOINT_LITTLE_METACARPAL_EXT:
			return fingerBase + "pinky_meta" + side;
		case XR_HAND_JOINT_LITTLE_PROXIMAL_EXT:
			return fingerBase + "pinky_0" + side;
		case XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT:
			return fingerBase + "pinky_1" + side;
		case XR_HAND_JOINT_LITTLE_DISTAL_EXT:
			return fingerBase + "pinky_2" + side;
		case XR_HAND_JOINT_LITTLE_TIP_EXT:
			return fingerBase + "pinky" + side + "_end";

			// Unmapped
		default:
			return "";
		}
	}


public:
	// Interactions
	virtual void CreateActionSet(std::string actionSetName, std::string readableName, int priority) = 0;
	virtual void SuggestBindings() = 0;
	virtual void RecordCurrentBindings() = 0;

	virtual void CreateActionPoses() = 0;
	virtual void AttachActionSet() = 0;

	virtual void PollActions(XrTime predictedTime) = 0;
	virtual void ObjectInteraction() = 0;

	virtual void CreateHandTrackers() = 0;
	virtual void TrackHands(XrTime predictedTime) = 0;

public:
	virtual void CreateSession() = 0;
	inline virtual void DestroySession()
	{
		if (mSession != XR_NULL_HANDLE)
			xrDestroySession(mSession);

		//for (int i = 0; i < 2; i++)
		//{
		//	if (xrDestroyHandTrackerEXT)
		//		xrDestroyHandTrackerEXT(mHands[i].mHandTracker);
		//}
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

	Hand mHands[2];
};
