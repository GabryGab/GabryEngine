#pragma once
#include "shader/Shader.h"

namespace PostProcessing {
	Shader& getProgram();
	void initializePostProcessing();
	void terminate();
	void createBuffers();
	void deleteBuffers();
	unsigned int getFramebufferId();
	void applyEffects();
	void draw();
	void setUseTonemapping(bool);
	void setUseGammacorrection(bool);
	void setExposure(float);
	void setGamma(float);
	void setUsePostProcessing(bool);
	void setUseBlackAndWhite(bool);
	void setUseGaussianBlur(bool);
	void setUseBloom(bool);
	bool getUseTonemapping();
	bool getUseGammacorrection();
	bool getUsePostProcessing();
	bool getUseBlackAndWhite();
	bool getUseGaussianBlur();
	bool getUseBloom();
	float getExposure();
	float getGamma();
}