#pragma once
#include <chrono>

class FPSCounter
{
public:
	inline FPSCounter() : previous(std::chrono::system_clock::now()), frameCount(0), fps(0) {}

	void Update();
	inline double GetFPS() const { return fps; }

private:
	std::chrono::system_clock::time_point previous;
	int frameCount;
	double fps;
};

