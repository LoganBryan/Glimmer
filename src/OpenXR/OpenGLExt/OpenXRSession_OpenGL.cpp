#define XR_USE_PLATFORM_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unknwn.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "OpenXR/OpenGLExt/OpenXRSession_OpenGL.h"
#include "OpenXRDebugUtils.h"
#include <cstdio>
#include <cmath>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

void XRSession_OpenGL::CreateSession()
{
#ifdef _WIN32
	PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
	xrGetInstanceProcAddr(mXrInstanceManager->GetInstance(), "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLGraphicsRequirementsKHR));

	XrGraphicsRequirementsOpenGLKHR glRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
	pfnGetOpenGLGraphicsRequirementsKHR(mXrInstanceManager->GetInstance(), mXrInstanceManager->GetSystemID(), &glRequirements);

	printf("OpenGL min version supported: %d.%d.%d\n",
		XR_VERSION_MAJOR(glRequirements.minApiVersionSupported),
		XR_VERSION_MINOR(glRequirements.minApiVersionSupported),
		XR_VERSION_PATCH(glRequirements.minApiVersionSupported));

	const char* currentOGLVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	printf("Current OpenGL version: %s\n", currentOGLVersion);

	// TODO: could verify current context meets requirements

	XrGraphicsBindingOpenGLWin32KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
	graphicsBinding.hDC = m_hdc;
	graphicsBinding.hGLRC = m_glrc;

	XrSessionCreateInfo sessionCI{ XR_TYPE_SESSION_CREATE_INFO };
	sessionCI.next = &graphicsBinding;
	sessionCI.systemId = mXrInstanceManager->GetSystemID();
	xrCreateSession(mXrInstanceManager->GetInstance(), &sessionCI, &mSession);
#else
	throw std:runtime_error("Session created without graphics binding!");
#endif // _WIN32
}

void XRSession_OpenGL::PollSystemEvents()
{
}

void XRSession_OpenGL::PollEvents()
{
	// Poll OpenXR for a new event
	XrEventDataBuffer eventData{ XR_TYPE_EVENT_DATA_BUFFER };
	auto XrPollEvents = [&]() -> bool
		{
			eventData = { XR_TYPE_EVENT_DATA_BUFFER };
			return xrPollEvent(mXrInstanceManager->GetInstance(), &eventData) == XR_SUCCESS;
		};

	while (XrPollEvents())
	{
		switch (eventData.type)
		{
			// Log number of lost events from runtime
		case XR_TYPE_EVENT_DATA_EVENTS_LOST:
		{
			XrEventDataEventsLost* eventsLost = reinterpret_cast<XrEventDataEventsLost*>(&eventData);
			printf("OPENXR: Events Lost: %i!", eventsLost->lostEventCount);
			break;
		}
		// Log a pending instance loss and shutdown application
		case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
		{
			XrEventDataInstanceLossPending* instanceLossPending = reinterpret_cast<XrEventDataInstanceLossPending*>(&eventData);
			printf("OPENXR: Instance Loss Pending at: %I64i!", instanceLossPending->lossTime);
			mSessionRunning = false;
			mApplicationRunning = false;
			break;
		}
		// Log that the interaction profile has changed
		case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
		{
			XrEventDataInteractionProfileChanged* interactionProfileChanged = reinterpret_cast<XrEventDataInteractionProfileChanged*>(&eventData);
			std::cout << "OPENXR: Interaction Profile changed for session: " << interactionProfileChanged->session << std::endl;
			RecordCurrentBindings();
			if (interactionProfileChanged->session != mSession)
			{
				printf("XrEventDataInteractionProfileCHanged for an unknown session!");
				break;
			}
			break;
		}
		// Log that there's a reference space change pending
		case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
		{
			XrEventDataReferenceSpaceChangePending* referenceSpaceChangePending = reinterpret_cast<XrEventDataReferenceSpaceChangePending*>(&eventData);
			std::cout << "OPENXR: Reference Space Change pending for session: " << referenceSpaceChangePending->session << std::endl;
			if (referenceSpaceChangePending->session != mSession)
			{
				printf("XrEventDataReferenceSpaceChangePending for unknown session!");
				break;
			}
			break;
		}
		// Session state changes
		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
		{
			XrEventDataSessionStateChanged* sessionStateChanged = reinterpret_cast<XrEventDataSessionStateChanged*>(&eventData);
			if (sessionStateChanged->session != mSession)
			{
				printf("XrEventDataSessionStateChanged for unknown session!");
				break;
			}

			if (sessionStateChanged->state == XR_SESSION_STATE_READY)
			{
				// SessionState is ready. Begin using XrViewConfigurationType
				XrSessionBeginInfo sessionBeginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
				sessionBeginInfo.primaryViewConfigurationType = mXrInstanceManager->GetViewConfiguration();
				xrBeginSession(mSession, &sessionBeginInfo);
				printf("OpenXR Session is running!\n");
				mSessionRunning = true;
			}
			if (sessionStateChanged->state == XR_SESSION_STATE_STOPPING)
			{
				// Stopping. End XrSession
				xrEndSession(mSession);
				printf("OpenXR Session is stopping..\n");
				mSessionRunning = false;
			}
			if (sessionStateChanged->state == XR_SESSION_STATE_EXITING)
			{
				// Exit application
				printf("OpenXR Engine is exiting..\n");
				mSessionRunning = false;
				mApplicationRunning = false;
			}
			if (sessionStateChanged->state == XR_SESSION_STATE_LOSS_PENDING)
			{
				// SessionState is loss pending. Exit application - XrInstance and XrSession can be reestablished, for now exiting is simpler
				printf("OpenXR Engine is loss pending!\n");
				mSessionRunning = false;
				mApplicationRunning = false;
			}
			mSessionState = sessionStateChanged->state;
			break;
		}
		default:
		{
			break;
		}
		}
	}
}

bool XRSession_OpenGL::RenderLayer(RenderLayerInfo& renderlayerInfo)
{
	// Locate views from view config within reference space at display time
	std::vector<XrView> views(mXrInstanceManager->GetViewConfigurationViews().size(), { XR_TYPE_VIEW });
	uint32_t viewCount = 0;
	XrViewState viewState{ XR_TYPE_VIEW_STATE };

	XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
	viewLocateInfo.viewConfigurationType = mXrInstanceManager->GetViewConfiguration();
	viewLocateInfo.displayTime = renderlayerInfo.predictedDisplayTime;
	viewLocateInfo.space = mLocalSpace;

	XrResult result = xrLocateViews(mSession, &viewLocateInfo, &viewState, static_cast<uint32_t>(views.size()), &viewCount, views.data());
	if (result != XR_SUCCESS)
	{
		printf("Failed to locate Views!");
		return false;
	}

	if (XR_SUCCEEDED(result))
	{
		mLastFrameViews.resize(viewCount);
		memcpy(mLastFrameViews.data(), views.data(), viewCount * sizeof(XrView));
	}
	else
	{
		// Use previous frame view as a fallback
		if (mLastFrameViews.empty())
		{
			mLastFrameViews.resize(mXrInstanceManager->GetViewConfigurationViews().size());

			for (auto& view : mLastFrameViews)
			{
				view.pose = { {0,0,0,1}, {0,0,0} };
				view.fov = { 90 };
			}
		}
	}

	renderlayerInfo.layerProjectionViews.resize(viewCount, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW });

	// Per view in view config
	for (uint32_t i = 0; i < viewCount; i++)
	{
		std::vector<SwapchainInfo> colorSwapchainInfos = mSwapchainManager->GetColorSwapchainInfo();
		std::vector<SwapchainInfo> depthSwapchainInfos = mSwapchainManager->GetDepthSwapchainInfo();

		SwapchainInfo& colorSwapchainInfo = colorSwapchainInfos[i];
		SwapchainInfo& depthSwapchainInfo = depthSwapchainInfos[i];

		// Acquire and wait for an image from swapchains
		// Get image index of an image in swapchains
		uint32_t colorImageIndex = 0;
		uint32_t depthImageIndex = 0;
		XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
		xrAcquireSwapchainImage(colorSwapchainInfo.swapchain, &acquireInfo, &colorImageIndex);
		xrAcquireSwapchainImage(depthSwapchainInfo.swapchain, &acquireInfo, &depthImageIndex);

		XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
		waitInfo.timeout = XR_INFINITE_DURATION;
		xrWaitSwapchainImage(colorSwapchainInfo.swapchain, &waitInfo);
		xrWaitSwapchainImage(depthSwapchainInfo.swapchain, &waitInfo);

		// Get width and heigh and construct viewport and scissors
		const uint32_t& width = mXrInstanceManager->GetViewConfigurationViews()[i].recommendedImageRectWidth;
		const uint32_t& height = mXrInstanceManager->GetViewConfigurationViews()[i].recommendedImageRectHeight;

		renderlayerInfo.layerProjectionViews[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
		renderlayerInfo.layerProjectionViews[i].pose = views[i].pose;
		renderlayerInfo.layerProjectionViews[i].fov = views[i].fov;
		renderlayerInfo.layerProjectionViews[i].subImage.swapchain = colorSwapchainInfo.swapchain;
		renderlayerInfo.layerProjectionViews[i].subImage.imageRect.offset = { 0, 0 };
		renderlayerInfo.layerProjectionViews[i].subImage.imageRect.extent = { static_cast<int32_t>(width), static_cast<int32_t>(height) };
		renderlayerInfo.layerProjectionViews[i].subImage.imageArrayIndex = 0;

		GLuint colorTexture = static_cast<GLuint>(reinterpret_cast<uintptr_t>(mSwapchainManager->GetSwapchainImage(colorSwapchainInfo.swapchain, colorImageIndex)));
		GLuint depthTexture = static_cast<GLuint>(reinterpret_cast<uintptr_t>(mSwapchainManager->GetSwapchainImage(depthSwapchainInfo.swapchain, depthImageIndex)));

		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			printf("Framebuffer is not complete!");
		}

		glViewport(0, 0, width, height);

		glm::mat4 viewMatrix = XrPoseToGLMMatrix(views[i].pose);
		glm::mat4 projMatrix = CreateProjectionMatrix(views[i].fov, 0.1f, 100.0f);
		mRenderer->RenderEye(viewMatrix, projMatrix, fbo, width, height);
		mRenderer->RenderHands(std::array<XrPosef, 2>{ mHandPose[0], mHandPose[1] }, std::array<XrActionStatePose, 2>{ mHandPoseState[0], mHandPoseState[1] });

		const int parentIndices[XR_HAND_JOINT_COUNT_EXT] =
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

		if (mXrInstanceManager->handTrackingSystemProperties.supportsHandTracking)
		{
			for (int j = 0; j < 2; j++)
			{
				auto hand = mHands[j];
				XrVector3f handColor = { 1.0f, 1.0f, 0.0f };
				for (int k = 0; k < XR_HAND_JOINT_COUNT_EXT; k++)
				{
					int parentIDx = parentIndices[k];
					if (parentIDx < 0 || parentIDx >= XR_HAND_JOINT_COUNT_EXT) continue;

					auto& joint = hand.mJointLocations[k];
					auto& parent = hand.mJointLocations[parentIDx];

					bool jointValid = (joint.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;
					bool parentValid = (parent.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;

					if (jointValid && parentValid)
						mRenderer->RenderLine(joint.pose.position, parent.pose.position, handColor);
				}
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &fbo);

		// Release swapchain
		XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		xrReleaseSwapchainImage(colorSwapchainInfo.swapchain, &releaseInfo);
		xrReleaseSwapchainImage(depthSwapchainInfo.swapchain, &releaseInfo);
	}

	renderlayerInfo.layerProjection.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
	renderlayerInfo.layerProjection.space = mLocalSpace;
	renderlayerInfo.layerProjection.viewCount = static_cast<uint32_t>(renderlayerInfo.layerProjectionViews.size());
	renderlayerInfo.layerProjection.views = renderlayerInfo.layerProjectionViews.data();

	return true;
}

void XRSession_OpenGL::RenderDesktopWindow()
{
	GLFWwindow* window = mWindow;

	int windowWidth, windowHeight;
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

	glViewport(0, 0, windowWidth, windowHeight);

	float windowAspect = static_cast<float>(windowWidth) / std::max(1.0f, static_cast<float>(windowHeight));

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 view = GetLeftEyeViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(60.0f), windowAspect, 0.1f, 100.0f);

	mRenderer->RenderEye(view, proj, 0, windowWidth, windowHeight);
	mRenderer->RenderHands(std::array<XrPosef, 2>{ mHandPose[0], mHandPose[1] }, std::array<XrActionStatePose, 2>{ mHandPoseState[0], mHandPoseState[1] });

	SwapBuffers(m_hdc);
}

void XRSession_OpenGL::RenderFrame()
{
	// Get XrFrameState for timing and rendering info
	XrFrameState frameState{ XR_TYPE_FRAME_STATE };
	XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
	xrWaitFrame(mSession, &frameWaitInfo, &frameState);

	// Tell OpenXR compositor that the application is beginning the frame
	XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
	xrBeginFrame(mSession, &frameBeginInfo);

	bool rendered = false;
	RenderLayerInfo renderLayerInfo;
	renderLayerInfo.predictedDisplayTime = frameState.predictedDisplayTime;

	// Check the session is active and that we should render
	bool sessionActive = { mSessionState == XR_SESSION_STATE_SYNCHRONIZED || mSessionState == XR_SESSION_STATE_VISIBLE || mSessionState == XR_SESSION_STATE_FOCUSED };
	if (sessionActive && frameState.shouldRender)
	{
		PollActions(frameState.predictedDisplayTime);
		ObjectInteraction();
		rendered = RenderLayer(renderLayerInfo);
		if (rendered)
			renderLayerInfo.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&renderLayerInfo.layerProjection));
	}

	// Tell OpenXR the frame is finished with
	XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO };
	frameEndInfo.displayTime = frameState.predictedDisplayTime;
	frameEndInfo.environmentBlendMode = mXrInstanceManager->mEnvironmentBlendMode;
	frameEndInfo.layerCount = static_cast<uint32_t>(renderLayerInfo.layers.size());
	frameEndInfo.layers = renderLayerInfo.layers.data();
	xrEndFrame(mSession, &frameEndInfo);
}

void XRSession_OpenGL::CreateActionSet(std::string actionSetName, std::string readableName, int priority)
{
	XrActionSetCreateInfo actionSetCI{ XR_TYPE_ACTION_SET_CREATE_INFO };
	strncpy(actionSetCI.actionSetName, actionSetName.c_str(), XR_MAX_ACTION_SET_NAME_SIZE);
	strncpy(actionSetCI.localizedActionSetName, readableName.c_str(), XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
	actionSetCI.priority = priority;

	xrCreateActionSet(mXrInstanceManager->GetInstance(), &actionSetCI, &mActionSet);

	auto CreateAction = [this](XrAction& xrAction, const char* name, XrActionType xrActionType, std::vector<const char*> subactionPaths = {}) -> void
		{
			XrActionCreateInfo actionCI{ XR_TYPE_ACTION_CREATE_INFO };
			actionCI.actionType = xrActionType;
			std::vector<XrPath> subactionXrPaths;

			for (auto p : subactionPaths)
			{
				subactionXrPaths.push_back(mXrInstanceManager->CreateXrPath(p));
			}
			actionCI.countSubactionPaths = (uint32_t)subactionXrPaths.size();
			actionCI.subactionPaths = subactionXrPaths.data();
			strncpy(actionCI.actionName, name, XR_MAX_ACTION_NAME_SIZE);
			strncpy(actionCI.localizedActionName, name, XR_MAX_LOCALIZED_ACTION_NAME_SIZE);
			xrCreateAction(mActionSet, &actionCI, &xrAction);
		};

	CreateAction(mGrabAction, "grab-object", XR_ACTION_TYPE_FLOAT_INPUT, { "/user/hand/left", "/user/hand/right" });
	CreateAction(mPalmPoseAction, "palm-pose", XR_ACTION_TYPE_POSE_INPUT, { "/user/hand/left", "/user/hand/right" });
	CreateAction(mBuzzAction, "buzz", XR_ACTION_TYPE_VIBRATION_OUTPUT, { "/user/hand/left", "/user/hand/right" });

	// XrPaths for subaction path names
	mHandPaths[0] = mXrInstanceManager->CreateXrPath("/user/hand/left");
	mHandPaths[1] = mXrInstanceManager->CreateXrPath("/user/hand/right");
}

void XRSession_OpenGL::SuggestBindings()
{
	auto SuggestBindings = [this](const char* profilePath, std::vector<XrActionSuggestedBinding> bindings) -> bool
		{
			XrInteractionProfileSuggestedBinding interactionProfileSuggestedBinding{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
			interactionProfileSuggestedBinding.interactionProfile = mXrInstanceManager->CreateXrPath(profilePath);
			interactionProfileSuggestedBinding.suggestedBindings = bindings.data();
			interactionProfileSuggestedBinding.countSuggestedBindings = (uint32_t)bindings.size();

			if (xrSuggestInteractionProfileBindings(mXrInstanceManager->GetInstance(), &interactionProfileSuggestedBinding) == XrResult::XR_SUCCESS)
				return true;

			printf("Failed to suggest bindings with %s!", profilePath);
			return false;
		};

	bool anyOK = false;
	// For native support, that profile can be called; such as /interaction_profiles/oculus/touch_controller - mainly for extended function with that hardware such as squeezing with the touch controllers
	anyOK |= SuggestBindings("/interaction_profiles/khr/simple_controller", {
		{mGrabAction, mXrInstanceManager->CreateXrPath("/user/hand/right/input/select/click")},
		{mGrabAction, mXrInstanceManager->CreateXrPath("/user/hand/left/input/select/click")},
		{mPalmPoseAction, mXrInstanceManager->CreateXrPath("/user/hand/left/input/grip/pose")},
		{mPalmPoseAction, mXrInstanceManager->CreateXrPath("/user/hand/right/input/grip/pose")},
		{mBuzzAction, mXrInstanceManager->CreateXrPath("/user/hand/left/output/haptic")},
		{mBuzzAction, mXrInstanceManager->CreateXrPath("/user/hand/right/output/haptic")} });

	if (!anyOK)
		throw;
}

void XRSession_OpenGL::RecordCurrentBindings()
{
	if (mSession)
	{
		XrInteractionProfileState interactionProfile = { XR_TYPE_INTERACTION_PROFILE_STATE, 0, 0 };
		xrGetCurrentInteractionProfile(mSession, mHandPaths[0], &interactionProfile);
		if (interactionProfile.interactionProfile)
			printf("user/hand/left ActiveProfile %s", mXrInstanceManager->FromXrPath(interactionProfile.interactionProfile).c_str());
		xrGetCurrentInteractionProfile(mSession, mHandPaths[1], & interactionProfile);
		if (interactionProfile.interactionProfile)
			printf("user/hand/right ActiveProfile %s", mXrInstanceManager->FromXrPath(interactionProfile.interactionProfile).c_str());
	}
}

void XRSession_OpenGL::CreateActionPoses()
{
	auto CreateActionPoseSpace = [this](XrSession session, XrAction xrAction, const char* subactionPath = nullptr) -> XrSpace
		{
			XrSpace xrSpace;
			const XrPosef xrPoseIdentity = { {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} };

			XrActionSpaceCreateInfo actionSpaceCI{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
			actionSpaceCI.action = xrAction;
			actionSpaceCI.poseInActionSpace = xrPoseIdentity;
			if (subactionPath)
				actionSpaceCI.subactionPath = mXrInstanceManager->CreateXrPath(subactionPath);
			xrCreateActionSpace(session, &actionSpaceCI, &xrSpace);
			return xrSpace;
		};
	mHandPoseSpace[0] = CreateActionPoseSpace(mSession, mPalmPoseAction, "/user/hand/left");
	mHandPoseSpace[1] = CreateActionPoseSpace(mSession, mPalmPoseAction, "/user/hand/right");
}

void XRSession_OpenGL::AttachActionSet()
{
	// Attach action set to session, multiple can be attached to one session
	XrSessionActionSetsAttachInfo actionSetAttachInfo{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
	actionSetAttachInfo.countActionSets = 1;
	actionSetAttachInfo.actionSets = &mActionSet;
	xrAttachSessionActionSets(mSession, &actionSetAttachInfo);
}

void XRSession_OpenGL::PollActions(XrTime predictedTime)
{
	XrActiveActionSet activeActionSet{};
	activeActionSet.actionSet = mActionSet;
	activeActionSet.subactionPath = XR_NULL_PATH;

	XrActionsSyncInfo actionsSyncInfo{ XR_TYPE_ACTIONS_SYNC_INFO };
	actionsSyncInfo.countActiveActionSets = 1;
	actionsSyncInfo.activeActionSets = &activeActionSet;
	xrSyncActions(mSession, &actionsSyncInfo);

	XrActionStateGetInfo actionStateGetInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
	actionStateGetInfo.action = mPalmPoseAction;

	// For each hand get pose state
	for (int i = 0; i < 2; i++)
	{
		actionStateGetInfo.subactionPath = mHandPaths[i];
		xrGetActionStatePose(mSession, &actionStateGetInfo, &mHandPoseState[i]);
		if (mHandPoseState[i].isActive)
		{
			XrSpaceLocation spaceLocation{ XR_TYPE_SPACE_LOCATION };
			XrResult result = xrLocateSpace(mHandPoseSpace[i], mLocalSpace, predictedTime, &spaceLocation);
			if (XR_UNQUALIFIED_SUCCESS(result) &&
				(spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
				(spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
				mHandPose[i] = spaceLocation.pose;
			else
				mHandPoseState[i].isActive = false;
		}
	}
	for (int i = 0; i < 2; i++)
	{
		actionStateGetInfo.action = mGrabAction;
		actionStateGetInfo.subactionPath = mHandPaths[i];
		xrGetActionStateFloat(mSession, &actionStateGetInfo, &mGrabState[i]);
	}

	if (mXrInstanceManager->handTrackingSystemProperties.supportsHandTracking)
	{
		XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };

		for (int i = 0; i < 2; i++)
		{
			bool unobstructed = true;
			Hand& hand = mHands[i];
			XrHandJointsMotionRangeInfoEXT motionRangeInfo{ XR_TYPE_HAND_JOINTS_MOTION_RANGE_INFO_EXT };
			motionRangeInfo.handJointsMotionRange = unobstructed ? XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT : XR_HAND_JOINTS_MOTION_RANGE_CONFORMING_TO_CONTROLLER_EXT;
			XrHandJointsLocateInfoEXT locateInfo{ XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT, &motionRangeInfo };
			locateInfo.baseSpace = mLocalSpace;
			locateInfo.time = predictedTime;

			XrHandJointLocationsEXT locations{ XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
			locations.jointCount = (uint32_t)XR_HAND_JOINT_COUNT_EXT;
			locations.jointLocations = hand.mJointLocations;
			mXrInstanceManager->xrLocateHandJointsEXT(hand.mHandTracker, &locateInfo, &locations);
		}
	}
}

glm::vec3 XRSession_OpenGL::GetHandPosition(int handIndex) const
{
	return glm::vec3(
		mHandPose[handIndex].position.x,
		mHandPose[handIndex].position.y,
		mHandPose[handIndex].position.z
	);
}

glm::quat XRSession_OpenGL::GetHandRotation(int handIndex) const
{
	return  glm::quat(
		mHandPose[handIndex].orientation.w,
		mHandPose[handIndex].orientation.x,
		mHandPose[handIndex].orientation.y,
		mHandPose[handIndex].orientation.z
	);
}

void XRSession_OpenGL::ObjectInteraction()
{
	const float GRABDISTANCE = 0.2f; // TODO: Allow this to be set!

	for (int i = 0; i < 2; i++)
	{
		if (mGrabState[i].isActive && mGrabState[i].currentState > 0.5f)
		{
			if (mGrabbedObject[i] == -1)
			{
				// Ray dir from hand pose
				glm::vec3 handPos = GetHandPosition(i);
				glm::quat handRot = GetHandRotation(i);
				glm::vec3 rayDir = handRot * glm::vec3(0, -1, 0);

				// Find closest object along ray
				int closestObject = -1;
				float closestDistance = FLT_MAX;
				glm::vec3 closestHitPoint;

				for (size_t j = 0; j < mRenderer->GetObjects().size(); j++)
				{
					auto& object = mRenderer->GetObjects()[j];
					if (object->isGrabbed) continue;

					// Simple sphere 
					float radius = 0.5f;
					glm::vec3 handToObject = object->transform.position - handPos;
					float t = glm::dot(handToObject, rayDir);

					if (t >= 0)
					{
						glm::vec3 nearest = handPos + rayDir * t;
						float distance = glm::length(nearest - object->transform.position);

						if (distance < radius && t < closestDistance && t < GRABDISTANCE)
						{
							closestDistance = t;
							closestObject = j;
							closestHitPoint = nearest;
						}
					}
				}

				if (closestObject != -1)
				{
					mGrabbedObject[i] = closestObject;
					glm::vec3 grabOffset = mRenderer->GetObjects()[closestObject]->transform.position - closestHitPoint;

					mRenderer->GetObjects()[closestObject]->grabOffset = grabOffset;
					mRenderer->GetObjects()[closestObject]->isGrabbed = true;
					mRenderer->GetObjects()[closestObject]->grabbedByHand = i;

					// Apply offset from hand local space
					glm::quat handInv = glm::inverse(handRot);
					mRenderer->GetObjects()[closestObject]->grabRotationOffset = handInv * mRenderer->GetObjects()[closestObject]->transform.rotation;
					mRenderer->GetObjects()[closestObject]->grabOffset = handInv * (mRenderer->GetObjects()[closestObject]->transform.position - handPos);
				}
			}

			if (mGrabbedObject[i] != -1)
			{
				auto& object = mRenderer->GetObjects()[mGrabbedObject[i]];
				glm::vec3 handPos = GetHandPosition(i);
				glm::quat handRot = GetHandRotation(i);

				//// Apply offset from hand local space
				//glm::quat handInv = glm::inverse(handRot);
				//object->grabRotationOffset = handInv * object->transform.rotation;
				//object->grabOffset = handInv * (object->transform.position - handPos);

				object->transform.position = handPos + (handRot * object->grabOffset);
				object->transform.rotation = handRot * (object->grabRotationOffset);

				if (!mGrabHapticTriggered[i])
				{
					XrHapticVibration vibration{ XR_TYPE_HAPTIC_VIBRATION };
					vibration.amplitude = 1.0f;
					vibration.duration = 150000000.0f;
					vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

					// Prepare haptic action
					XrHapticActionInfo hapticActionInfo{ XR_TYPE_HAPTIC_ACTION_INFO };
					hapticActionInfo.action = mBuzzAction;
					hapticActionInfo.subactionPath = mHandPaths[i];

					xrApplyHapticFeedback(mSession, &hapticActionInfo, reinterpret_cast<XrHapticBaseHeader*>(&vibration));

					mGrabHapticTriggered[i] = true;
				}
			}
		}
		else
		{
			if (mGrabbedObject[i] != -1)
			{
				mRenderer->GetObjects()[mGrabbedObject[i]]->isGrabbed = false;
				mRenderer->GetObjects()[mGrabbedObject[i]]->grabbedByHand = -1;
				mGrabbedObject[i] = -1;
				mGrabHapticTriggered[i] = false;
			}
		}
	}
}
