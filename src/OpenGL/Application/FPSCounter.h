#pragma once
#include <chrono>

// This class might be renamed in the future - it should also be a singleton as it'll be accessed globally for the application's lifetime (and we don't want to call update in every class)
class FPSCounter
{
public:
	inline FPSCounter() : previous(std::chrono::system_clock::now()), frameCount(0), fps(0), delta{0.0f} {}

	void Update();
	inline double GetFPS() const { return fps; }
	inline double GetDelta() const { return delta.count(); }

private:
	std::chrono::system_clock::time_point previous;
	std::chrono::duration<double> delta;
	int frameCount;
	double fps;
};

