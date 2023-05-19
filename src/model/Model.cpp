#include "Model.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <fstream>
#include <iostream>
#include "ProjectDirectory.h"
#include "ModelLoader.h"
#include <vector>
#include <map>
#include "Mesh.h"
// Assimp.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Name_Model map to store all assetModels.
// To actually use an asset must instantiate a ModelInstance using an asset model.
static std::map<std::string, Model> assetModels;

void loadAssetModel(const std::string& modelName, const std::string& extension) {

	Model assetModel(modelName, extension);

	assetModels.insert({ modelName, assetModel });
}

void Model::loadModel(const std::string& modelName, const std::string& extension) {
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(project_directory + "\\assets\\models\\" + modelName + "\\" + (modelName + "." + extension), 
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes | aiProcess_CalcTangentSpace);
	if (!scene || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene) {
	// Process all the node's meshes (if any).
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene, node->mMeshes[i]));
	}
	// Then do the same for each of its children.
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}

// Read texture properties file and update vectors passed.
typedef std::vector<TextureGammaContainer> tgContainer;
static void createTextureVectorsFromPropertiesFile(tgContainer& diff, tgContainer& metal, tgContainer& rough, tgContainer& norm, unsigned int meshIndex, const std::string& name) {
	
	// File is opened for each mesh.
	// This is inefficient so should be changed.
	// Possibly make this as a single read for everything.
	try {
		std::ifstream texturePropertiesFile(project_directory + "\\assets\\models\\" + name + "\\" + "texture_properties.txt");
		// Build texture vectors.
		std::string line;
		while (std::getline(texturePropertiesFile, line)) {

			// Skip line if empty
			if (std::strcmp(line.c_str(), "") == 0)
				continue;

			// Get index of mesh of our file line.
			// If it's not the same index skip the line.
			int spaceIndex = line.find(' ');
			std::string subline = line.substr(0, spaceIndex);
			line = line.substr(spaceIndex + 1);
			unsigned int lineMeshIndex = std::stoi(subline);

			if (lineMeshIndex == meshIndex) {

				// Texture type string.
				spaceIndex = line.find(' ');
				std::string type = line.substr(0, spaceIndex);

				// Texture file name.
				line = line.substr(spaceIndex + 1);
				spaceIndex = line.find(' ');
				std::string textureName = line;
				// If there is another space it means gamma is specified.
				// In this case modify textuer file name so it doens't include "gamma".
				if (spaceIndex != std::string::npos)
					textureName = line.substr(0, spaceIndex);

				TextureGammaContainer container;
				container.gammaCorrect = false;

				// Read if there is gamma.
				if (spaceIndex != std::string::npos) {

					std::string applyGamma = line.substr(spaceIndex + 1);
					if (std::strcmp(applyGamma.c_str(), "gamma") == 0)
						container.gammaCorrect = true;
				}

				if (std::strcmp(type.c_str(), "diffuse") == 0) {
					container.name = textureName;
					diff.push_back(container);
				}
				else if (std::strcmp(type.c_str(), "metallic") == 0) {
					container.name = textureName;
					metal.push_back(container);
				}
				else if (std::strcmp(type.c_str(), "roughness") == 0) {
					container.name = textureName;
					rough.push_back(container);
				}
				else if (std::strcmp(type.c_str(), "normals") == 0) {
					container.name = textureName;
					norm.push_back(container);
				}
			}
		}
		texturePropertiesFile.close();
	}
	catch (...) {
		std::cout << "Error while opening textures properties file" << std::endl;
	}
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene, unsigned int meshIndex) {

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	// Vertices, normals, texture coordinates, tangents.
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		Vertex ver;
		ver.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		ver.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		ver.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
		if (mesh->mTextureCoords[0]) {
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			ver.TexCoords = vec;
		}
		else
			ver.TexCoords = glm::vec2(0.0f, 0.0f);
		vertices.push_back(ver);
	}

	for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
		aiFace face = mesh->mFaces[t];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	// Material (including textures).
	Material mat;
	mat.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	mat.diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
	mat.specular = glm::vec3(0.7f, 0.7f, 0.7f);
	mat.shininess = 100;
	mat.roughness = 0.4f;
	mat.metallic = 0.0f;
	if (mesh->mMaterialIndex >= 0) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		aiColor4D ambient;
		if (aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &ambient) == AI_SUCCESS)
			mat.ambient = glm::vec3(ambient.r, ambient.g, ambient.b);
		aiColor4D diffuse;
		if (aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse) == AI_SUCCESS)
			mat.diffuse = glm::vec3(diffuse.r, diffuse.g, diffuse.b);
		aiColor4D specular;
		if (aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &specular) == AI_SUCCESS)
			mat.specular = glm::vec3(specular.r, specular.g, specular.b);
		ai_real shininess;
		if (aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shininess) == AI_SUCCESS)
			mat.shininess = shininess;
		ai_real roughness;
		if (aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &roughness) == AI_SUCCESS)
			mat.roughness = roughness;
		ai_real metallic;
		if (aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &metallic) == AI_SUCCESS)
			mat.metallic = metallic;

		std::vector<TextureGammaContainer> diffuseTextures, metallicTextures, roughnessTextures, normalsTextures;
		
		createTextureVectorsFromPropertiesFile(diffuseTextures, metallicTextures, roughnessTextures, normalsTextures, meshIndex, name);

		// Textures.
		std::vector<Texture> diffuseMaps = loadMaterialTextures(diffuseTextures, TextureType::DIFFUSE);
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

		std::vector<Texture> roughnessMaps = loadMaterialTextures(roughnessTextures, TextureType::ROUGHNESS);
		textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());

		std::vector<Texture> metallicMaps = loadMaterialTextures(metallicTextures, TextureType::METALLIC);
		textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

		std::vector<Texture> normalsMaps = loadMaterialTextures(normalsTextures, TextureType::NORMAL);
		textures.insert(textures.end(), normalsMaps.begin(), normalsMaps.end());
	}
	return Mesh(vertices, indices, textures, mat);
}

std::vector<Texture> Model::loadMaterialTextures(const std::vector<TextureGammaContainer>& textureNames, TextureType textureType) {

	std::vector<Texture> textures;

	for (unsigned int i = 0; i < textureNames.size(); i++)
	{
		std::string textureName = textureNames[i].name;
		bool skip = false;
		for (int j = 0; j < modelTextures.size(); ++j) {
			if (std::strcmp(modelTextures[j].name.data(), textureName.data()) == 0) {
				textures.push_back(modelTextures[j]);
				skip = true;
				std::cout << "skip" << std::endl;
				break;
			}
		}
		if (!skip) {
			Texture texture;
			texture.id = textureFromFile(textureName, textureNames[i].gammaCorrect);
			texture.type = textureType;
			texture.name = textureName;
			textures.push_back(texture);
		}
	}
	return textures;
}

unsigned int Model::textureFromFile(const std::string& fileName, bool gammaCorrect) {

	std::string completePath = project_directory + "\\assets\\models\\" + name + "\\" + fileName;

	// std::cout << fileName << std::endl;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(completePath.c_str(), &width, &height, &nrComponents, 0);

	if (data)
	{
		GLenum format = GL_RGBA;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Might want to change this.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << completePath << std::endl;
		stbi_image_free(data);
		return 0;
	}

	return textureID;
}

// Return model in assetModels map.
// Returns const model so you can't modify it.
Model* getAssetModel(const std::string& name) {
	// Iterator to model we want.
	// If the model we are looking for isn't there find returns end iterator.
	const auto& it = assetModels.find(name);
	if (it != assetModels.end())
		return &it->second;

	// Load model if it was not found.
	std::string extension, lineString;
	try {
		std::ifstream modFile(project_directory + "\\assets\\models\\" + name + "\\model_properties.txt");

		while (std::getline(modFile, lineString)) {
			if (lineString.find("fileExtension") == 0)
				extension = lineString.substr(lineString.find(' ') + 1);
		}
	}
	catch (...) {
		std::cout << "Error while opening or reading model_properties.txt file" << std::endl;
	}
	
	loadAssetModel(name, extension);

	return &assetModels.at(name);
}

std::map<std::string, Model>& getModels() {
	return assetModels;
}

// Not to confuse with global delete model.
void Model::deleteModel() {
	// Delete all meshes.
	for (auto& m : meshes) {
		m.deleteMesh();
	}
}

// We don't check for whether name exists or not.
// If it doesn't, program probably crashes.
// Also if you delete a model that is used
// in a ModelInstance it may cause problems. 
void deleteAssetModel(const std::string& name) {
	Model& m = assetModels.at(name);
	m.deleteModel();
	assetModels.erase(name);
}

// Deletes all asset models.
// To use at the end of the program.
void deleteAllAssetModels() {
	for (auto m : assetModels) {
		deleteAssetModel(m.first);
	}
}

Model::~Model() {
	
}