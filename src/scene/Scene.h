#pragma once
#include <vector>
#include <string>
#include "model/ModelInstance.h"
#include "renderer/SunLight.h"
#include "renderer/Light.h"
#include "shader/Shader.h"
#ifndef MAX_LIGHTS
#define MAX_LIGHTS 8
#endif

class ModelInstancesManager {
private:
	std::vector<ModelInstance> modelInstances;
	std::string sceneName;
public:
	ModelInstancesManager(const std::string& name) : sceneName(name), modelInstances() { };
	void initialize();
	void terminate();
	void save();
	// Return a reference to the vector which contains all modelInstances.
	std::vector<ModelInstance>& getModelInstances();
};

class LightsManager {
private:
	std::vector<Light> lights;
	SunLight sunLight;
	bool useSunLight;
	std::string sceneName;
public:
	LightsManager(const std::string& name) : sceneName(name), useSunLight(false) { };
	void initialize();
	void terminate();
	void save();
	int getSize();
	Light& getLight(int);
	void addLight(Light);
	void removeLight(int);
	// To be called for each light every frame.
	void updateLight(Shader&, int index);
	std::vector<Light>& getLights();
	// To be called every frame.
	void updateSunLight(Shader&);
	SunLight& getSunLight();
	bool getUseSunLight();
	void setUseSunLight(bool);
};

class Scene {
private:
	ModelInstancesManager modelInstancesManager;
	LightsManager lightsManager;
	std::string sceneName;
public:
	Scene(const std::string& name) : sceneName(name), modelInstancesManager(name), lightsManager(name) { };
	Scene(const Scene& s) : sceneName(s.sceneName), modelInstancesManager(s.sceneName), lightsManager(s.sceneName) { };
	Scene() : sceneName(""), modelInstancesManager(""), lightsManager("") { };

	// Load everything needed by reading scene file with name stored in the class.
	// This function will call all initialize functions, if present, of all managers in the class.
	void initialize();
	// Terminate the scene, this includes models and other...
	// This function will call all terminate functions, if present, of all managers in the class.
	void terminate();
	// Save scene with what we currently have in model isntances and lights vectors.
	// And everything else of course.
	// This function basically calls every save method of all our "managers".
	void save();

	ModelInstancesManager& getModelInstancesManager() { return modelInstancesManager; }
	LightsManager& getLightsManager() { return lightsManager; }
};