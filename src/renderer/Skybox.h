#pragma once
#include <glm/glm.hpp>

namespace Skybox {
	void initialize();
	void terminate();
	void draw(const glm::mat4& projection, const glm::mat4& view);
	bool getUseSkybox();
	void setUseSkybox(bool);
}