#include "Bloom.h"
#include <iostream>
#include "BloomMip.h"
// Glad prima di glfw.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "window/Window.h"
#include <vector>
#include "shader/Shader.h"
#include "ProjectDirectory.h"

static bool buffersInitialized = false, usePrefilter = true;
static float bloomBias = 0.05f, prefilterThreshold = 0.75f;
static unsigned int fbo, vao;
static int lastWidth, lastHeight;
static std::vector<BloomMip> mipMapChain;
static Shader downProgram, upProgram;

static void initializeBuffers();
static void renderDownsamples(unsigned int);
static void renderUpsamples();

void Bloom::initialize() {

	// Create shader programs.
	downProgram = Shader(project_directory + "\\shaders\\vshader_post.glsl", project_directory + "\\shaders\\fshader_bloom_downsampling.glsl");
	upProgram = Shader(project_directory + "\\shaders\\vshader_post.glsl", project_directory + "\\shaders\\fshader_bloom_upsampling.glsl");

	// lastWidth and lastHeight are used to determine if we have to update texture sizes.
	// If they stay the same from a frame to another nothing is done to the textures.
	lastWidth = getWindowWidth();
	lastHeight = getWindowHeight();

	// Initialize textures and buffers.
	initializeBuffers();
}

void Bloom::bloomPass(unsigned int srcTexture) {

	// First check if size has changed.
	// If so, delete previous textures and buffers and create new one.
	// Also, change lastWidth and lastHeight if it has changed.
	// Note that we only actually care about width.
	if (getWindowWidth() != lastWidth || getWindowHeight() != lastHeight) {
		lastWidth = getWindowWidth();
		lastHeight = getWindowHeight();
		// Destroy and reinitialize.
		destroy();
		initializeBuffers();
	}

	// Apply bloom.

	// Bind fbo (important) and set gl parameters.
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glDisable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	renderDownsamples(srcTexture);
	renderUpsamples();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void renderDownsamples(unsigned int srcTexture) {

	glUseProgram(downProgram.getShaderID());
	downProgram.setVec2("srcResolution", glm::vec2((float)lastWidth, (float)lastHeight));
	downProgram.setBool("usePrefilter", usePrefilter);
	downProgram.setFloat("prefilterThreshold", prefilterThreshold);

	// Bind srcTexture (HDR color buffer) as initial texture input
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, srcTexture);

	// Progressively downsample through the mip chain
	for (int i = 0; i < mipMapChain.size(); i++)
	{
		// Set mipLevel in the shader.
		downProgram.setInt("mipLevel", i);

		const BloomMip& mip = mipMapChain[i];
		glViewport(0, 0, mip.size.x, mip.size.y);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, mip.texture, 0);

		// Render screen-filled quad of resolution of current mip
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		// Set current mip resolution as srcResolution for next iteration
		downProgram.setVec2("srcResolution", mip.size);
		// Set current mip as texture input for next iteration
		glBindTexture(GL_TEXTURE_2D, mip.texture);
	}
	
	glUseProgram(0);
}

static void renderUpsamples() {

	glUseProgram(upProgram.getShaderID());

	// Enable additive blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	for (int i = mipMapChain.size() - 1; i > 0; --i)
	{
		const BloomMip& mip = mipMapChain[i];
		const BloomMip& nextMip = mipMapChain[i - 1];

		// Set filterRadius for each mipmap so that texture fetches adjacent pixels both in x and y.
		glm::vec2 filterRadius(1.0f / mip.intSize.x, 1.0f / mip.intSize.y);
		upProgram.setVec2("filterRadius", filterRadius);

		// Bind viewport and texture from where to read
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mip.texture);

		// Set framebuffer render target (we write to this texture)
		glViewport(0, 0, nextMip.size.x, nextMip.size.y);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, nextMip.texture, 0);

		// Render screen-filled quad of resolution of current mip
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	// Disable additive blending
	glDisable(GL_BLEND);

	glUseProgram(0);
}

static void initializeBuffers() {

	// Return and don't continue if it is already initialized.
	if (buffersInitialized)
		return;

	// Generate vao.
	glGenVertexArrays(1, &vao);

	// Initialize and bind.
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// mipChainLength should be changed based on resolution.
	// The final mipmap size, aka the smallest, should be in the 0-100 range.
	int mipChainLength = 0;
	// Width is the only thing used to determine how many mips to create.
	// Height doesn't matter.
	int mipmapFinalWidth = getWindowWidth();
	// Use do while so there is at least one mip.
	do {
		mipChainLength++;
		mipmapFinalWidth /= 2;
	} while (mipmapFinalWidth >= 60);

	glm::vec2 mipSize((float)lastWidth, (float)lastHeight);
	glm::vec2 mipIntSize(lastWidth, lastHeight);

	// IMPORTANT!
	// Clean mipMapChain before creating it.
	mipMapChain.clear();
	
	for (int i = 0; i < mipChainLength; ++i) {

		BloomMip mip;

		mipSize *= 0.5f;
		mipIntSize /= 2;
		mip.size = mipSize;
		mip.intSize = mipIntSize;

		glGenTextures(1, &mip.texture);
		glBindTexture(GL_TEXTURE_2D, mip.texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, (int)mipSize.x, (int)mipSize.y, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		mipMapChain.emplace_back(mip);
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mipMapChain[0].texture, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// Check completion status.
	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		printf("gbuffer FBO error, status: 0x\%x\n", status);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Set initialized state.
	buffersInitialized = true;
}

// Safe to call even if it is not initialized.
// All it has to do is delete textures and buffers.
// This method is called both when terminating post processing 
// and when updating textures sizes on screen size change.
void Bloom::destroy() {

	// Return and don't continue if it is not initialized.
	if (!buffersInitialized)
		return;
		
	// Destroy vao.
	glDeleteVertexArrays(1, &vao);

	// Destroy buffers and textures.
	for (int i = 0; i < mipMapChain.size(); ++i) {
		glDeleteTextures(1, &mipMapChain[i].texture);
	}
	glDeleteFramebuffers(1, &fbo);

	// Set initialized state.
	buffersInitialized = false;
}

unsigned int Bloom::getBlurredTexture() {
	return mipMapChain[0].texture;
}

bool Bloom::getUsePrefilter() {
	return usePrefilter;
}

void Bloom::setUsePrefilter(bool b) {
	usePrefilter = b;
}

float Bloom::getBloomBias() {
	return bloomBias;
}

void Bloom::setBloomBias(float f) {
	bloomBias = f;
}

float Bloom::getPrefilterThreshold() {
	return prefilterThreshold;
}

void Bloom::setPrefilterThreshold(float f) {
	prefilterThreshold = f;
}