#pragma once
#include "OpenXR/OpenXRInstance.h"

class OpenXRInstance_OpenGL : public OpenXRInstance
{
public:
	XrEnvironmentBlendMode mEnvironmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;

public:
	OpenXRInstance_OpenGL() {};
	inline ~OpenXRInstance_OpenGL() { DestroyInstance(); }

	void CreateInstance() override;
	void DestroyInstance() override;

	void CreateDebugMessenger() override;
	void DestroyDebugMessenger() override;

	void GetInstanceProperties() override;
	void CreateSystemID() override;
	void CreateViewConfigurationViews() override;
	void GetEnvironmentBlendModes() override;

public:
	inline XrInstance GetInstance() const { return mXrInstance; }
	inline XrSystemId GetSystemID() const { return mSystemID; }
	inline const std::vector<XrViewConfigurationView>& GetViewConfigurationViews() const { return mViewConfigurationViews; }
	inline const std::vector<const char*>& GetActiveInstanceExtensions() const { return mActiveInstanceExtensions; }
	inline XrViewConfigurationType GetViewConfiguration() const { return mViewConfiguration; }

private:
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

