#pragma once
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "Mesh.h"
#include <iostream>

struct TextureGammaContainer {
	std::string name;
	bool gammaCorrect;
};

class Model {
private:
	std::vector<Mesh> meshes;
	std::vector<Texture> modelTextures;
	void loadModel(const std::string& modelName, const std::string& extension);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene, unsigned int meshIndex);
	std::vector<Texture> loadMaterialTextures(const std::vector<TextureGammaContainer>& textureNames, TextureType textureType);
	unsigned int textureFromFile(const std::string& name, bool gammaCorrect);
	std::string name, extension;
public:
	Model(const std::string& modelName, const std::string& modelExtension) : name(modelName), extension(modelExtension) {
		loadModel(modelName, modelExtension);
	};
	~Model();
	void deleteModel();
	//Model& operator=(const Model& m) {
		// Should be defined.
		//return *this;
	//}
	const std::vector<Mesh>& getMeshes() const { return meshes; }
	std::vector<Mesh>& getMeshes() { return meshes; }
	std::string getName() const { return name; }
};

void loadAssetModel(const std::string&, const std::string&);
void deleteAssetModel(const std::string&);
void deleteAllAssetModels();
Model* getAssetModel(const std::string&);
std::map<std::string, Model>& getModels();