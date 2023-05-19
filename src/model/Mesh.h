#pragma once
#include <vector>
#include "Material.h"
#include <glm/glm.hpp>
#include <string>

struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
	glm::vec3 Tangent;
};

enum class TextureType {
	DIFFUSE, SPECULAR, ROUGHNESS, METALLIC, NORMAL, AMBIENT_OCCLUSION
};

struct Texture {
	unsigned int id;
	std::string name;
	TextureType type;
};

class Mesh {
private:
	unsigned int VAO, VBO, EBO;
	void setupMesh();
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;
	Material material;
	Mesh(std::vector<Vertex>& v, std::vector<unsigned int>& i, std::vector<Texture>& t, Material& mat)
		: vertices(v), indices(i), textures(t), material(mat), VAO(0), VBO(0), EBO(0) {
		vertices = v;
		indices = i;
		textures = t;
		material = mat;
		setupMesh();
	};
	void deleteMesh();
	unsigned int getVao() const { return VAO; }
	unsigned int getIndicesSize() const { return indices.size(); }
	const Material& getMaterial() const { return material; }
	std::vector<Texture> getTexturesByType(TextureType tt) const;
};