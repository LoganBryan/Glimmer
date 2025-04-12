#pragma once
#include <openxr/openxr.h>

#include <vector>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <cstring>

#include "OpenXRDebugUtils.h"

class OpenXRInstance
{
public:
	XrEnvironmentBlendMode mEnvironmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;
	XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties = { XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT };

	PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT = nullptr;
	PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT = nullptr;
	PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT = nullptr;

public:
	virtual ~OpenXRInstance() = default;

	virtual void CreateInstance(std::string applicationName, int applicationVersion, std::string engineName, int engineVersion) = 0;
	inline virtual void DestroyInstance()
	{
		if (mXrInstance != XR_NULL_HANDLE)
		{
			OPENXR_CHECK(xrDestroyInstance(mXrInstance), "Failed to destroy instance!");
			mXrInstance = XR_NULL_HANDLE;
		}
	}

	inline virtual void CreateDebugMessenger()
	{
		if (IsStringInVector(mActiveInstanceExtensions, XR_EXT_DEBUG_UTILS_EXTENSION_NAME))
			mDebugUtilsMessenger = CreateOpenXRDebugUtilsMessenger(mXrInstance);
	}
	virtual void DestroyDebugMessenger()
	{
		if (mDebugUtilsMessenger != XR_NULL_HANDLE)
			DestroyOpenXRDebugUtilsMessenger(mXrInstance, mDebugUtilsMessenger);
	}

	inline virtual void GetInstanceProperties()
	{
		XrInstanceProperties instanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };

		OPENXR_CHECK(xrGetInstanceProperties(mXrInstance, &instanceProperties), "Failed to get InstanceProperties!");

		printf("OpenXR Runtime: %s", instanceProperties.runtimeName);
		std::cout << "-" << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << "." << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << "." << XR_VERSION_PATCH(instanceProperties.runtimeVersion) << std::endl;
	}
	inline virtual void CreateSystemID()
	{
		// Get XrSystemId from instance and supplied XrFormFactor
		XrSystemGetInfo systemGI{ XR_TYPE_SYSTEM_GET_INFO };
		systemGI.formFactor = mFormFactor;
		OPENXR_CHECK(xrGetSystem(mXrInstance, &systemGI, &mSystemID), "Failed to get SystemID!");

		mSystemProperties.next = &handTrackingSystemProperties;

		// Get system properties for general hardware and vendor info
		OPENXR_CHECK(xrGetSystemProperties(mXrInstance, mSystemID, &mSystemProperties), "Failed to get SystemProperties!");
	}
	inline virtual XrPath CreateXrPath(const char* pathString)
	{
		XrPath xrPath;
		OPENXR_CHECK(xrStringToPath(mXrInstance, pathString, &xrPath), "Failed to create XrPath from string!");
		return xrPath;
	}
	inline virtual std::string FromXrPath(XrPath path)
	{
		uint32_t strl;
		char text[XR_MAX_PATH_LENGTH];
		XrResult result;
		result = xrPathToString(mXrInstance, path, XR_MAX_PATH_LENGTH, &strl, text);
		std::string str;
		if (result == XR_SUCCESS)
		{
			str = text;
		}
		else
		{
			OPENXR_CHECK(result, "Failed to retrieve path!");
		}

		return str;
	}

	inline virtual void CreateViewConfigurationViews()
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
	inline virtual void GetEnvironmentBlendModes()
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

protected:
	XrInstance mXrInstance = XR_NULL_HANDLE;

	std::vector<std::string> mInstanceExtensions;
	std::vector<const char*> mActiveInstanceExtensions;
	XrDebugUtilsMessengerEXT mDebugUtilsMessenger = XR_NULL_HANDLE;

	XrFormFactor mFormFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	XrSystemId mSystemID = {};
	XrSystemProperties mSystemProperties{ XR_TYPE_SYSTEM_PROPERTIES };

	std::vector<XrViewConfigurationType> mApplicationViewConfigurations{ XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO };
	std::vector<XrViewConfigurationType> mViewConfigurations;
	XrViewConfigurationType mViewConfiguration = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;
	std::vector<XrViewConfigurationView> mViewConfigurationViews;

	std::vector<XrEnvironmentBlendMode> mApplicationEnvironmentBlendModes{ XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE };
	std::vector<XrEnvironmentBlendMode> mEnvironmentBlendModes;

};

