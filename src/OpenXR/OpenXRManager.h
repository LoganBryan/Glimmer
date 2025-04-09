#pragma once
#include <string>

struct XRApplicationInfo
{
	std::string applicationName;
	std::string engineName;
	int applicationVersion;
	int engineVersion;
};

class OpenXRManager
{
public:
	virtual ~OpenXRManager() = default;

	virtual bool Init(XRApplicationInfo applicationInfo) = 0;
	virtual void Run() = 0;
	virtual void Destroy() = 0;

};

