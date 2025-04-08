#pragma once

class OpenXRManager
{
public:
	virtual ~OpenXRManager() = default;

	virtual bool Init() = 0;
	virtual void Run() = 0;
	virtual void Destroy() = 0;

};

