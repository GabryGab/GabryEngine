#include "ModelLoader.h"
#include <map>
#include "Model.h"
#include <iostream>
#include <fstream>
#include "ProjectDirectory.h"
#include <glm/glm.hpp>



/*

ModelLoader.cpp must not do OpenGL operations.
Its sole purpose is to build meshes and get material values.

*/



// Triple to represent already existing vertices.
static struct Triple {
	int x, y, z;
	Triple() : x(0), y(0), z(0) {}
	Triple(int a, int b, int c) : x(a), y(b), z(c) {}
	Triple& operator=(const Triple& t) {
		x = t.x;
		y = t.y;
		z = t.z;
		return *this;
	}
	bool operator<(const Triple& t) const {
		if (x < t.x)
			return true;
		else if (x == t.x) {
			if (y < t.y)
				return true;
			else if (y == t.y) {
				if (z < t.z)
					return true;
				else
					return false;
			}
			else return false;
		}
		else
			return false;
	}
};

// Function to process each line.
// Vertex (v), texture (vt), normal (vn), face (f).
static void vertexProcessing(const std::string&, std::vector<glm::vec3>&);
static void textureProcessing(const std::string&, std::vector<glm::vec2>&);
static void normalProcessing(const std::string&, std::vector<glm::vec3>&);
static void faceProcessing(const std::string&, std::vector<int>&, std::vector<float>&, std::map<Triple, int>&,
	std::vector<glm::vec3>&, std::vector<glm::vec2>&, std::vector<glm::vec3>&);

static std::string materialTextureProcessing(const std::string&);

static void printVec2(glm::vec2);
static void printVec3(glm::vec3);

// Process a standard line that is formed like this:
// <name> <value> <value> <value>
// Example:
// Kd 0.800000 0.800000 0.800000
static glm::vec3 processStandardVec3(const std::string&);

// Process a standard line that is formed like this:
// <name> <value>
// Example:
// Ns 0.800000
static float processStandardFloat(const std::string&);

void ModelLoader::loadModel(const std::string& name, std::vector<float>& finalValues, std::vector<int>& indexValues) {
	// Load model and catch eventual errors.
	try {
		// Input file stream of our .obj file.
		// Obj file name is derived from folder.
		std::string lineString;
		std::ifstream objFile(project_directory + "\\assets\\models\\" + name + "\\" + (name + ".obj"));

		// Vectors to store values.
		std::vector<glm::vec3> vertexValues, normalValues;
		std::vector<glm::vec2> textureValues;
		std::map<Triple, int> alreadyExistingValues;

		// Per line processing.
		while (std::getline(objFile, lineString)) {
			if (lineString.find("v ") == 0)
				vertexProcessing(lineString, vertexValues);
			if (lineString.find("vt ") == 0)
				textureProcessing(lineString, textureValues);
			if (lineString.find("vn ") == 0)
				normalProcessing(lineString, normalValues);
			if (lineString.find("f ") == 0)
				faceProcessing(lineString, indexValues, finalValues, alreadyExistingValues,
					vertexValues, textureValues, normalValues);
		}
		objFile.close();
	}
	catch (...) {
		std::cout << "Error loading model" << std::endl;
	}
}

static void vertexProcessing(const std::string& line, std::vector<glm::vec3>& vec) {

	float x, y, z;

	std::string subLine = line.substr(line.find(' ') + 1);

	std::size_t index = subLine.find(' ');

	x = std::stof(subLine.substr(0, index));

	index = subLine.find(' ');

	subLine = subLine.substr(index + 1);
	index = subLine.find(' ');
	y = std::stof(subLine.substr(0, index));

	subLine = subLine.substr(index + 1);
	index = subLine.find(' ');
	z = std::stof(subLine.substr(0, index));

	vec.push_back(glm::vec3(x, y, z));
}

static void textureProcessing(const std::string& line, std::vector<glm::vec2>& vec) {

	float x, y;

	std::string subLine = line.substr(line.find(' ') + 1);

	std::size_t index = subLine.find(' ');

	x = std::stof(subLine.substr(0, index));

	index = subLine.find(' ');

	subLine = subLine.substr(index + 1);
	index = subLine.find(' ');
	y = std::stof(subLine.substr(0, index));

	vec.push_back(glm::vec2(x, y));
}

static void normalProcessing(const std::string& line, std::vector<glm::vec3>& vec) {

	float x, y, z;

	std::string subLine = line.substr(line.find(' ') + 1);

	std::size_t index = subLine.find(' ');

	x = std::stof(subLine.substr(0, index));

	index = subLine.find(' ');

	subLine = subLine.substr(index + 1);
	index = subLine.find(' ');
	y = std::stof(subLine.substr(0, index));

	subLine = subLine.substr(index + 1);
	index = subLine.find(' ');
	z = std::stof(subLine.substr(0, index));

	vec.push_back(glm::vec3(x, y, z));
}

static void faceProcessing(const std::string& line, std::vector<int>& indexValues, std::vector<float>& finalValues,
	std::map<Triple, int>& alreadyExistingValues, std::vector<glm::vec3>& vertex,
	std::vector<glm::vec2>& texture, std::vector<glm::vec3>& normal) {

	int x, y, z;

	std::string subLine = line;

	std::size_t index;

	for (int i = 0; i < 3; i++) {
		subLine = subLine.substr(subLine.find(' ') + 1);

		std::string tempLine = subLine;
		index = tempLine.find('/');
		x = std::stoi(tempLine.substr(0, index));
		tempLine = tempLine.substr(index + 1);

		index = tempLine.find('/');
		y = std::stoi(tempLine.substr(0, index));
		tempLine = tempLine.substr(index + 1);

		index = tempLine.find(' ');
		z = std::stoi(tempLine.substr(0, index));

		auto triple = alreadyExistingValues.find(Triple(x, y, z));
		if (triple != alreadyExistingValues.end()) {
			// If triple already exists only add index.
			indexValues.push_back(triple->second);
		}
		else {
			// If triple doesn't exist add index and elements to finalValues.
			int i = alreadyExistingValues.size();
			alreadyExistingValues.insert({ Triple(x,y,z), i });
			indexValues.push_back(i);
			glm::vec2 tex = texture[y - 1]; 
			glm::vec3 ver = vertex[x - 1], nor = normal[z - 1];
			// Order is: vertex, normal, texture.
			finalValues.push_back(ver.x); finalValues.push_back(ver.y); finalValues.push_back(ver.z);
			finalValues.push_back(nor.x); finalValues.push_back(nor.y); finalValues.push_back(nor.z);
			finalValues.push_back(tex.x); finalValues.push_back(tex.y);
			// std::cout << ver.x << " " << ver.y << " " << ver.z << " " << nor.x << " " << nor.y << " " << nor.z << " " << tex.x << " " << tex.y << std::endl;
		}
	}
}

static void printVec3(glm::vec3 v) {
	std::cout << v.x << " " << v.y << " " << v.z << std::endl;
}

static void printVec2(glm::vec2 v) {
	std::cout << v.x << " " << v.y << std::endl;
}

// Must be changed if mtl has more than one material.
// Currently works for one material only.
std::vector<ModelLoader::Material> ModelLoader::loadMaterial(const std::string& name) {
	// Load material and catch eventual errors.
	try {
		// Input file stream of our .mtl file.
		// Mtl file name is derived from folder.
		std::string lineString;
		std::ifstream objFile(project_directory + "\\assets\\models\\" + name + "\\" + (name + ".mtl"));

		Material mat;
		mat.hasTexture = false;
		std::vector<Material> returnVector;

		// Per line processing.
		while (std::getline(objFile, lineString)) {
			if (lineString.find("map_Kd") == 0) {
				mat.textureName = materialTextureProcessing(lineString);
				mat.hasTexture = true;
			}
			if (lineString.find("Ka") == 0) {
				mat.ambient = processStandardVec3(lineString);
			}
			if (lineString.find("Kd") == 0) {
				mat.diffuse = processStandardVec3(lineString);
			}
			if (lineString.find("Ks") == 0) {
				mat.specular = processStandardVec3(lineString);
			}
			if (lineString.find("Ni") == 0) {
				mat.shininess = processStandardFloat(lineString);
			}
		}
		objFile.close();

		returnVector.push_back(mat);
		return returnVector;
	}
	catch (...) {
		std::cout << "Error loading material" << std::endl;
	}
}

static std::string materialTextureProcessing(const std::string& line) {
	std::string subLine = line.substr(line.find(' ') + 1);
	return subLine;
}

static glm::vec3 processStandardVec3(const std::string& line) {
	float x, y, z;

	std::string subLine = line.substr(line.find(' ') + 1);

	std::size_t index = subLine.find(' ');

	x = std::stof(subLine.substr(0, index));

	index = subLine.find(' ');

	subLine = subLine.substr(index + 1);
	index = subLine.find(' ');
	y = std::stof(subLine.substr(0, index));

	subLine = subLine.substr(index + 1);
	index = subLine.find(' ');
	z = std::stof(subLine.substr(0, index));

	return glm::vec3(x, y, z);
}

static float processStandardFloat(const std::string& line) {
	float x;

	std::string subLine = line.substr(line.find(' ') + 1);

	std::size_t index = subLine.find(' ');

	x = std::stof(subLine.substr(0, index));

	return x;
}