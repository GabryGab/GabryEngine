#pragma once
#include <glm/glm.hpp>

struct BloomMip {
	glm::vec2 size;
	glm::ivec2 intSize;
	unsigned int texture;
};