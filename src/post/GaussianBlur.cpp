#include "GaussianBlur.h"
#include <vector>
#include "window/Window.h"
#include <iostream>
#include <cmath>
#include "shader/Shader.h"
#include "ProjectDirectory.h"
// Glad prima di GLFW.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define M_PI 3.14159265358979323846 /* pi */

// GaussianBlur creates framebuffers for each blur, as it is intended to use on single blur passes.
// If it is ever needed to use on contiguous frame, make it that it doesn't create buffers unless needed.
// Examples of needing to recreate buffers is resizing of window. 

static std::vector<float> buildDistribution();
static float gaussianDistributionAt(float);
static void updateBuffers(int, int);
static void prepare(const std::vector<float>&, int vpWidth, int vpHeight);
static void draw(unsigned int, unsigned int);

// strength affects quality but makes it possible to use less blur iterations.
static float strength = 1.0f, quality = 1.0f;
static int radius = 12, iterations = 1;
static Shader program;
static unsigned int fbo[2], texture[2], vao;
static int lastWidth = 0, lastHeight = 0;

void GaussianBlur::blur(unsigned int outputFbo, unsigned int inputTex, int width, int height) {

	// Build distribution.
	std::vector<float> values = buildDistribution();

	// Build FBOs.
	updateBuffers(width, height);

	// Prepare for draw.
	prepare(values, width, height);

	// Draw.
	draw(outputFbo, inputTex);
}

static void draw(unsigned int outputFbo, unsigned int inputTex) {

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(vao);

	bool horizontal = true, firstIteration = true;
	for (int i = 0; i < iterations * 2; ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, i != (iterations * 2 - 1) ? fbo[horizontal] : outputFbo);
		program.setBool("horizontal", horizontal);
		glBindTexture(GL_TEXTURE_2D, firstIteration ? inputTex : texture[!horizontal]);
		// Draw actual quad.
		glDrawArrays(GL_TRIANGLES, 0, 6);
		horizontal = !horizontal;
		if (firstIteration)
			firstIteration = false;
	}

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void prepare(const std::vector<float>& values, int vpWidth, int vpHeight) {
	// Set program and opengl context status.
	glUseProgram(program.getShaderID());

	glViewport(0, 0, vpWidth, vpHeight);

	glDisable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	// Set parameters for blur.
	program.setInt("radius", values.size());
	for (int i = 0; i < values.size(); ++i) {
		program.setFloat(std::string(("distribution[") + std::to_string(i) + std::string("]")).c_str(), values[i]);
	}
}

static void updateBuffers(int width, int height) {

	// Update textures (according to size).
	// If size didn't change texture stays the same.
	if (width != lastWidth || height != lastHeight) {
		for (int i = 0; i < 2; ++i) {
			glBindTexture(GL_TEXTURE_2D, texture[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		}
		lastWidth = width;
		lastHeight = height;
	}
}

static std::vector<float> buildDistribution() {

	// If we change 3 with some other value we can change blur quality.
	// It is based on percentile of gaussian function covered. 3 is 99.something %.
	float range = std::min(3.0f, 3 * quality); // Make sure range is maximum 3.

	// Adjusted to screen width to assure blur looks almost identical at different resolutions.
	// radius is intended as radius/1000, so if resolution is 2000 pixels finalRadius will be two times radius.
	int finalRadius = std::round(static_cast<float>(getWindowWidth()) / 1000.0f * radius);

	if (radius != 0)
		finalRadius = finalRadius < 3 ? finalRadius = 3 : finalRadius;

	std::vector<float> values;

	float sum = 0;

	for (int i = 0; i < finalRadius; ++i) {
		float y = gaussianDistributionAt(std::pow(static_cast<float>(i) / (finalRadius - 1), strength) * range);
		values.push_back(y);
		sum += i == 0 ? y : y * 2;
	}

	float normalizationFactor = 1.0f / sum;

	// Normalize values so that sum is equal to 1.
	for (int i = 0; i < finalRadius; ++i) {
		values[i] = values[i] * normalizationFactor;
	}

	return values;
}

static float gaussianDistributionAt(float x) {
	float factor1 = 1.0f / std::sqrt(2 * M_PI);
	float numerator = -std::pow(x, 2);
	float denominator = 2;
	float exponent = numerator / denominator;
	float factor2 = std::exp(exponent);
	return factor1 * factor2;
}

void GaussianBlur::initialize() {
	// Initialize program.
	program = Shader(project_directory + "\\shaders\\vshader_post.glsl", project_directory + "\\shaders\\fshader_gaussian.glsl");

	// Generate vao.
	glGenVertexArrays(1, &vao);

	// Create framebuffers and textures.
	glGenFramebuffers(2, fbo);
	glGenTextures(2, texture);

	for (int i = 0; i < 2; ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);
		glBindTexture(GL_TEXTURE_2D, texture[i]);
		// Size is random if I set it to 0 it gives framebuffer error (don't know why).
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 64, 64, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture[i], 0);

		// Check Framebuffer status at the end.
		if (!(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE))
			std::cout << "Something went wrong while creating Framebuffer" << std::endl;
	}

	// Bind default framebuffer to avoid undesired behaviour.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GaussianBlur::terminate() {
	// Delete everything.
	glDeleteFramebuffers(2, fbo);
	glDeleteTextures(2, texture);
}

int GaussianBlur::getRadius() {
	return radius;
}

float GaussianBlur::getStrength() {
	return strength;
}

int GaussianBlur::getIterations() {
	return iterations;
}

float GaussianBlur::getQuality() {
	return quality;
}

void GaussianBlur::setStrength(float f) {
	strength = f;
}

void GaussianBlur::setRadius(int i) {
	radius = i;
}

void GaussianBlur::setIterations(int i) {
	iterations = i;
}

void GaussianBlur::setQuality(float f) {
	quality = f;
}