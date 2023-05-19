#include "Scene.h"
#include <filesystem>
#include "ProjectDirectory.h"

// Function used to load everything needed in the scene.
// For example by reading a file, or simply creating every model instance by hand.
// This function will call all initialize functions, if present, of all managers in the class.
void Scene::initialize() {
	// Check if directory exists.
	// If it doesn't it creates it.
	// This process will also be done for every initialize function below,
	// but with their respective .json files.
	try {
		std::filesystem::path folderPath(project_directory + "\\assets\\scenes\\" + sceneName);
		if (!std::filesystem::exists(folderPath))
			std::filesystem::create_directory(folderPath);
	}
	catch (...) {
		std::cout << "Error while checking existence/creating scene directory." << std::endl;
	}

	modelInstancesManager.initialize();
	lightsManager.initialize();
}

// Terminate everything that needs to be terminated.
// For example all models.
// This function will call all terminate functions, if present, of all managers in the class.
void Scene::terminate() {
	modelInstancesManager.terminate();
	lightsManager.terminate();
}

// Save scene with what we currently have in model isntances and lights vectors.
// And everything else of course.
// This function basically calls every save method of all our "managers".
void Scene::save() {
	modelInstancesManager.save();
	lightsManager.save();
}