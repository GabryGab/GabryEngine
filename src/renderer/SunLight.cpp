#include "SunLight.h"
#include <iostream>
#include <string>

void SunLight::print() {
	std::string finalString = "";
	finalString.append("Position: " + std::to_string(position.x) + " " + std::to_string(position.y) + " " + std::to_string(position.z) + "\n");
	finalString.append("Ambient: " + std::to_string(ambient.x) + " " + std::to_string(ambient.y) + " " + std::to_string(ambient.z) + "\n");
	finalString.append("Diffuse: " + std::to_string(diffuse.x) + " " + std::to_string(diffuse.y) + " " + std::to_string(diffuse.z) + "\n");
	finalString.append("Specular: " + std::to_string(specular.x) + " " + std::to_string(specular.y) + " " + std::to_string(specular.z));
	std::cout << finalString << std::endl;
}