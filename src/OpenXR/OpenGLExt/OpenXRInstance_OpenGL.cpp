#include "OpenXRInstance_OpenGL.h"

void OpenXRInstance_OpenGL::CreateInstance()
{
	// TODO: Allow changing the application and engine name/version
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
}

void OpenXRInstance_OpenGL::DestroyInstance()
{
	if (mXrInstance != XR_NULL_HANDLE)
	{
		OPENXR_CHECK(xrDestroyInstance(mXrInstance), "Failed to destroy instance!");
		mXrInstance = XR_NULL_HANDLE;
	}
}

void OpenXRInstance_OpenGL::CreateDebugMessenger()
{
	if (IsStringInVector(mActiveInstanceExtensions, XR_EXT_DEBUG_UTILS_EXTENSION_NAME))
		mDebugUtilsMessenger = CreateOpenXRDebugUtilsMessenger(mXrInstance);
}

void OpenXRInstance_OpenGL::DestroyDebugMessenger()
{
	if (mDebugUtilsMessenger != XR_NULL_HANDLE)
		DestroyOpenXRDebugUtilsMessenger(mXrInstance, mDebugUtilsMessenger);
}

void OpenXRInstance_OpenGL::GetInstanceProperties()
{
	XrInstanceProperties instanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
	OPENXR_CHECK(xrGetInstanceProperties(mXrInstance, &instanceProperties), "Failed to get InstanceProperties!");

	printf("OpenXR Runtime: %s", instanceProperties.runtimeName);
	std::cout << "-" << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << "." << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << "." << XR_VERSION_PATCH(instanceProperties.runtimeVersion) << std::endl;
}

void OpenXRInstance_OpenGL::CreateSystemID()
{
	// Get XrSystemId from instance and supplied XrFormFactor
	XrSystemGetInfo systemGI{ XR_TYPE_SYSTEM_GET_INFO };
	systemGI.formFactor = mFormFactor;
	OPENXR_CHECK(xrGetSystem(mXrInstance, &systemGI, &mSystemID), "Failed to get SystemID!");

	// Get system properties for general hardware and vendor info
	OPENXR_CHECK(xrGetSystemProperties(mXrInstance, mSystemID, &mSystemProperties), "Failed to get SystemProperties!");
}

void OpenXRInstance_OpenGL::CreateViewConfigurationViews()
{
	// Get view configuration types.
	uint32_t viewConfigCount = 0;
	OPENXR_CHECK(xrEnumerateViewConfigurations(mXrInstance, mSystemID, 0, &viewConfigCount, nullptr), "Failed to enumerate ViewConfigurations!");
	mViewConfigurations.resize(viewConfigCount);
	OPENXR_CHECK(xrEnumerateViewConfigurations(mXrInstance, mSystemID, viewConfigCount, &viewConfigCount, mViewConfigurations.data()), "Failed to enumerate ViewConfigurations!");

	// Select first application support view config type that is supported by hardware 
	for (const XrViewConfigurationType& viewConfig : mApplicationViewConfigurations)
	{
		if (std::find(mViewConfigurations.begin(), mViewConfigurations.end(), viewConfig) != mViewConfigurations.end())
		{
			mViewConfiguration = viewConfig;
			break;
		}
	}
	if (mViewConfiguration == XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM)
	{
		printf("Failed to find a view configuration type. Defauling to XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STERO.");
		mViewConfiguration = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	}

	// Get view config views.
	uint32_t viewConfigViewCount = 0;
	OPENXR_CHECK(xrEnumerateViewConfigurationViews(mXrInstance, mSystemID, mViewConfiguration, 0, &viewConfigViewCount, nullptr), "Failed to enumerate ViewConfiguration Views!");
	mViewConfigurationViews.resize(viewConfigViewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
	OPENXR_CHECK(xrEnumerateViewConfigurationViews(mXrInstance, mSystemID, mViewConfiguration, viewConfigViewCount, &viewConfigViewCount, mViewConfigurationViews.data()), "Failed to enumerate ViewConfiguration Views!");
}

void OpenXRInstance_OpenGL::GetEnvironmentBlendModes()
{
	// Get available blend modes.
	uint32_t environmentBlendModeCount = 0;
	OPENXR_CHECK(xrEnumerateEnvironmentBlendModes(mXrInstance, mSystemID, mViewConfiguration, 0, &environmentBlendModeCount, nullptr), "Failed to enumerate EnvironmentBlendModes!");
	mEnvironmentBlendModes.resize(environmentBlendModeCount);
	OPENXR_CHECK(xrEnumerateEnvironmentBlendModes(mXrInstance, mSystemID, mViewConfiguration, environmentBlendModeCount, &environmentBlendModeCount, mEnvironmentBlendModes.data()), "Failed to enumerate EnvironmentBlendModes!");

	// Select first supported blend mode by the hardware
	for (const XrEnvironmentBlendMode& environmentBlendMode : mApplicationEnvironmentBlendModes)
	{
		if (std::find(mEnvironmentBlendModes.begin(), mEnvironmentBlendModes.end(), environmentBlendMode) != mEnvironmentBlendModes.end())
		{
			mEnvironmentBlendMode = environmentBlendMode;
			break;
		}
	}
	if (mEnvironmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM)
	{
		printf("Failed to find compatible blend mode!\n Defauling to XR_ENVIRONMENT_BLEND_MODE_OPAQUE...");
		mEnvironmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	}
}
