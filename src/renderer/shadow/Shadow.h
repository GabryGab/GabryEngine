#pragma once
#include "shader/Shader.h"
#include <glm/glm.hpp>

namespace Shadow {
	void initialize();
	void shadowPass(const glm::mat4& proj, const glm::mat4& view);
	unsigned int getTextureId();
	Shader& getShader();
	glm::mat4& getLightSpaceMatrix();
	void setShadowParametersForRendering(const Shader& program);
	void terminate();
	bool getUsePcf();
	bool getUsePoissonPcf();
	bool getUseShadows();
	void setUsePcf(bool);
	void setUsePoissonPcf(bool);
	void setUseShadows(bool);
	int getPoissonPcfSamplesNumber();
	void setPoissonPcfSamplesNumber(int);
	float getPoissonPcfDiameter();
	void setPoissonPcfDiameter(float);
	float getCsmPlanesDistanceInterpolationFactor();
	void setCsmPlanesDistanceInterpolationFactor(float);
	float getCsmBlendingOffset();
	void setCsmBlendingOffset(float);
	float getShadowBiasMultiplier();
	void setShadowBiasMultiplier(float);
	float getShadowBiasMinimum();
	void setShadowBiasMinimum(float);
}