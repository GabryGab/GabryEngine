#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace PoissonDisk {
	std::vector<glm::vec2> generatePoissonDisk(int pcfSamples);
}