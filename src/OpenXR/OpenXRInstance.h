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
	virtual ~OpenXRInstance() = default;

	virtual void CreateInstance() = 0;
	virtual void DestroyInstance() = 0;

	virtual void CreateDebugMessenger() = 0;
	virtual void DestroyDebugMessenger() = 0;

	virtual void GetInstanceProperties() = 0;
	virtual void CreateSystemID() = 0;
	virtual void CreateViewConfigurationViews() = 0;
	virtual void GetEnvironmentBlendModes() = 0;
};

