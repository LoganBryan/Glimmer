#include "FPSCounter.h"

void FPSCounter::Update()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	frameCount++;

	std::chrono::duration<double> delta = now - previous;

	if (delta.count() >= 1.0)
	{
		fps = frameCount / std::chrono::duration<double>(delta).count();

		frameCount = 0;
		previous = now;
	}
}
