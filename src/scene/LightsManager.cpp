#include "Scene.h"
#include "ProjectDirectory.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void LightsManager::initialize()
{
	// First check if file exists. If it doesn't create it and skip reading step.
	try {
		std::ifstream filePath(project_directory + "\\assets\\scenes\\" + sceneName + "\\lights.json");
		if (!filePath) {
			filePath.close();
			std::fstream newFile;
			newFile.open(project_directory + "\\assets\\scenes\\" + sceneName + "\\lights.json", std::ios::out);
			newFile.close();
			return;
		} else {
			filePath.close();
		}
	}
	catch (...) {
		std::cout << "Error while checking existence/creating scene directory." << std::endl;
	}

	// Read Lights and add them to lights vector.
	try {
		std::ifstream f(project_directory + "\\assets\\scenes\\" + sceneName + "\\lights.json");
		json data = json::parse(f);

		glm::vec3 sunPosition(data["sunLight"]["position"][0],
			data["sunLight"]["position"][1], data["sunLight"]["position"][2]);
		glm::vec3 sunAmbient(data["sunLight"]["ambient"][0],
			data["sunLight"]["ambient"][1], data["sunLight"]["ambient"][2]);
		glm::vec3 sunDiffuse(data["sunLight"]["diffuse"][0],
			data["sunLight"]["diffuse"][1], data["sunLight"]["diffuse"][2]);
		glm::vec3 sunSpecular(data["sunLight"]["specular"][0],
			data["sunLight"]["specular"][1], data["sunLight"]["specular"][2]);

		sunLight = SunLight(sunPosition, sunAmbient, sunDiffuse, sunSpecular);

		// Enable sunLight if its value is true.
		useSunLight = data["sunLight"]["useSunLight"];

		int lightsNumber = data["lights"].size();
		for (int i = 0; i < lightsNumber; ++i) {
			glm::vec3 position(data["lights"][i]["position"][0],
				data["lights"][i]["position"][1], data["lights"][i]["position"][2]);
			glm::vec3 ambient(data["lights"][i]["ambient"][0],
				data["lights"][i]["ambient"][1], data["lights"][i]["ambient"][2]);
			glm::vec3 diffuse(data["lights"][i]["diffuse"][0],
				data["lights"][i]["diffuse"][1], data["lights"][i]["diffuse"][2]);
			glm::vec3 specular(data["lights"][i]["specular"][0],
				data["lights"][i]["specular"][1], data["lights"][i]["specular"][2]);
			float constant = data["lights"][i]["constant"];
			float linear = data["lights"][i]["linear"];
			float quadratic = data["lights"][i]["quadratic"];

			lights.push_back(Light(position, ambient, diffuse, specular, constant, linear, quadratic));
		}
		// Remember to close the file.
		f.close();
	}
	catch (...) {
		std::cout << "Error while loading scene lights." << std::endl;
	}
}

void LightsManager::terminate() {
	// Clear all lights.
	lights.clear();
}

void LightsManager::save() {
	std::ofstream outputFile;
	try {
		json sceneJson;
		// Create json object and dump as string (int in parethesis represents indentation spaces).
		sceneJson["sunLight"]["position"][0] = sunLight.getPosition().x;
		sceneJson["sunLight"]["position"][1] = sunLight.getPosition().y;
		sceneJson["sunLight"]["position"][2] = sunLight.getPosition().z;
		sceneJson["sunLight"]["ambient"][0] = sunLight.getAmbient().r;
		sceneJson["sunLight"]["ambient"][1] = sunLight.getAmbient().g;
		sceneJson["sunLight"]["ambient"][2] = sunLight.getAmbient().b;
		sceneJson["sunLight"]["diffuse"][0] = sunLight.getDiffuse().r;
		sceneJson["sunLight"]["diffuse"][1] = sunLight.getDiffuse().g;
		sceneJson["sunLight"]["diffuse"][2] = sunLight.getDiffuse().b;
		sceneJson["sunLight"]["specular"][0] = sunLight.getSpecular().r;
		sceneJson["sunLight"]["specular"][1] = sunLight.getSpecular().g;
		sceneJson["sunLight"]["specular"][2] = sunLight.getSpecular().b;
		sceneJson["sunLight"]["useSunLight"] = useSunLight;
		for (int i = 0; i < lights.size(); ++i) {
			const Light& l = lights[i];
			sceneJson["lights"][i]["position"][0] = l.getPosition().x;
			sceneJson["lights"][i]["position"][1] = l.getPosition().y;
			sceneJson["lights"][i]["position"][2] = l.getPosition().z;
			sceneJson["lights"][i]["ambient"][0] = l.getAmbient().r;
			sceneJson["lights"][i]["ambient"][1] = l.getAmbient().g;
			sceneJson["lights"][i]["ambient"][2] = l.getAmbient().b;
			sceneJson["lights"][i]["diffuse"][0] = l.getDiffuse().r;
			sceneJson["lights"][i]["diffuse"][1] = l.getDiffuse().g;
			sceneJson["lights"][i]["diffuse"][2] = l.getDiffuse().b;
			sceneJson["lights"][i]["specular"][0] = l.getSpecular().r;
			sceneJson["lights"][i]["specular"][1] = l.getSpecular().g;
			sceneJson["lights"][i]["specular"][2] = l.getSpecular().b;
			sceneJson["lights"][i]["constant"] = l.getConstant();
			sceneJson["lights"][i]["linear"] = l.getLinear();
			sceneJson["lights"][i]["quadratic"] = l.getQuadratic();
		}
		std::string outputText(sceneJson.dump(4));
		outputFile.open(project_directory + "\\assets\\scenes\\" + sceneName + "\\lights.json", std::ofstream::out | std::ofstream::trunc);
		outputFile << outputText;
		outputFile.close();
	}
	catch (...) {
		std::cout << "Error while saveing lights of scene." << std::endl;
	}
}

Light& LightsManager::getLight(int index) {
	if (index < MAX_LIGHTS)
		return lights[index];
	else
		return lights[MAX_LIGHTS - 1];
}

int LightsManager::getSize() {
	return lights.size();
}

void LightsManager::addLight(Light l) {
	if (lights.size() == MAX_LIGHTS)
		return;
	lights.push_back(std::move(l));
}

// Remove light and set update of following to multiple to make sure shader changes the values.
void LightsManager::removeLight(int index) {
	if (index < lights.size()) {
		lights.erase(lights.begin() + index);
		// Set light update of following lights to multiple.
		for (int i = index; i < lights.size(); ++i) {
			lights[i].setUpdate(LightUpdate::MULTIPLE);
		}
	}
}

std::vector<Light>& LightsManager::getLights() {
	return lights;
}

SunLight& LightsManager::getSunLight()
{
	return sunLight;
}

void LightsManager::setUseSunLight(bool b)
{
	useSunLight = b;
}

bool LightsManager::getUseSunLight() {
	return useSunLight;
}

// Must be called every frame.
// Function to update shader values of light if they changed.
void LightsManager::updateSunLight(Shader& program)
{
	program.setBool("useSunLight", useSunLight);

	if (useSunLight) {
		switch (sunLight.getUpdate()) {
		case SunLightUpdate::FALSE:
			break;
		case SunLightUpdate::POSITION:
			program.setVec3("sunLight.position", sunLight.getPosition());
			sunLight.resetUpdate();
			break;
		case SunLightUpdate::AMBIENT:
			program.setVec3("sunLight.ambient", sunLight.getAmbient());
			sunLight.resetUpdate();
			break;
		case SunLightUpdate::DIFFUSE:
			program.setVec3("sunLight.diffuse", sunLight.getDiffuse());
			sunLight.resetUpdate();
			break;
		case SunLightUpdate::SPECULAR:
			program.setVec3("sunLight.specular", sunLight.getSpecular());
			sunLight.resetUpdate();
			break;
		case SunLightUpdate::MULTIPLE:
			program.setVec3("sunLight.position", sunLight.getPosition());
			program.setVec3("sunLight.ambient", sunLight.getAmbient());
			program.setVec3("sunLight.diffuse", sunLight.getDiffuse());
			program.setVec3("sunLight.specular", sunLight.getSpecular());
			sunLight.resetUpdate();
			break;
		}
	}
}

// Must be called every frame.
// Function to update shader values of light if they changed.
void LightsManager::updateLight(Shader& program, int index) {
	Light& l = lights[index];
	switch (l.getUpdate()) {
	case LightUpdate::FALSE:
		break;
	case LightUpdate::POSITION:
		program.setVec3("lights[" + std::to_string(index) + "].position", l.getPosition());
		l.resetUpdate();
		break;
	case LightUpdate::AMBIENT:
		program.setVec3("lights[" + std::to_string(index) + "].ambient", l.getAmbient());
		l.resetUpdate();
		break;
	case LightUpdate::DIFFUSE:
		program.setVec3("lights[" + std::to_string(index) + "].diffuse", l.getDiffuse());
		l.resetUpdate();
		break;
	case LightUpdate::SPECULAR:
		program.setVec3("lights[" + std::to_string(index) + "].specular", l.getSpecular());
		l.resetUpdate();
		break;
	case LightUpdate::CONSTANT:
		program.setFloat("lights[" + std::to_string(index) + "].constant", l.getConstant());
		l.resetUpdate();
		break;
	case LightUpdate::LINEAR:
		program.setFloat("lights[" + std::to_string(index) + "].linear", l.getLinear());
		l.resetUpdate();
		break;
	case LightUpdate::QUADRATIC:
		program.setFloat("lights[" + std::to_string(index) + "].quadratic", l.getQuadratic());
		l.resetUpdate();
		break;
	case LightUpdate::MULTIPLE:
		program.setVec3("lights[" + std::to_string(index) + "].position", l.getPosition());
		program.setVec3("lights[" + std::to_string(index) + "].ambient", l.getAmbient());
		program.setVec3("lights[" + std::to_string(index) + "].diffuse", l.getDiffuse());
		program.setVec3("lights[" + std::to_string(index) + "].specular", l.getSpecular());
		program.setFloat("lights[" + std::to_string(index) + "].constant", l.getConstant());
		program.setFloat("lights[" + std::to_string(index) + "].linear", l.getLinear());
		program.setFloat("lights[" + std::to_string(index) + "].quadratic", l.getQuadratic());
		l.resetUpdate();
		break;
	}
}
