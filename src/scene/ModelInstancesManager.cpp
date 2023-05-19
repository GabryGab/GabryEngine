#include "Scene.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "ProjectDirectory.h"

using json = nlohmann::json;

void ModelInstancesManager::initialize() {
	
	// First check if file exists. If it doesn't create it and skip reading step.
	try {
		std::ifstream filePath(project_directory + "\\assets\\scenes\\" + sceneName + "\\model_instances.json");
		if (!filePath) {
			filePath.close();
			std::fstream newFile;
			newFile.open(project_directory + "\\assets\\scenes\\" + sceneName + "\\model_instances.json", std::ios::out);
			newFile.close();
			return;
		}
		else {
			filePath.close();
		}
	}
	catch (...) {
		std::cout << "Error while checking existence/creating scene directory." << std::endl;
	}

	// Read ModelInstances and add them to modelInstances (instances to render).
	try {
		std::ifstream f(project_directory + "\\assets\\scenes\\" + sceneName + "\\model_instances.json");
		json data = json::parse(f);

		int modelInstancesNumber = data["modelInstances"].size();
		for (int i = 0; i < modelInstancesNumber; ++i) {
			float posX = data["modelInstances"][i]["position"][0], 
				posY = data["modelInstances"][i]["position"][1], posZ = data["modelInstances"][i]["position"][2];
			glm::vec3 scale(data["modelInstances"][i]["scale"][0], 
				data["modelInstances"][i]["scale"][1], data["modelInstances"][i]["scale"][2]);
			glm::vec3 rotation(data["modelInstances"][i]["rotation"][0], 
				data["modelInstances"][i]["rotation"][1], data["modelInstances"][i]["rotation"][2]);
			
			modelInstances.push_back(ModelInstance(posX, posY, posZ, rotation, scale, getAssetModel(data["modelInstances"][i]["modelName"])));
		}
		// Remember to close the file.
		f.close();
	}
	catch (...) {
		std::cout << "Error while loading scene model instances." << std::endl;
	}
}

void ModelInstancesManager::terminate() {
	// Unload all loaded models.
	// Could add a function to swap with another scene 
	// so that you don't have to reload already loaded models.
	deleteAllAssetModels();

	// Clear all model instances.
	modelInstances.clear();
}

void ModelInstancesManager::save() {
	std::ofstream outputFile;
	try {
		json sceneJson;
		// Create json object and dump as string (int in parethesis represents indentation spaces).
		for (int i = 0; i < modelInstances.size(); ++i) {
			const ModelInstance& mi = modelInstances[i];
			sceneJson["modelInstances"][i]["position"][0] = mi.getX();
			sceneJson["modelInstances"][i]["position"][1] = mi.getY();
			sceneJson["modelInstances"][i]["position"][2] = mi.getZ();
			sceneJson["modelInstances"][i]["rotation"][0] = mi.getRotation().x;
			sceneJson["modelInstances"][i]["rotation"][1] = mi.getRotation().y;
			sceneJson["modelInstances"][i]["rotation"][2] = mi.getRotation().z;
			sceneJson["modelInstances"][i]["scale"][0] = mi.getScale().x;
			sceneJson["modelInstances"][i]["scale"][1] = mi.getScale().y;
			sceneJson["modelInstances"][i]["scale"][2] = mi.getScale().z;
			sceneJson["modelInstances"][i]["modelName"] = mi.getModel()->getName();
		}
		std::string outputText(sceneJson.dump(4));
		outputFile.open(project_directory + "\\assets\\scenes\\" + sceneName + "\\model_instances.json", std::ofstream::out | std::ofstream::trunc);
		outputFile << outputText;
		outputFile.close();
	}
	catch (...) {
		std::cout << "Error while saveing model instance of scene." << std::endl;
	}
}

// Return reference to modelInstances vector.
std::vector<ModelInstance>& ModelInstancesManager::getModelInstances() {
	return modelInstances;
}