#include "Renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "model/Model.h"
#include "shader/Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include "window/Window.h"
#include "camera/Camera.h"
#include "post/PostProcessing.h"
#include "shadow/Shadow.h"
#include "Skybox.h"

static Shader program;
static void (*renderFunctionPointer)();
static void (*initFunctionPointer)();
static void (*terminateFunctionPointer)();

void render() {

	// Can't remove.
	// Must be before render function.
	/* Should actually be after processInput()
	   in Window.cpp. */
	updateCameraTime();

	// Call specific render function.
	// Each render function represents a different renderer.
	// Each renderer must do everything by itself apart managing camera.
	(*renderFunctionPointer)();
}

// Must set init renderer function pointer.
// Must set renderer function pointer.
void initRenderer(void (*initF)(), void (*renderF)(), void (*terminateF)()) {

	// Set the function that will eventually be called.
	// Must do.
	renderFunctionPointer = renderF;
	initFunctionPointer = initF;
	terminateFunctionPointer = terminateF;

	// NOT NEEDED ANYMORE!
	// ---------------------------------------------- //
	// Load default models.
	// Default models must be loaded and cannot be removed.
	//loadAssetModel("default_cube", "obj");
	//loadAssetModel("default_sphere", "obj");

	// Load models.
	// Done on global renderer init because asset models will be used by all renderers.
	// ...
	//loadAssetModel("flat_sphere", "obj");
	//loadAssetModel("old_sponza", "obj");
	//loadAssetModel("rusty_sphere", "fbx");
	//loadAssetModel("plastic_sphere", "fbx");
	//loadAssetModel("dwarven_revolver", "obj");
	// ---------------------------------------------- //

	// Initialize post processing. (shader program, vao, ...)
	PostProcessing::initializePostProcessing();

	// Initialize skybox.
	Skybox::initialize();

	// Can't be removed.
	// Calls init function of renderer we passed.
	(*initFunctionPointer)();

	// Initialize shadows.
	// Must be done after initFunctionPointer because shadow initializing has dependencies on SimpleRenderer program.
	Shadow::initialize();
}

void terminateRenderer() {

	// Terminate what needs to be terminated.
	deleteAllAssetModels();

	// Terminate post processing and shadows.
	PostProcessing::terminate();
	Shadow::terminate();

	// Terminate skybox.
	Skybox::terminate();

	// Can't be removed.
	// Calls terminate function of renderer we passed.
	(*terminateFunctionPointer)();
}