#pragma once
#include <glm/glm.hpp>

struct Material {
	// Phong.
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;

	// PBR.
	float roughness, metallic;
};