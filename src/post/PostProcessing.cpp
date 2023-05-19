#include "PostProcessing.h"
#include <glad/glad.h>
#include <iostream>
#include "window/Window.h"
#include "ProjectDirectory.h"
#include "model/Model.h"
#include <stb_image.h>
#include "renderer/shadow/Shadow.h"
#include "GaussianBlur.h"
#include "bloom/Bloom.h"

static unsigned int fbo, vao, textureColorbuffer, rbo;
static bool isCreated = false, useTonemapping = true, useGammacorrection = true,
usePostProcessing = true, useBlackAndWhite = false, useGaussianBlur = false, useBloom = true;
static float exposure = 1.0f, gamma = 2.2f;
static Shader program;

void PostProcessing::createBuffers() {
	// Create framebuffer and bind it.
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Create color attachment as texture.
	glGenTextures(1, &textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, getWindowWidth(), getWindowHeight(), 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Create depth and stencil attachment as renderbuffer.
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, getWindowWidth(), getWindowHeight());
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// Add attachments.
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	// Check Framebuffer status at the end.
	if (!(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE))
		std::cout << "Something went wrong while creating Framebuffer" << std::endl;

	// Bind default framebuffer to avoid undesired behaviour.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Set isCreated to avoid deleting unexisting framebuffer.
	isCreated = true;
}

// Safe to call even if it hasn't been created.
void PostProcessing::deleteBuffers() {
	if (isCreated) {
		glDeleteFramebuffers(1, &fbo);
		glDeleteTextures(1, &textureColorbuffer);
		glDeleteRenderbuffers(1, &rbo);
		isCreated = false;
	}
}

void PostProcessing::draw() {

	glViewport(0, 0, getWindowWidth(), getWindowHeight());

	// Bind default framebuffer, aka the one that gets outputed to the screen.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program.getShaderID());

	// Set offset uniforms in shader.
	// Used in texture coordinates to obtain adjacent pixels.
	// Remember to use 1.0f in division otherwise it is divisione between two integers
	// and the result will round down to 0.
	program.setFloat("offsetX", 1.0f / getWindowWidth());
	program.setFloat("offsetY", 1.0f / getWindowHeight());

	// Set actual post processing parameters such as exposure and gamma.
	// Set also if they have to be used.
	program.setBool("usePostProcessing", usePostProcessing);
	program.setBool("useGammacorrection", useGammacorrection);
	program.setBool("useTonemapping", useTonemapping);
	program.setBool("useBlackAndWhite", useBlackAndWhite);
	program.setBool("useBloom", useBloom);
	program.setFloat("bloomBias", Bloom::getBloomBias());
	program.setFloat("exposure", exposure);
	program.setFloat("gamma", gamma);

	glDisable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	// Set texture unit and texture.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);

	// Set texture unit and texture of bloom blurred texture, only if bloom is enabled.
	if (useBloom) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Bloom::getBlurredTexture());
	}

	// Draw actual quad.
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void PostProcessing::applyEffects() {

	// Bloom before gaussian blur (or opposite if we want different effect).
	if (useBloom)
		Bloom::bloomPass(textureColorbuffer);

	if (useGaussianBlur)
		GaussianBlur::blur(fbo, textureColorbuffer, getWindowWidth(), getWindowHeight());
}

void PostProcessing::initializePostProcessing() {
	// Initialize shader program.
	program = Shader(project_directory + "\\shaders\\vshader_post.glsl", project_directory + "\\shaders\\fshader_post.glsl");

	// Generate vao.
	glGenVertexArrays(1, &vao);

	// Make sure buffers are created even if Window.cpp framebuffer_callback doesn't get called.
	// This makes sure screen isn't black at start of execution.
	// We call deleteBuffers() before so that if framebuffer_callback gets called we don't memory leak previous framebuffer.
	deleteBuffers();
	createBuffers();

	// Initialize gaussian blur.
	// Can be moved elsewhere, maybe in Renderer.cpp.
	GaussianBlur::initialize();

	// Initialize bloom.
	Bloom::initialize();
}

void PostProcessing::terminate() {
	// Delete buffers and terminate gaussian blur and bloom.
	PostProcessing::deleteBuffers();
	GaussianBlur::terminate();
	Bloom::destroy();
}

Shader& PostProcessing::getProgram() {
	return program;
}

unsigned int PostProcessing::getFramebufferId() {
	return fbo;
}

void PostProcessing::setUseGammacorrection(bool b) {
	useGammacorrection = b;
}

void PostProcessing::setUseTonemapping(bool b) {
	useTonemapping = b;
}

void PostProcessing::setExposure(float f) {
	exposure = f;
}

void PostProcessing::setGamma(float f) {
	gamma = f;
}

void PostProcessing::setUsePostProcessing(bool b)
{
	usePostProcessing = b;
}

void PostProcessing::setUseBlackAndWhite(bool b)
{
	useBlackAndWhite = b;
}

void PostProcessing::setUseGaussianBlur(bool b) {
	useGaussianBlur = b;
}

void PostProcessing::setUseBloom(bool b) {
	useBloom = b;
}

bool PostProcessing::getUseTonemapping()
{
	return useTonemapping;
}

bool PostProcessing::getUseGammacorrection()
{
	return useGammacorrection;
}

bool PostProcessing::getUsePostProcessing()
{
	return usePostProcessing;
}

bool PostProcessing::getUseBlackAndWhite()
{
	return useBlackAndWhite;
}

bool PostProcessing::getUseGaussianBlur() {
	return useGaussianBlur;
}

bool PostProcessing::getUseBloom() {
	return useBloom;
}

float PostProcessing::getExposure()
{
	return exposure;
}

float PostProcessing::getGamma()
{
	return gamma;
}

