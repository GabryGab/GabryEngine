#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace ModelLoader {

	struct Material {
		std::string textureName;
		bool hasTexture;
		glm::vec3 ambient, diffuse, specular;
		float shininess;
	};

	void loadModel(const std::string&, std::vector<float>&, std::vector<int>&);

	std::vector<Material> loadMaterial(const std::string&);
}