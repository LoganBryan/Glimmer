#define XR_USE_PLATFORM_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unknwn.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

#include "Application/Application.h"
#include <OpenXRDebugUtils.h>
#include <DebugOutput.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

HWND _hwnd;
HDC _hdc;
HGLRC _glrc;

enum GraphicsAPI_Type : uint8_t {
	UNKNOWN,
	D3D11,
	D3D12,
	OPENGL,
	OPENGL_ES,
	VULKAN
};

// TODO: move these graphics functions to their own class/ file (same with the openxr class)
const char* GetGraphicsAPIInstanceExtensionString(GraphicsAPI_Type type) {
#if defined(XR_USE_GRAPHICS_API_D3D11)
	if (type == D3D11) {
		return XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
	}
#endif
#if defined(XR_USE_GRAPHICS_API_D3D12)
	if (type == D3D12) {
		return XR_KHR_D3D12_ENABLE_EXTENSION_NAME;
	}
#endif
#if defined(XR_USE_GRAPHICS_API_OPENGL)
	if (type == OPENGL) {
		return XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
	}
#endif
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
	if (type == OPENGL_ES) {
		return XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME;
	}
#endif
#if defined(XR_USE_GRAPHICS_API_VULKAN)
	if (type == VULKAN) {
		return XR_KHR_VULKAN_ENABLE_EXTENSION_NAME;
	}
#endif
	std::cerr << "ERROR: Unknown Graphics API." << std::endl;
	DEBUG_BREAK;
	return nullptr;
}

class OpenXR
{
public:
	OpenXR(GraphicsAPI_Type apiType)
	{
		printf("OpenXR Engine Starting..\n");
	}
	
	~OpenXR() = default;
	void Run() 
	{
		CreateInstance();
		CreateDebugMessenger();

		GetInstanceProperties();
		GetSystemID();

		CreateSession();

		while (mApplicationRunning)
		{
			PollSystemEvents();
			PollEvents();

			if (mSessionRunning)
			{
				// Rendering
			}
		}

		DestroySession();
		DestroyDebugMessenger();
		DestroyInstance();
	}

private:
	void CreateInstance() 
	{
		XrApplicationInfo appInfo = {};
		strncpy_s(appInfo.applicationName, "Glimmer", XR_MAX_APPLICATION_NAME_SIZE);
		appInfo.applicationVersion = 1;
		strncpy_s(appInfo.engineName, "Glimmer OpenXR Engine", XR_MAX_ENGINE_NAME_SIZE);
		appInfo.engineVersion = 1;
		appInfo.apiVersion = XR_API_VERSION_1_0;

		mInstanceExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
		mInstanceExtensions.push_back("XR_KHR_opengl_enable");

		// Get all API layers from OpenXR runtime
		uint32_t apiLayerCount = 0;
		std::vector<XrApiLayerProperties> apiLayerProps;
		OPENXR_CHECK(xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr), "Failed to enumerate ApiLayerProperties!");
		apiLayerProps.resize(apiLayerCount, { XR_TYPE_API_LAYER_PROPERTIES });
		OPENXR_CHECK(xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, apiLayerProps.data()), "Failed to enumerate ApiLayerProperties!");

		// Check requested API layers against ones in OpenXR. Add found layers to active
		//for (auto& reqLayer : mAPILayers)
		//{
		//	for (auto& layerProp : apiLayerProps)
		//	{
		//		if (strcmp(reqLayer.c_str(), layerProp.layerName) != 0)
		//		{
		//			continue;
		//		}
		//		else
		//		{
		//			// Found layer
		//			mActiveAPILayers.push_back(reqLayer.c_str());
		//			break;
		//		}
		//	}
		//}

		// Get all instance extensions from OpenXR instance.
		uint32_t extensionCount = 0;
		std::vector<XrExtensionProperties> extensionProps;
		OPENXR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr), "Failed to enumerate InstanceExtensionProperties!");
		extensionProps.resize(extensionCount, { XR_TYPE_EXTENSION_PROPERTIES });
		OPENXR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProps.data()), "Failed to enumerate InstanceExtensionProperties!");

		// Check the requested instance extensions against those from the OpenXR runtime
		// Add found extensions to active
		for (auto& requestedInstanceExt : mInstanceExtensions)
		{
			bool found = false;
			for (auto& extProp : extensionProps)
			{
				if (strcmp(requestedInstanceExt.c_str(), extProp.extensionName) != 0)
				{
					continue;
				}
				else
				{
					mActiveInstanceExtensions.push_back(requestedInstanceExt.c_str());
					found = true;
					break;
				}
			}
			if (!found)
			{
				XR_TUT_LOG_ERROR("Failed to find OpenXR instance extension: " << requestedInstanceExt);
			}
		}

		XrInstanceCreateInfo instanceCI{ XR_TYPE_INSTANCE_CREATE_INFO };
		instanceCI.createFlags = 0;
		instanceCI.applicationInfo = appInfo;
		instanceCI.enabledApiLayerCount = 0;
		//instanceCI.enabledApiLayerNames = mActiveAPILayers.data();
		instanceCI.enabledExtensionCount = static_cast<uint32_t>(mActiveInstanceExtensions.size());
		instanceCI.enabledExtensionNames = mActiveInstanceExtensions.data();
		OPENXR_CHECK(xrCreateInstance(&instanceCI, &mXrInstance), "Failed to create instance!");
	}

	void DestroyInstance() 
	{
		OPENXR_CHECK(xrDestroyInstance(mXrInstance), "Failed to destroy instance!");
	}

	void GetInstanceProperties()
	{
		XrInstanceProperties instanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
		OPENXR_CHECK(xrGetInstanceProperties(mXrInstance, &instanceProperties), "Failed to get InstanceProperties!");

		XR_TUT_LOG("OpenXR Runtime: " << instanceProperties.runtimeName << " - " << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << "." << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << "." << XR_VERSION_PATCH(instanceProperties.runtimeVersion));
	}

	void GetSystemID()
	{
		// Get XrSystemId from instance and supplied XrFormFactor
		XrSystemGetInfo systemGI{ XR_TYPE_SYSTEM_GET_INFO };
		systemGI.formFactor = mFormFactor;
		OPENXR_CHECK(xrGetSystem(mXrInstance, &systemGI, &mSystemID), "Failed to get SystemID!");

		// Get system properties for general hardware and vendor info
		OPENXR_CHECK(xrGetSystemProperties(mXrInstance, mSystemID, &mSystemProperties), "Failed to get SystemProperties!");
	}

	void CreateDebugMessenger() 
	{
		// Check 'XR_EXT_debug_utils' is in active instance extensions before creating debug messenger
		if (IsStringInVector(mActiveInstanceExtensions, XR_EXT_DEBUG_UTILS_EXTENSION_NAME))
			mDebugUtilsMessenger = CreateOpenXRDebugUtilsMessenger(mXrInstance); 
	}

	void DestroyDebugMessenger()
	{
		// Check 'XR_EXT_debug_utils' is in active instance extensions before destroying debug messenger
		if (mDebugUtilsMessenger != XR_NULL_HANDLE)
			DestroyOpenXRDebugUtilsMessenger(mXrInstance, mDebugUtilsMessenger);
	}

	void CreateSession() 
	{
#ifdef _WIN32
		PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
		OPENXR_CHECK(xrGetInstanceProcAddr(mXrInstance, "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLGraphicsRequirementsKHR)), "Failed to get OpenGL graphics requirements function pointer!");

		XrGraphicsRequirementsOpenGLKHR glRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
		OPENXR_CHECK(pfnGetOpenGLGraphicsRequirementsKHR(mXrInstance, mSystemID, &glRequirements), "Failed to get OpenGL graphics requirements!");

		printf("OpenGL min version supported: %d.%d.%d\n",
			XR_VERSION_MAJOR(glRequirements.minApiVersionSupported),
			XR_VERSION_MINOR(glRequirements.minApiVersionSupported),
			XR_VERSION_PATCH(glRequirements.minApiVersionSupported));

		const char* currentOGLVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		printf("Current OpenGL version: %s\n", currentOGLVersion);

		// TODO: could verify current context meets requirements

		XrGraphicsBindingOpenGLWin32KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
		graphicsBinding.hDC = _hdc;
		graphicsBinding.hGLRC = _glrc;

		XrSessionCreateInfo sessionCI{ XR_TYPE_SESSION_CREATE_INFO };
		sessionCI.next = &graphicsBinding;
		sessionCI.systemId = mSystemID;
		OPENXR_CHECK(xrCreateSession(mXrInstance, &sessionCI, &mSession), "Failed to create session!");
#else
		throw std:runtime_error("Session created without graphics binding!");
#endif // _WIN32
	}

	void DestroySession() 
	{
		if (mSession != XR_NULL_HANDLE)
			OPENXR_CHECK(xrDestroySession(mSession), "Failed to destroy session!");
	}

	void PollSystemEvents() {}
	void PollEvents() 
	{
		// Poll OpenXR for a new event
		XrEventDataBuffer eventData{ XR_TYPE_EVENT_DATA_BUFFER };
		auto XrPollEvents = [&]() -> bool
			{
				eventData = { XR_TYPE_EVENT_DATA_BUFFER };
				return xrPollEvent(mXrInstance, &eventData) == XR_SUCCESS;
			};

		while (XrPollEvents())
		{
			switch (eventData.type)
			{
			// Log number of lost events from runtime
			case XR_TYPE_EVENT_DATA_EVENTS_LOST: 
			{
				XrEventDataEventsLost* eventsLost = reinterpret_cast<XrEventDataEventsLost*>(&eventData);
				XR_TUT_LOG("OPENXR: Events Lost: " << eventsLost->lostEventCount);
				break;
			}
			// Log a pending instance loss and shutdown application
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
			{
				XrEventDataInstanceLossPending* instanceLossPending = reinterpret_cast<XrEventDataInstanceLossPending*>(&eventData);
				XR_TUT_LOG("OPENXR: Instance Loss Pending at: " << instanceLossPending->lossTime);
				mSessionRunning = false;
				mApplicationRunning = false;
				break;
			}
			// Log that the interaction profile has changed
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
			{
				XrEventDataInteractionProfileChanged* interactionProfileChanged = reinterpret_cast<XrEventDataInteractionProfileChanged*>(&eventData);
				XR_TUT_LOG("OPENXR: Interaction Profile changed for session: " << interactionProfileChanged->session);
				if (interactionProfileChanged->session != mSession)
				{
					XR_TUT_LOG("XrEventDataInteractionProfileCHanged for an unknown session!");
					break;
				}
				break;
			}
			// Log that there's a reference space change pending
			case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
			{
				XrEventDataReferenceSpaceChangePending* referenceSpaceChangePending = reinterpret_cast<XrEventDataReferenceSpaceChangePending*>(&eventData);
				XR_TUT_LOG("OPENXR: Reference Space Change pending for session: " << referenceSpaceChangePending->session);
				if (referenceSpaceChangePending->session != mSession)
				{
					XR_TUT_LOG("XrEventDataReferenceSpaceChangePending for unknown session!");
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
					XR_TUT_LOG("XrEventDataSessionStateChanged for unknown session!");
					break;
				}

				if (sessionStateChanged->state == XR_SESSION_STATE_READY)
				{
					// SessionState is ready. Begin using XrViewConfigurationType
					XrSessionBeginInfo sessionBeginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
					sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
					OPENXR_CHECK(xrBeginSession(mSession, &sessionBeginInfo), "Failed to begin session!");
					printf("OpenXR Session is running!\n");
					mSessionRunning = true;
				}
				if (sessionStateChanged->state == XR_SESSION_STATE_STOPPING)
				{
					// Stopping. End XrSession
					OPENXR_CHECK(xrEndSession(mSession), "Failed to end session!");
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

private:
	XrInstance mXrInstance = {};
	//std::vector<const char*> mActiveAPILayers = {};
	std::vector<const char*> mActiveInstanceExtensions = {};
	//std::vector<std::string> mAPILayers = {};
	std::vector<std::string> mInstanceExtensions = {};

	XrDebugUtilsMessengerEXT mDebugUtilsMessenger = {};

	XrFormFactor mFormFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	XrSystemId mSystemID = {};
	XrSystemProperties mSystemProperties = { XR_TYPE_SYSTEM_PROPERTIES };

	//GraphicsAPI_Type mAPIType = UNKNOWN;
	//std::unique_ptr<GraphicsAPI> mGraphicsAPI = nullptr;

	XrSession mSession = XR_NULL_HANDLE;
	XrSessionState mSessionState = XR_SESSION_STATE_UNKNOWN;

	bool mApplicationRunning = true;
	bool mSessionRunning = false;
};

int main()
{
	Application application(1920, 1080, "Glimmer");
	if (!application.Init())
	{
		printf("Application failed to initialize!");
		return -1;
	}
	_hwnd = glfwGetWin32Window(application.GetWindow());
	_hdc = GetDC(_hwnd);
	_glrc = glfwGetWGLContext(application.GetWindow());

	OpenXR openXRAPP(OPENGL);
	openXRAPP.Run();

	application.Run();

	return 0;
}

// TODO: everything here needs to be re-implemented (currently just missing callbacks)
//#define TINYGLTF_IMPLEMENTATION
//
//#include <stdio.h>
//#include <string>
//#include <sstream>
//#include <format>
//#include <vector>
//#include <map>
//#include <memory>
//#include <chrono>
//
//#include <glad/glad.h>
//#include <GLFW/glfw3.h>
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//
//#include <imgui.h>
//#include <imgui_impl_glfw.h>
//#include <imgui_impl_opengl3.h>
//
//#include "stb_image.h"
//
//#include <fastgltf/core.hpp>
//#include <fastgltf/types.hpp>
//#include <fastgltf/tools.hpp>
//
//#include "Shader.h"
//#include "Camera.h"
//#include "FPSCounter.h"
//#include "GltfLoader.h"
//
//#define PI 3.14159265358979323846
//#define TWO_PI 6.2831855
//
//// TODO: General code cleanup
//
//// TEMP
//float vertices[] = {
//	// Pos          // Normal           // TexCoord
//	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
//	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
//	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
//	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
//	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
//	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
//
//	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
//	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
//	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
//	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
//	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
//	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
//
//	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
//	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
//	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
//	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
//	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
//	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
//
//	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
//	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
//	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
//	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
//	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
//	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
//
//	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
//	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
//	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
//	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
//	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
//	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
//
//	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
//	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
//	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
//	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
//	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
//	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
//};
//
//float skyboxVertices[] = {
//	// positions          
//	-1.0f,  1.0f, -1.0f,
//	-1.0f, -1.0f, -1.0f,
//	 1.0f, -1.0f, -1.0f,
//	 1.0f, -1.0f, -1.0f,
//	 1.0f,  1.0f, -1.0f,
//	-1.0f,  1.0f, -1.0f,
//
//	-1.0f, -1.0f,  1.0f,
//	-1.0f, -1.0f, -1.0f,
//	-1.0f,  1.0f, -1.0f,
//	-1.0f,  1.0f, -1.0f,
//	-1.0f,  1.0f,  1.0f,
//	-1.0f, -1.0f,  1.0f,
//
//	 1.0f, -1.0f, -1.0f,
//	 1.0f, -1.0f,  1.0f,
//	 1.0f,  1.0f,  1.0f,
//	 1.0f,  1.0f,  1.0f,
//	 1.0f,  1.0f, -1.0f,
//	 1.0f, -1.0f, -1.0f,
//
//	-1.0f, -1.0f,  1.0f,
//	-1.0f,  1.0f,  1.0f,
//	 1.0f,  1.0f,  1.0f,
//	 1.0f,  1.0f,  1.0f,
//	 1.0f, -1.0f,  1.0f,
//	-1.0f, -1.0f,  1.0f,
//
//	-1.0f,  1.0f, -1.0f,
//	 1.0f,  1.0f, -1.0f,
//	 1.0f,  1.0f,  1.0f,
//	 1.0f,  1.0f,  1.0f,
//	-1.0f,  1.0f,  1.0f,
//	-1.0f,  1.0f, -1.0f,
//
//	-1.0f, -1.0f, -1.0f,
//	-1.0f, -1.0f,  1.0f,
//	 1.0f, -1.0f, -1.0f,
//	 1.0f, -1.0f, -1.0f,
//	-1.0f, -1.0f,  1.0f,
//	 1.0f, -1.0f,  1.0f
//};
//
//float quadVertices[] = { // vertex for a quad to fill the screen
//	// positions   // texCoords
//	-1.0f,  1.0f,  0.0f, 1.0f,
//	-1.0f, -1.0f,  0.0f, 0.0f,
//	 1.0f, -1.0f,  1.0f, 0.0f,
//
//	-1.0f,  1.0f,  0.0f, 1.0f,
//	 1.0f, -1.0f,  1.0f, 0.0f,
//	 1.0f,  1.0f,  1.0f, 1.0f
//};
//
//std::vector<glm::vec3> staticLightPositions = {
//	glm::vec3(1.0f,  1.0f,  1.0f),
//	glm::vec3(2.3f, -3.3f, -4.0f),
//	glm::vec3(-4.0f,  2.0f, -12.0f),
//	glm::vec3(0.5f,  0.8f, -3.0f)
//};
//
//std::vector<glm::vec3> lightPositions = {
//	glm::vec3(1.0f,  1.0f,  1.0f),
//	glm::vec3(2.3f, -3.3f, -4.0f),
//	glm::vec3(-4.0f,  2.0f, -12.0f),
//	glm::vec3(0.0f,  0.0f, -3.0f)
//};
//
//std::vector<std::string> skyboxFaces = {
//	"assets/textures/skybox/right.jpg",
//	"assets/textures/skybox/left.jpg",
//	"assets/textures/skybox/top.jpg",
//	"assets/textures/skybox/bottom.jpg",
//	"assets/textures/skybox/front.jpg",
//	"assets/textures/skybox/back.jpg"
//};
//
//// Todo: Needs moving to it's own class
//void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
//void ProcessInput(GLFWwindow* window);
//void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
//unsigned int LoadTexture(char const* path);
//
//// This will eventually be in the camera class v
//void MouseCallback(GLFWwindow* window, double xpos, double ypos); 
//void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
//
//void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLenum id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
//
//GLenum polyMode = GL_FILL;
//
//int width = 1920;
//int height = 1080;
//int nChannels;
//
//unsigned int textureOne, textureTwo;
//std::string imagePath = "assets/textures/boxTex.png";
//std::string image2Path = "assets/textures/boxSpecular.png";
//std::string image3Path = "assets/textures/boxEmission.png";
//std::string uvPath = "assets/textures/uvmap.jpg";
//
//std::string testModel = "assets/models/flightHelm/FlightHelmet.gltf";
//
//bool firstMouseInput = true;
//float lastMouseX = 400, lastMouseY = 400;
//float yaw = -90.0f;
//float pitch = 0.0f;
//float fov = 90.0f;
//
//Camera mainCamera(glm::vec3(0.0f, 0.0f, 3.0f));
//
//glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
//
//unsigned int GenerateCubemap(std::vector<std::string> faces) 
//{
//	unsigned int textureID;
//	glGenTextures(1, &textureID);
//	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
//
//	int width, height, nChannels;
//	unsigned char* data;
//
//	for (unsigned int i = 0; i < faces.size(); i++)
//	{
//		data = stbi_load(faces[i].c_str(), &width, &height, &nChannels, 0);
//		if (data)
//		{
//			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
//		}
//		else
//		{
//			printf("Cubemap texture failed to load at %s", faces[i].c_str());
//		}
//		stbi_image_free(data);
//	}
//
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//
//	return textureID;
//}
//
//int main()
//{
//	glfwInit();
//	glfwWindowHint(GLFW_SAMPLES, 8);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//
//	// Create window context
//	GLFWwindow* window = glfwCreateWindow(width, height, "Glimmer", NULL, NULL);
//	if (window == NULL)
//	{
//		printf("Failed to create window!");
//		glfwTerminate();
//		return -1;
//	}
//	glfwMakeContextCurrent(window);
//	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
//
//	// Initialize OGL Context
//	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
//	{
//		printf("Failed to initialize GLAD!");
//		return -1;
//	}
//
//	Viewer viewer;
//	GltfLoader gltfObject;
//	//GltfLoader gltf2Object;
//	//GltfLoader gltf3Object;
//	auto gltfFile = std::filesystem::path("assets/models/helmet/DamagedHelmet.gltf");
//	//auto gltfFile = std::filesystem::path("assets/models/flightHelm/FlightHelmet.gltf");
//	glfwSetWindowUserPointer(window, &viewer);
//
//	IMGUI_CHECKVERSION();
//	ImGui::CreateContext();
//	ImGui::StyleColorsDark();
//
//	ImGuiIO& io = ImGui::GetIO(); (void)io;
//	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; right now this being enabled breaks the mouse wrapping code..g kjsej g
//	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
//
//	ImGuiStyle& style = ImGui::GetStyle();
//	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
//	{
//		style.WindowRounding = 0.0f;
//		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
//	}
//
//	ImGui_ImplGlfw_InitForOpenGL(window, true);
//	ImGui_ImplOpenGL3_Init("#version 460");
//
//	//glViewport(0, 0, 800, 600);
//	std::cout << glGetString(GL_RENDERER) << "\n" << glGetString(GL_VERSION) << std::endl;
//
//	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//	glfwSetKeyCallback(window, KeyCallback);
//	//glfwSetCursorPosCallback(window, MouseCallback);
//	glfwSetScrollCallback(window, ScrollCallback);
//
//	// Setup shaders
//	Shader mainShader("shaders/shader.vert", "shaders/shader.frag");
//	Shader lightSourceShader("shaders/lightFullBright.vert", "shaders/lightFullBright.frag");
//	Shader outlineShader("shaders/shader.vert", "shaders/singleColor.frag");
//	Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");
//	Shader framebufferShader("shaders/framebuffer.vert", "shaders/framebuffer.frag");
//
//	framebufferShader.Use();
//	framebufferShader.SetInt("screenTexture", 5);
//
//	// Setup framebuffer object
//	unsigned int fbo;
//	glGenFramebuffers(1, &fbo);
//	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//
//	// Create texture attachment
//	unsigned int textureCBuffer;
//	glGenTextures(1, &textureCBuffer);
//	glBindTexture(GL_TEXTURE_2D, textureCBuffer);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureCBuffer, 0); // Attach
//	//glBindTexture(GL_TEXTURE_2D, 0);
//
//	// Setup renderbuffer object attachment
//	unsigned int rbo;
//	glGenRenderbuffers(1, &rbo);
//	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
//	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
//	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // Attach
//	//glBindRenderbuffer(GL_RENDERBUFFER, 0);
//
//	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
//		printf("Framebuffer %d is incomplete!\n", fbo);
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//
//	// Screen quad
//	unsigned int screenQuadVAO, screenQuadVBO;
//	glGenVertexArrays(1, &screenQuadVAO);
//	glGenBuffers(1, &screenQuadVBO);
//	glBindVertexArray(screenQuadVAO);
//	glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
//	glEnableVertexAttribArray(0);
//	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
//	glEnableVertexAttribArray(1);
//	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
//
//	// Object for light source
//	unsigned int VBO;
//	glGenBuffers(1, &VBO);
//
//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//
//	unsigned int lightVAO;
//	glGenVertexArrays(1, &lightVAO);
//	glBindVertexArray(lightVAO);
//
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
//	glEnableVertexAttribArray(0);
//
//	// Unbind
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	glBindVertexArray(0);
//
//	// Load skybox textures
//	unsigned int skyboxTexture = GenerateCubemap(skyboxFaces);
//
//	// Skybox object
//	unsigned int skyboxVAO, skyboxVBO;
//	glGenVertexArrays(1, &skyboxVAO);
//	glGenBuffers(1, &skyboxVBO);
//	glBindVertexArray(skyboxVAO);
//	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
//	glEnableVertexAttribArray(0);
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
//
//	skyboxShader.Use();
//	skyboxShader.SetInt("skybox", 0);
//
//	gltfObject.LoadModel(gltfFile, mainShader);
//	//gltf2Object.LoadModel(gltfFile, mainShader);
//	//gltf3Object.LoadModel(gltfFile, mainShader);
//
//	FPSCounter fpsCounter;
//	glEnable(GL_DEPTH_TEST);
//	glDepthFunc(GL_LESS);
//
//	glEnable(GL_CULL_FACE);
//	glCullFace(GL_BACK);
//
//	glEnable(GL_STENCIL_TEST);
//
//	glDisable(GL_BLEND);
//
//	// Main loop
//	while (!glfwWindowShouldClose(window))
//	{
//		// Input process
//		ProcessInput(window);
//		//mainCamera.HandleCameraInput(window);
//
//		fpsCounter.Update();
//		std::stringstream windowTitle;
//		windowTitle << "Glimmer [ " << fpsCounter.GetFPS() << " fps ]";
//		glfwSetWindowTitle(window, windowTitle.str().c_str());
//
//		//ImGui_ImplOpenGL3_NewFrame();
//		//ImGui_ImplGlfw_NewFrame();
//		//ImGui::NewFrame();
//
//		//ImGui::ShowDemoWindow();
//
//		// Render
//		glClearColor(0.25f, 0.25f, 0.4f, 1.0f);
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
//		glStencilFunc(GL_EQUAL, 1, 0xFF);
//		glClearColor(0.25f, 0.25f, 0.8f, 1.0f);
//
//		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
//
//		glPolygonMode(GL_FRONT_AND_BACK, polyMode);
//
//		glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 100.0f);
//		glm::mat4 view = mainCamera.Update();
//		glm::mat4 model = glm::mat4(1.0f);
//
//		glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Draw to framebuffer
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//		glEnable(GL_DEPTH_TEST);
//
//		// gLTF object
//		mainShader.Use();
//		glStencilFunc(GL_ALWAYS, 1, 0xFF); // All fragments pass stencil test
//		glStencilMask(0xFF); // Enable writing to stencil buffer
//
//		mainShader.SetMatrix4("projection", projection);
//		mainShader.SetMatrix4("view", view);
//
//		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
//		mainShader.SetMatrix4("model", model);
//
//		gltfObject.DrawModel();
//
//
//		//model = glm::translate(model, glm::vec3(2.5f, 0.0f, 0.0f));
//		//mainShader.SetMatrix4("model", model);
//		//gltf2Object.DrawModel();
//
//		//model = glm::translate(model, glm::vec3(-5.0f, 0.0f, 0.0f));
//		//mainShader.SetMatrix4("model", model);
//		//gltf3Object.DrawModel();
//
//
//
//		//DrawModel(exModel);
//
//		//DrawModel(vertElementbuffers, exModel);
//
//		//// gLTF object -- Scaled
//		//glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
//		//glStencilMask(0x00);
//		//glDisable(GL_DEPTH_TEST);
//
//		//outlineShader.Use();
//
//		//outlineShader.SetMatrix4("projection", projection);
//		//outlineShader.SetMatrix4("view", view);
//
//		////glm::mat4 model = glm::mat4(1.0f);
//		//model = glm::scale(model, glm::vec3(1.1, 1.1, 1.1));
//		//outlineShader.SetMatrix4("model", model);
//
//		//DrawModel(vertElementbuffers, exModel);
//
//		//if (!asset.scenes.empty() && sceneIndex < asset.scenes.size())
//		//{
//		//	fastgltf::iterateSceneNodes(asset, sceneIndex, fastgltf::math::fmat4x4(), [&](fastgltf::Node& node, fastgltf::math::fmat4x4 matrix)
//		//		{
//		//			if (node.meshIndex.has_value()) 
//		//				DrawGLTFMesh(&viewer, *node.meshIndex, matrix);
//		//		});
//		//}
//
//		//glStencilMask(0xFF);
//		//glStencilFunc(GL_ALWAYS, 1, 0xFF);
//		//glEnable(GL_DEPTH_TEST);
//
//		// Light object
//		lightSourceShader.Use();
//		lightSourceShader.SetMatrix4("projection", projection);
//		lightSourceShader.SetMatrix4("view", view);
//
//		glBindVertexArray(lightVAO);
//
//		//TODO: Should eventually be changed to allow adding/ removing pointlights, but this should be simple to change		
//
//		model = glm::translate(model, lightPositions[0]);
//		model = glm::scale(model, glm::vec3(0.3f));
//		lightSourceShader.SetMatrix4("model", model);
//
//		lightPositions[0].x = sin((float)glfwGetTime() + 6) * staticLightPositions[0].x * 2.5;
//		lightPositions[0].y = sin((float)glfwGetTime() + 4) * staticLightPositions[0].y * 6;
//		lightPositions[0].z = sin((float)glfwGetTime() + 2);
//
//		glDisable(GL_CULL_FACE);
//		glDrawArrays(GL_TRIANGLES, 0, 36);
//		glEnable(GL_CULL_FACE);
//
//		// Skybox - Drawn last
//		glDepthFunc(GL_LEQUAL);
//		skyboxShader.Use();
//		view = glm::mat4(glm::mat3(mainCamera.GetViewMatrix()));
//
//		skyboxShader.SetMatrix4("view", view);
//		skyboxShader.SetMatrix4("projection", projection);
//
//		// Draw skybox cube
//		glBindVertexArray(skyboxVAO);
//		glActiveTexture(GL_TEXTURE5);
//		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
//		glDrawArrays(GL_TRIANGLES, 0, 36);
//		glBindVertexArray(0);
//		glDepthFunc(GL_LESS);
//
//		glBindFramebuffer(GL_FRAMEBUFFER, 0); // Revert to default framebuffer
//
//		//Render frame buffer
//		framebufferShader.Use();
//		glBindVertexArray(screenQuadVAO);
//		glDisable(GL_DEPTH_TEST);
//		glActiveTexture(GL_TEXTURE5);
//		glBindTexture(GL_TEXTURE_2D, textureCBuffer);
//		glDrawArrays(GL_TRIANGLES, 0, 6);
//
//		// Start ImGui frame
//		ImGui_ImplOpenGL3_NewFrame();
//		ImGui_ImplGlfw_NewFrame();
//		ImGui::NewFrame();
//
//		static ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking;
//		ImGui::Begin("Viewport", (bool*)false, flags); // TODO: Recreate on resize
//		ImGui::SetWindowSize(ImVec2(854, 480));
//
//		ImVec2 viewportPos = ImGui::GetCursorScreenPos();
//		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
//
//		// TODO Should use quaternions for camera hhhhhhh
//		static bool isWarping = false;
//		if (ImGui::IsWindowHovered())
//		{
//			if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
//			{
//				double currentMouseX, currentMouseY;
//				glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
//
//				if (firstMouseInput)
//				{
//					lastMouseX = currentMouseX;
//					lastMouseY = currentMouseY;
//					firstMouseInput = false;
//				}
//
//				if (!isWarping)
//				{
//					float xOffset = static_cast<float>(currentMouseX - lastMouseX);
//					float yOffset = static_cast<float>(lastMouseY - currentMouseY);
//
//					const float speed = 0.25f;
//					xOffset *= speed;
//					yOffset *= speed;
//
//
//					yaw += xOffset;
//					pitch += yOffset;
//
//					pitch = glm::clamp(pitch, -89.0f, 89.0f);
//
//					if (pitch > 89.0f) pitch = 89.0f;
//					else if (pitch < -89.0f) pitch = -89.0f;
//
//					glm::vec3 direction;
//					direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
//					direction.y = sin(glm::radians(pitch));
//					direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
//					mainCamera.SetFront(glm::normalize(direction));
//				}
//
//				isWarping = false;
//
//				lastMouseX = currentMouseX;
//				lastMouseY = currentMouseY;
//
//				// Handle edge wrapping
//				if (currentMouseX <= viewportPos.x)
//				{
//					glfwSetCursorPos(window, viewportPos.x + viewportSize.x - 1, currentMouseY);
//					lastMouseX = viewportPos.x + viewportSize.x - 1;
//					isWarping = true;
//				}
//				else if (currentMouseX >= viewportPos.x + viewportSize.x - 1)
//				{
//					glfwSetCursorPos(window, viewportPos.x + 1, currentMouseY);
//					lastMouseX = viewportPos.x + 1;
//					isWarping = true;
//				}
//
//				if (currentMouseY <= viewportPos.y)
//				{
//					glfwSetCursorPos(window, currentMouseX, viewportPos.y + viewportSize.y - 1);
//					lastMouseY = viewportPos.y + viewportSize.y - 1;
//					isWarping = true;
//				}
//				else if (currentMouseY >= viewportPos.y + viewportSize.y - 1)
//				{
//					glfwSetCursorPos(window, currentMouseX, viewportPos.y + 1);
//					lastMouseY = viewportPos.y + 1;
//					isWarping = true;
//				}
//			}
//			else
//			{
//				firstMouseInput = true;
//			}
//
//			//ImGui::SetMouseCursor(ImGuiMouseCursor_None);
//
//			const float cameraSpeed = 2.5f * io.DeltaTime;
//
//			if (ImGui::IsKeyDown(ImGuiKey_W) || (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS))
//			{
//				mainCamera.cameraPos += cameraSpeed * mainCamera.cameraFront;
//			}
//			if (ImGui::IsKeyDown(ImGuiKey_S) || (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS))
//			{
//				mainCamera.cameraPos -= cameraSpeed * mainCamera.cameraFront;
//			}
//			if (ImGui::IsKeyDown(ImGuiKey_A) || (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS))
//			{
//				mainCamera.cameraPos -= glm::normalize(glm::cross(mainCamera.cameraFront, mainCamera.cameraUp)) * cameraSpeed;
//			}
//			if (ImGui::IsKeyDown(ImGuiKey_D) || (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS))
//			{
//				mainCamera.cameraPos += glm::normalize(glm::cross(mainCamera.cameraFront, mainCamera.cameraUp)) * cameraSpeed;
//			}
//		}
//		else
//		{
//			firstMouseInput = true;
//		}
//
//
//		ImGui::Image((ImTextureID)(intptr_t)textureCBuffer, viewportSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
//
//		ImGui::End();
//
//		// Render ImGui
//		ImGui::Render();
//		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//
//		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
//		{
//			GLFWwindow* glfwCurrentContext = glfwGetCurrentContext();
//			ImGui::UpdatePlatformWindows();
//			ImGui::RenderPlatformWindowsDefault();
//			glfwMakeContextCurrent(glfwCurrentContext);
//		}
//
//		// Call events + swap buffers
//		glfwSwapInterval(0);
//		glfwSwapBuffers(window);
//		glfwPollEvents();
//	}
//
//	// Deallocate
//	//glDeleteVertexArrays(1, &vertElementbuffers.first);
//	glDeleteVertexArrays(1, &skyboxVAO);
//	glDeleteVertexArrays(1, &screenQuadVAO);
//	glDeleteBuffers(1, &skyboxVBO);
//	glDeleteBuffers(1, &screenQuadVBO);
//	glDeleteRenderbuffers(1, &rbo);
//	glDeleteFramebuffers(1, &fbo);
//	glDeleteVertexArrays(1, &lightVAO);
//	glDeleteBuffers(1, &VBO);
//
//	ImGui_ImplOpenGL3_Shutdown();
//	ImGui_ImplGlfw_Shutdown();
//	ImGui::DestroyContext();
//
//	for (auto& mesh : viewer.meshes)
//	{
//		glDeleteBuffers(1, &mesh.drawBuffer);
//
//		for (auto& primitive : mesh.primitives)
//		{
//			glDeleteVertexArrays(1, &primitive.vertexArray);
//			glDeleteBuffers(1, &primitive.indexBuffer);
//			glDeleteBuffers(1, &primitive.vertexBuffer);
//		}
//	}
//
//	//CleanupBuffers(vertElementbuffers, exModel);
//	mainShader.Delete();
//
//	glfwDestroyWindow(window);
//	glfwTerminate();
//	return 0;
//}
//
//void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
//{
//	glViewport(0, 0, width, height);
//}
//
//void ProcessInput(GLFWwindow* window)
//{
//	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//	{
//		glfwSetWindowShouldClose(window, true);
//	}
//
//}
//
//void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods)
//{
//	if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
//	{
//		polyMode = polyMode == GL_FILL ? GL_LINE : GL_FILL;
//	}
//}
//
//// Pretty rough function, but for now it's fine
//unsigned int LoadTexture(char const* path)
//{
//	unsigned int textureID;
//	glGenTextures(1, &textureID);
//
//	int width, height, nComponents;
//	unsigned char* data = stbi_load(path, &width, &height, &nComponents, 0);
//
//	if (data)
//	{
//		GLenum format{};
//		if (nComponents == 1)
//		{
//			format = GL_RED;
//		}
//		else if (nComponents == 3)
//		{
//			format = GL_RGB;
//		}
//		else if (nComponents == 4)
//		{
//			format = GL_RGBA;
//		}
//
//		glBindTexture(GL_TEXTURE_2D, textureID);
//		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
//		glGenerateMipmap(GL_TEXTURE_2D);
//
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//
//		stbi_image_free(data);
//	}
//	else
//	{
//		printf("Failed to load texture at path: %s!", path);
//		stbi_image_free(data);
//	}
//
//	return textureID;
//}
//
//void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
//{
//	fov -= (float)yoffset;
//	if (fov < 1.0f)
//		fov = 1.0f;
//	if (fov > 90.0f)
//		fov = 90.0f;
//}
//
//void MessageCallback(GLenum source, GLenum type, GLenum id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
//{
//	std::cerr << "OGL Debug\n";
//	printf("Source: 0x%x | Type: 0x%x | ID: %u\n Severity: 0x%x\n%s\n\n", source, type, id, severity, message);
//}
