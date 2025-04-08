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

void XRSession_OpenGL::CreateReferenceSpace()
{
	// Create reference XrSpace, specifying a local space with identity pose for origin.
	XrReferenceSpaceCreateInfo referenceSpaceCI{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	referenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	referenceSpaceCI.poseInReferenceSpace = { {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} };
	xrCreateReferenceSpace(mSession, &referenceSpaceCI, &mLocalSpace);
}

void XRSession_OpenGL::DestroyReferenceSpace()
{
	xrDestroySpace(mLocalSpace);
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

		//mRenderer->RenderFrame(width, height);

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
