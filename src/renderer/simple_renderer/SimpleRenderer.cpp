#include "SimpleRenderer.h"
// Imgui prima di glad e glfw.
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
// Glad prima di glfw.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "model/Model.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "camera/Camera.h"
#include "window/Window.h"
#include "ProjectDirectory.h"
#include <iostream>
#include "model/ModelInstance.h"
#include "model/Mesh.h"
#include <vector>
#include "renderer/Light.h"
#include "gui/Gui.h"
#include "post/PostProcessing.h"
#include "renderer/shadow/Shadow.h"
#include "renderer/Skybox.h"

static Shader program;

static Scene currentScene;

static void draw(const ModelInstance&, const glm::mat4& view);
static void prepareFrame(const glm::mat4& projection, const glm::mat4& view);
static void prepareLights(const glm::mat4& view);
static void drawLights(const glm::mat4& view);
static void prepareTextures(const Mesh&);

static bool usePbr = true;
static float nearPlane = 0.1f, farPlane = 200.0f, fov = 45.0f;

void SimpleRenderer::render() {

	// Build matrices, view and projection, here and pass them when needed to not create them multiple times.
	glm::mat4 view = glm::mat4(1.0f);
	view = buildViewMatrix();
	glm::mat4 projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(fov), static_cast<float>(getWindowWidth()) / getWindowHeight(), nearPlane, farPlane);
	
	// Shadow pass.
	Shadow::shadowPass(projection, view);

	// Prepare frame for drawing.
	prepareFrame(projection, view);

	// After clearing buffers.
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	prepareLights(view);

	// Draw instances of models and set default color for the ones that have no texture.
	program.setBool("useLighting", true);
	for (const auto& mi : currentScene.getModelInstancesManager().getModelInstances()) {
		draw(mi, view);
	}

	// Draw lights before post processing to see actual color of lights.
	// If we don't the colors we see might not be accurate.
	drawLights(view);

	// Draw skybox.
	// Skybox is the last thing to be rendered before post processing.
	Skybox::draw(projection, view);

	// Post processing.
	PostProcessing::applyEffects();
	PostProcessing::draw();

	// After drawing.
	ImGui::Begin("SimpleRenderer");
	Gui::buildSimpleRendererGui();
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void SimpleRenderer::initRenderer() {
	// PROJECT_FOLDER is string macro so it get concatenated with "".
	program = Shader(project_directory + "\\shaders\\vshader.glsl", project_directory + "\\shaders\\fshader.glsl");
	
	// Load scene.
	currentScene = Scene("test");
	currentScene.initialize();
}

void SimpleRenderer::terminateRenderer() {

	// Terminate scene.
	currentScene.terminate();
}

// Stuff to do before rendering.
static void prepareFrame(const glm::mat4& projection, const glm::mat4& view) {

	// Viewport set for drawing models (not shadows).
	// Don't do shadow drawing between prepareFrame and actual drawing.
	glViewport(0, 0, getWindowWidth(), getWindowHeight());

	// Bind post processing framebuffer.
	glBindFramebuffer(GL_FRAMEBUFFER, PostProcessing::getFramebufferId());

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program.getShaderID());

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	program.setMat4("view", view);

	program.setMat4("projection", projection);

	program.setBool("usePbr", usePbr);

	Shadow::setShadowParametersForRendering(program);
}

// Set light parameters in shader.
static void prepareLights(const glm::mat4& view) {

	// Update SunLight.
	currentScene.getLightsManager().updateSunLight(program);

	// Do after changing number of lights.
	// Do not put code between this line and the next for.
	program.setInt("lightsNumber", currentScene.getLightsManager().getSize());

	for (int i = 0; i < currentScene.getLightsManager().getSize(); ++i) {
		currentScene.getLightsManager().updateLight(program, i);
	}
}

static void drawLights(const glm::mat4& view) {

	// Get default_cube asset and change its position every time we render it.
	ModelInstance lmi(0, 0, 0, glm::vec3(0, 0, 0), glm::vec3(1, 1, 1), getAssetModel("default_cube"));
	program.setBool("useTexture", false);
	program.setBool("useLighting", false);
	// Draw sun.
	// Looks wrong because it's directional.
	if (currentScene.getLightsManager().getUseSunLight()) {
		SunLight& sl = currentScene.getLightsManager().getSunLight();
		program.setVec3("defaultColor", sl.getDiffuse());
		lmi.setPosition(glm::vec3(sl.getPosition().x, sl.getPosition().y, sl.getPosition().z));
		draw(lmi, view);
	}
	for (int i = 0; i < currentScene.getLightsManager().getSize(); ++i) {
		Light& l = currentScene.getLightsManager().getLight(i);
		program.setVec3("defaultColor", l.getDiffuse());
		lmi.setPosition(glm::vec3(l.getPosition().x, l.getPosition().y, l.getPosition().z));
		draw(lmi, view);
	}
}

static void prepareTextures(const Mesh& mesh) {

	// Prepare textures in shader program.
	if (mesh.getTexturesByType(TextureType::DIFFUSE).size() > 0) {
		program.setBool("material.hasDiffuseMap", true);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mesh.getTexturesByType(TextureType::DIFFUSE)[0].id);
	}
	else
		program.setBool("material.hasDiffuseMap", false);

	if (mesh.getTexturesByType(TextureType::NORMAL).size() > 0) {
		program.setBool("material.hasNormalsMap", true);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, mesh.getTexturesByType(TextureType::NORMAL)[0].id);
	}
	else
		program.setBool("material.hasNormalsMap", false);

	if (mesh.getTexturesByType(TextureType::ROUGHNESS).size() > 0) {
		program.setBool("material.hasRoughnessMap", true);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, mesh.getTexturesByType(TextureType::ROUGHNESS)[0].id);
	}
	else
		program.setBool("material.hasRoughnessMap", false);

	if (mesh.getTexturesByType(TextureType::METALLIC).size() > 0) {
		program.setBool("material.hasMetallicMap", true);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, mesh.getTexturesByType(TextureType::METALLIC)[0].id);
	}
	else
		program.setBool("material.hasMetallicMap", false);
}

static void draw(const ModelInstance& mi, const glm::mat4& view) {

	if (mi.isDrawable()) {
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(mi.getX(), mi.getY(), mi.getZ()));
		model = glm::rotate(model, glm::radians(mi.getRotation()).z, glm::vec3(0, 0, 1));
		model = glm::rotate(model, glm::radians(mi.getRotation().x), glm::vec3(1, 0, 0));
		model = glm::rotate(model, glm::radians(mi.getRotation().y), glm::vec3(0, 1, 0));
		model = glm::scale(model, mi.getScale());

		glm::mat3 normal = glm::mat3(1.0f);
		normal = glm::mat3(glm::transpose(glm::inverse(view * model)));

		program.setMat4("model", model);
		program.setMat3("NormalMat", normal);

		program.setBool("useTexture", true);

		for (const auto& mesh : mi.getModel()->getMeshes()) {
			// Phong.
			program.setVec3("material.ambient", mesh.getMaterial().ambient);
			program.setVec3("material.diffuse", mesh.getMaterial().diffuse);
			program.setVec3("material.specular", mesh.getMaterial().specular);
			program.setFloat("material.shininess", mesh.getMaterial().shininess);

			// PBR.
			program.setFloat("material.roughness", mesh.getMaterial().roughness);
			program.setFloat("material.metallic", mesh.getMaterial().metallic);

			// General.

			// Textures.
			prepareTextures(mesh);

			glBindVertexArray(mesh.getVao());
			glDrawElements(GL_TRIANGLES, mesh.getIndicesSize(), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}
}

Scene& SimpleRenderer::getScene() {
	return currentScene;
}

void SimpleRenderer::setScene(Scene s) {
	currentScene = s;
}

void SimpleRenderer::setUsePbr(bool b) {
	usePbr = b;
}

bool SimpleRenderer::getUsePbr() {
	return usePbr;
}

const Shader& SimpleRenderer::getProgram() {
	return program;
}

float SimpleRenderer::getNearPlane() {
	return nearPlane;
}

float SimpleRenderer::getFarPlane() {
	return farPlane;
}

float SimpleRenderer::getFov() {
	return fov;
}