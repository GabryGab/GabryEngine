#pragma once

// GaussianBlur creates framebuffers for each blur, as it is intended to use on single blur passes.
// If it is ever needed to use on contiguous frame, make it that it doesn't create buffers unless needed.
// Examples of needing to recreate buffers is resizing of window. 

namespace GaussianBlur {
	void initialize();
	void blur(unsigned int, unsigned int, int, int);
	void terminate();

	void setStrength(float);
	float getStrength();

	void setIterations(int);
	int getIterations();

	void setRadius(int);
	int getRadius();

	void setQuality(float);
	float getQuality();
}