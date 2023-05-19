#include "Window.h"
// Imgui prima di glad e glfw.
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
// Glad prima di glfw.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "renderer/Renderer.h"
#include "camera/Camera.h"
#include "renderer/simple_renderer/SimpleRenderer.h"
#include "post/PostProcessing.h"

void framebuffer_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow*);

bool initializeWindow();
bool initializeGLAD();
void initializeImgui(GLFWwindow*);

void startLoop();

static GLFWwindow* window = 0;
static GLFWmonitor* monitor = 0;
static int width = 800, height = 600;

static bool cursorEnabled = false;

int startProgram() {

	// Create window and make context current. Checks failure.
	if (!initializeWindow())
		return -1;

	// Initialize GLAD. Checks failure.
	if (!initializeGLAD())
		return -1;

	// Disable vsync.
	glfwSwapInterval(0);

	glViewport(0, 0, width, height);
	glfwSetFramebufferSizeCallback(window, framebuffer_callback);

	// Configure mouse.
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);

	// Key callback.
	// Used for "single" press keys.
	// For movement and continuous press use processInput.
	glfwSetKeyCallback(window, key_callback);

	// IMPORTANT!!!
	// Must be initialized after setting callbacks.
	// If you don't then glfw will overwrite those set by ImGui during initialization.
	initializeImgui(window);

	// Program loop.
	// When this finishes program finishes.
	startLoop();

	// Terminate Imgui.
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

// For counting FPS.
unsigned int fpsCounter, lastFps;
double lastTime;

void startLoop() {

	// Pass the renderer init and render functions.
	initRenderer(SimpleRenderer::initRenderer, SimpleRenderer::render, SimpleRenderer::terminateRenderer);

	// For counting FPS.
	fpsCounter = 0, lastFps = 0;
	lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		processInput(window);

		render();

		glfwSwapBuffers(window);
		glfwPollEvents();

		// Increase FPS counter. Print and reset if over 1 sec passed.
		++fpsCounter;
		if (glfwGetTime() - lastTime > 1.0) {
			// Not done because now shown in gui.
			// If gui is not used then uncomment if needed.
			// std::cout << "FPS: " << fpsCounter << std::endl;
			lastFps = fpsCounter;
			fpsCounter = 0;
			lastTime = glfwGetTime();
		}
	}

	// Terminate renderer.
	terminateRenderer();
}

bool initializeWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(800, 600, "GabryEngine", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create window. :(" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);

	// Get monitor for application.
	// Not needed immediately.
	int mNumber = 0;
	GLFWmonitor** monitors = glfwGetMonitors(&mNumber);
	monitor = monitors[0];

	return true;
}

bool initializeGLAD() {
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD. :(" << std::endl;
		return false;
	}
	return true;
}

void initializeImgui(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

void framebuffer_callback(GLFWwindow* window, int width, int height) {
	::width = width;
	::height = height;

	// No need to do it because we do it per frame.
	// glViewport(0, 0, width, height);

	// Update Post Processing framebuffers because their size changes.
	// MUST be done after changeing size variables.
	// This function gets called also on fullscreen toggle
	// so no need to put this code in the fullscreen function.
	PostProcessing::deleteBuffers();
	PostProcessing::createBuffers();
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	// DON'T REMOVE.
	// Make sure to add event to ImGui so that when changin windows it still works.
	ImGui::GetIO().AddMousePosEvent(xpos, ypos);
	// Update camera.
	updateCameraMouse(cursorEnabled, xpos, ypos);
}

void processInput(GLFWwindow* window) {
	// Move only if no gui item is active.
	if (!ImGui::IsAnyItemActive()) {
		// Movement block.
		bool isShiftDown = false;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			isShiftDown = true;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			updateCamera(GLFW_KEY_W, isShiftDown);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			updateCamera(GLFW_KEY_A, isShiftDown);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			updateCamera(GLFW_KEY_S, isShiftDown);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			updateCamera(GLFW_KEY_D, isShiftDown);
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			updateCamera(GLFW_KEY_SPACE, isShiftDown);
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			updateCamera(GLFW_KEY_LEFT_CONTROL, isShiftDown);
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Move only if no gui item is active.
	if (!ImGui::IsAnyItemActive()) {
		// Fullscreen.
		if (key == GLFW_KEY_F && action == GLFW_PRESS) {
			if (glfwGetWindowMonitor(window) == NULL) {
				// Case is windowed.
				const GLFWvidmode* mode = glfwGetVideoMode(monitor);
				glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			}
			else {
				// Case is fullscreen.
				glfwSetWindowMonitor(window, NULL, 100, 100, 800, 600, 0);
			}
		}
		// Close window and exit program.
		else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}
		else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
			if (cursorEnabled) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				cursorEnabled = false;
			}
			else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				cursorEnabled = true;
			}
		}
	}
}

int getWindowWidth() {
	return width;
}

int getWindowHeight() {
	return height;
}

unsigned int Window::getFps() {
	return lastFps;
}