#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>

static glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
static glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
static glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
static float cameraSpeed = 2.0f;
static float lastFrame = 0.0f;
static bool firstMouse = true;
static float yaw = -90.0f, pitch, lastX, lastY;

void updateCamera(short keyCode, bool shiftDown) {
	float deltaTime = glfwGetTime() - lastFrame;
    glm::vec3 normalizedCameraFront = glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
    float multiplier = shiftDown? 8.0f: 1.0f;
    float cameraSpeed = ::cameraSpeed * multiplier;
	switch (keyCode) {
	case GLFW_KEY_W:
		cameraPos += normalizedCameraFront * cameraSpeed * deltaTime;
		break;
	case GLFW_KEY_A:
		cameraPos -= glm::normalize(glm::cross(normalizedCameraFront, cameraUp)) * cameraSpeed * deltaTime;
		break;
	case GLFW_KEY_S:
		cameraPos -= normalizedCameraFront * cameraSpeed * deltaTime;
		break;
	case GLFW_KEY_D:
		cameraPos += glm::normalize(glm::cross(normalizedCameraFront, cameraUp)) * cameraSpeed * deltaTime;
		break;
    case GLFW_KEY_SPACE:
        cameraPos.y += cameraSpeed * deltaTime;
        break;
    case GLFW_KEY_LEFT_CONTROL:
        cameraPos.y -= cameraSpeed * deltaTime;
        break;
	}
}

glm::mat4 buildViewMatrix() {
	return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

// Has to be called once per frame. Possibly near the end.
void updateCameraTime() {
	lastFrame = glfwGetTime();
}

// Called in mouse_callback (Window.cpp). 
// Updates rotation based on mouse movement.
void updateCameraMouse(bool cursorEnabled, double xpos, double ypos) {
    if (cursorEnabled) {
        firstMouse = true;
        return;
    }

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}