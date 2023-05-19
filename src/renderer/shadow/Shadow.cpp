#include "Shadow.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader/Shader.h"
#include "ProjectDirectory.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "renderer/simple_renderer/SimpleRenderer.h"
#include "window/Window.h"
#include "camera/Camera.h"
#include "PoissonDisk.h"
#include <cmath>

static Shader program;
static unsigned int fbo, depthMaps, poissonPcfSamples = 16, csmLayers = 4;
static const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
static glm::mat4 projection, view, lightSpaceMat;
static bool useShadows = true, usePcf = true, usePoissonPcf = true;
static float poissonPcfDiameter = 6.0f, csmBlendingOffset = 1.0f, csmPlanesDistanceInterpolationFactor = 0.35f, csmZMultiplier = 20.0f, shadowBiasMultiplier = 0.015f, shadowBiasMinimum = 0.0015f;
static std::vector<glm::mat4> lightSpaceMatrices;
static std::vector<float> pcfMultipliers, csmPlanes;

static void prepareDraw(const glm::mat4& proj, const glm::mat4& view);
static void draw();
static std::vector<glm::vec4> getFrustumCoordinatesWorldSpace(const glm::mat4& proj, const glm::mat4& mView);
static glm::vec3 getCenterCoordinateFromCorners(const std::vector<glm::vec4>& corners);
static glm::mat4 getProjectionMatrixFromCorners(std::vector<float>& pcfMult, const std::vector<glm::vec4>& corners, const glm::mat4& view, float zMult);
static void updatePoissonDisk(const std::vector<glm::vec2>& pcfSamples, const Shader& program);
static std::vector<float> buildClipPlanes(float near, float far, int numberOfVolumes);

void Shadow::initialize() {
	// Generate framebuffer and attachments.
	glGenFramebuffers(1, &fbo);

	glGenTextures(1, &depthMaps);
	glBindTexture(GL_TEXTURE_2D_ARRAY, depthMaps);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, SHADOW_WIDTH, SHADOW_HEIGHT, csmLayers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMaps, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	program = Shader(project_directory + "\\shaders\\vshader_shadow.glsl", project_directory + "\\shaders\\gshader_shadow.glsl", project_directory + "\\shaders\\fshader_empty.glsl");

	projection = glm::mat4(1.0f);
	view = glm::mat4(1.0f);

	// Set up poisson disk pcf.
	std::vector<glm::vec2> pcfPoints = PoissonDisk::generatePoissonDisk(poissonPcfSamples);
	const Shader& spProgram = SimpleRenderer::getProgram();
	updatePoissonDisk(pcfPoints, spProgram);
}

void Shadow::shadowPass(const glm::mat4& proj, const glm::mat4& view) {
	if (useShadows) {
		prepareDraw(proj, view);
		draw();
	}
}

void prepareDraw(const glm::mat4& proj, const glm::mat4& view) {

	// Viewport set for drawing.
	// Width and height set to shadow map resolution.
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

	// Bind post processing framebuffer.
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_TEXTURE_2D_ARRAY, depthMaps, 0);

	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(program.getShaderID());

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	pcfMultipliers.clear();
	lightSpaceMatrices.clear();
	csmPlanes.clear();
	csmPlanes = buildClipPlanes(SimpleRenderer::getNearPlane(), SimpleRenderer::getFarPlane(), csmLayers);
	for (int i = 0; i < csmLayers; ++i) {

		// Build and set everything needed in terms of matrices.
		glm::mat4 tempProj;
		if (i == 0) {
			tempProj = glm::perspective(glm::radians(SimpleRenderer::getFov()),
				static_cast<float>(getWindowWidth()) / getWindowHeight(), csmPlanes[i], csmPlanes[i + 1] + csmBlendingOffset / 2);
		}
		else if (i == csmLayers - 1) {
			tempProj = glm::perspective(glm::radians(SimpleRenderer::getFov()),
				static_cast<float>(getWindowWidth()) / getWindowHeight(), csmPlanes[i] - csmBlendingOffset / 2, csmPlanes[i + 1]);
		}
		else {
			tempProj = glm::perspective(glm::radians(SimpleRenderer::getFov()),
				static_cast<float>(getWindowWidth()) / getWindowHeight(), csmPlanes[i] - csmBlendingOffset / 2, csmPlanes[i + 1] + csmBlendingOffset / 2);
		}

		std::vector<glm::vec4> corners = getFrustumCoordinatesWorldSpace(tempProj, view);

		// Calculate center for lookAt.
		glm::vec3 center = getCenterCoordinateFromCorners(corners);

		// Build view matrix.
		glm::vec3 lightDir = glm::normalize(SimpleRenderer::getScene().getLightsManager().getSunLight().getPosition());
		glm::mat4 finalView = glm::lookAt(center + lightDir, center, glm::vec3(0, 1, 0));

		glm::mat4 finalProj = getProjectionMatrixFromCorners(pcfMultipliers, corners, finalView, csmZMultiplier);

		// Build final lightSpaceMat matrix.
		glm::mat4 lightSpaceMat = finalProj * finalView;

		// This setMat4 is for shadow shader.
		// The one in setShadowParametersForRendering is for SimpleRenderer shader.
		program.setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", lightSpaceMat);

		lightSpaceMatrices.push_back(lightSpaceMat);
	}
	for (int i = csmLayers; i > 0; --i) {
		pcfMultipliers[i - 1] = pcfMultipliers[0] / pcfMultipliers[i - 1];
	}
}

void draw() {
	std::vector<ModelInstance>& modelInstances = SimpleRenderer::getScene().getModelInstancesManager().getModelInstances();
	for (const auto& mi : modelInstances) {
		if (mi.isDrawable()) {
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(mi.getX(), mi.getY(), mi.getZ()));
			model = glm::rotate(model, glm::radians(mi.getRotation()).z, glm::vec3(0, 0, 1));
			model = glm::rotate(model, glm::radians(mi.getRotation().x), glm::vec3(1, 0, 0));
			model = glm::rotate(model, glm::radians(mi.getRotation().y), glm::vec3(0, 1, 0));
			model = glm::scale(model, mi.getScale());

			program.setMat4("model", model);

			for (const auto& mesh : mi.getModel()->getMeshes()) {

				glBindVertexArray(mesh.getVao());
				glDrawElements(GL_TRIANGLES, mesh.getIndicesSize(), GL_UNSIGNED_INT, 0);
				glBindVertexArray(0);
			}
		}
	}
}

void Shadow::setShadowParametersForRendering(const Shader& mainProgram) {
	mainProgram.setBool("useShadows", Shadow::getUseShadows());
	mainProgram.setBool("usePcf", Shadow::getUsePcf());
	mainProgram.setBool("usePoissonPcf", Shadow::getUsePoissonPcf());
	mainProgram.setFloat("poissonPcfDiameter", Shadow::getPoissonPcfDiameter());
	mainProgram.setFloat("csmBlendingOffset", csmBlendingOffset);
	mainProgram.setFloat("shadowBiasMultiplier", shadowBiasMultiplier);
	mainProgram.setFloat("shadowBiasMinimum", shadowBiasMinimum);

	// Set shadow matrices and texture.
	for (int i = 0; i < csmLayers; ++i) {
		mainProgram.setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", lightSpaceMatrices[i]);
		mainProgram.setFloat("pcfMultipliers[" + std::to_string(i) + "]", pcfMultipliers[i]);
	}
	for (int i = 0; i < csmLayers + 1; ++i) {
		mainProgram.setFloat("cascadePlaneDistances[" + std::to_string(i) + "]", csmPlanes[i]);
	}
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D_ARRAY, Shadow::getTextureId());
}

// Update poisson pcf samples in shader.
static void updatePoissonDisk(const std::vector<glm::vec2>& pcfSamples, const Shader& program) {
	program.setInt("pcfSamplesNumber", pcfSamples.size());
	for (int i = 0; i < pcfSamples.size(); i++) {
		program.setVec2("pcfSamples[" + std::to_string(i) + "]", pcfSamples[i]);
	}
}

static std::vector<glm::vec4> getFrustumCoordinatesWorldSpace(const glm::mat4& proj, const glm::mat4& view) {
	const glm::mat4 inv = glm::inverse(proj * view);

	std::vector<glm::vec4> frustumCorners;
	for (unsigned int x = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				const glm::vec4 pt =
					inv * glm::vec4(
						2.0f * x - 1.0f,
						2.0f * y - 1.0f,
						2.0f * z - 1.0f,
						1.0f);
				frustumCorners.push_back(pt / pt.w);
			}
		}
	}

	return frustumCorners;
}

static glm::vec3 getCenterCoordinateFromCorners(const std::vector<glm::vec4>& corners) {
	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const auto& v : corners)
	{
		center += glm::vec3(v);
	}
	center /= corners.size();

	return center;
}

static glm::mat4 getProjectionMatrixFromCorners(std::vector<float>& pcfMul, const std::vector<glm::vec4>& corners, const glm::mat4& view, float zMult) {

	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::lowest();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::lowest();
	for (const auto& v : corners)
	{
		const auto trf = view * v;
		minX = std::min(minX, trf.x);
		maxX = std::max(maxX, trf.x);
		minY = std::min(minY, trf.y);
		maxY = std::max(maxY, trf.y);
		minZ = std::min(minZ, trf.z);
		maxZ = std::max(maxZ, trf.z);
	}

	pcfMul.push_back(maxX - minX);

	// Tune this parameter according to the scene
	if (minZ < 0)
	{
		minZ *= zMult;
	}
	else
	{
		minZ /= zMult;
	}
	if (maxZ < 0)
	{
		maxZ /= zMult;
	}
	else
	{
		maxZ *= zMult;
	}

	glm::mat4 projection = glm::mat4(1.0f);
	projection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

	return projection;
}

static float lerp(float a, float b, float f) {
	return (a * (1.0 - f)) + (b * f);
}

// Don't use it until it works.
// Last thing to change in csm.
static std::vector<float> buildClipPlanes(float near, float far, int numberOfVolumes) {
	std::vector<float> planesVector;

	float multiplier = std::pow(far - near, 1.0f / numberOfVolumes);
	planesVector.push_back(near);
	for (int i = 1; i < numberOfVolumes; ++i) {
		planesVector.push_back(lerp(std::pow(multiplier, i), (far - near) / numberOfVolumes * i, csmPlanesDistanceInterpolationFactor) + near);
	}
	planesVector.push_back(far);

	//for (auto x : planesVector)
		//std::cout << x << std::endl;

	return planesVector;
}

// Safe to call even if it hasn't been created.
void Shadow::terminate() {
	glDeleteFramebuffers(1, &fbo);
}

glm::mat4& Shadow::getLightSpaceMatrix() {
	return lightSpaceMat;
}

unsigned int Shadow::getTextureId() {
	return depthMaps;
}

Shader& Shadow::getShader() {
	return program;
}

int Shadow::getPoissonPcfSamplesNumber() {
	return poissonPcfSamples;
}

// Sets poissonPcfSamples, generates new points and sets them in the shader.
void Shadow::setPoissonPcfSamplesNumber(int n) {
	poissonPcfSamples = n <= 16 ? n : 16;
	std::vector<glm::vec2> pcfPoints = PoissonDisk::generatePoissonDisk(poissonPcfSamples);
	const Shader& spProgram = SimpleRenderer::getProgram();
	updatePoissonDisk(pcfPoints, spProgram);
}

bool Shadow::getUsePcf() { return usePcf; }
bool Shadow::getUsePoissonPcf() { return usePoissonPcf; }
bool Shadow::getUseShadows() { return useShadows; }

void Shadow::setUsePcf(bool b) { usePcf = b; }
void Shadow::setUsePoissonPcf(bool b) { usePoissonPcf = b; }
void Shadow::setUseShadows(bool b) { useShadows = b; }

float Shadow::getPoissonPcfDiameter() {
	return poissonPcfDiameter;
}

void Shadow::setPoissonPcfDiameter(float f) {
	poissonPcfDiameter = f;
}

float Shadow::getCsmPlanesDistanceInterpolationFactor() { return csmPlanesDistanceInterpolationFactor; }
void Shadow::setCsmPlanesDistanceInterpolationFactor(float f) { csmPlanesDistanceInterpolationFactor = f; }

float Shadow::getCsmBlendingOffset() { return csmBlendingOffset; }
void Shadow::setCsmBlendingOffset(float f) { csmBlendingOffset = f; }

float Shadow::getShadowBiasMultiplier() { return shadowBiasMultiplier; }
void Shadow::setShadowBiasMultiplier(float f) { shadowBiasMultiplier = f; }

float Shadow::getShadowBiasMinimum() { return shadowBiasMinimum; }
void Shadow::setShadowBiasMinimum(float f) { shadowBiasMinimum = f; }