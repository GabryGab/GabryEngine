#pragma once

namespace Bloom {
	void initialize();

	// Safe to call even if it is not initialized.
	void destroy();

	void bloomPass(unsigned int srcTexture);

	unsigned int getBlurredTexture();

	void setUsePrefilter(bool);
	bool getUsePrefilter();

	void setBloomBias(float);
	float getBloomBias();

	void setPrefilterThreshold(float);
	float getPrefilterThreshold();
}