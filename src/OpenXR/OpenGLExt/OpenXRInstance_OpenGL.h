#pragma once
#include "OpenXR/OpenXRInstance.h"

class OpenXRInstance_OpenGL : public OpenXRInstance
{
public:


public:
	OpenXRInstance_OpenGL() {};
	inline ~OpenXRInstance_OpenGL() { DestroyInstance(); }

	void CreateInstance(std::string applicationName, int applicationVersion, std::string engineName, int engineVersion) override;
public:
	inline XrInstance GetInstance() const { return mXrInstance; }
	inline XrSystemId GetSystemID() const { return mSystemID; }
	inline const std::vector<XrViewConfigurationView>& GetViewConfigurationViews() const { return mViewConfigurationViews; }
	inline const std::vector<const char*>& GetActiveInstanceExtensions() const { return mActiveInstanceExtensions; }
	inline XrViewConfigurationType GetViewConfiguration() const { return mViewConfiguration; }
};

