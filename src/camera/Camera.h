#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

void updateCamera(short, bool);

glm::mat4 buildViewMatrix();

void updateCameraTime();

void updateCameraMouse(bool, double, double);