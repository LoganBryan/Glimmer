#include "OpenXRInstance_OpenGL.h"

void OpenXRInstance_OpenGL::CreateInstance(std::string applicationName, int applicationVersion, std::string engineName, int engineVersion)
{
	// TODO: Allow changing the application and engine name/version
	XrApplicationInfo appInfo = {};
	strncpy_s(appInfo.applicationName, applicationName.c_str(), XR_MAX_APPLICATION_NAME_SIZE);
	appInfo.applicationVersion = applicationVersion;
	strncpy_s(appInfo.engineName, engineName.c_str(), XR_MAX_ENGINE_NAME_SIZE);
	appInfo.engineVersion = engineVersion;
	appInfo.apiVersion = XR_API_VERSION_1_0;

	mInstanceExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
	mInstanceExtensions.push_back("XR_KHR_opengl_enable");

	mInstanceExtensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
	mInstanceExtensions.push_back(XR_EXT_HAND_INTERACTION_EXTENSION_NAME);

	// Get all API layers from OpenXR runtime
	uint32_t apiLayerCount = 0;
	std::vector<XrApiLayerProperties> apiLayerProps;
	OPENXR_CHECK(xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr), "Failed to enumerate ApiLayerProperties!");
	apiLayerProps.resize(apiLayerCount, { XR_TYPE_API_LAYER_PROPERTIES });
	OPENXR_CHECK(xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, apiLayerProps.data()), "Failed to enumerate ApiLayerProperties!");

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
			printf("Failed to find OpenXR instance extension %s!\n", requestedInstanceExt.c_str());
		}
	}

	XrInstanceCreateInfo instanceCI{ XR_TYPE_INSTANCE_CREATE_INFO };
	instanceCI.createFlags = 0;
	instanceCI.applicationInfo = appInfo;
	instanceCI.enabledApiLayerCount = 0;
	instanceCI.enabledExtensionCount = static_cast<uint32_t>(mActiveInstanceExtensions.size());
	instanceCI.enabledExtensionNames = mActiveInstanceExtensions.data();
	OPENXR_CHECK(xrCreateInstance(&instanceCI, &mXrInstance), "Failed to create instance!");

	OPENXR_CHECK(xrGetInstanceProcAddr(mXrInstance, "xrCreateHandTrackerEXT", (PFN_xrVoidFunction*)&xrCreateHandTrackerEXT), "Failed to get xrCreateHandTrackerEXT.");
	OPENXR_CHECK(xrGetInstanceProcAddr(mXrInstance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction*)&xrDestroyHandTrackerEXT), "Failed to get xrDestroyHandTrackerEXT.");
	OPENXR_CHECK(xrGetInstanceProcAddr(mXrInstance, "xrLocateHandJointsEXT", (PFN_xrVoidFunction*)&xrLocateHandJointsEXT), "Failed to get xrLocateHandJointsEXT.");
}