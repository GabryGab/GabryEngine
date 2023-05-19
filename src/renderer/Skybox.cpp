#include "Skybox.h"
#include <string>
#include <vector>
#include <glad/glad.h>
#include <stb_image.h>
#include "ProjectDirectory.h"
#include <iostream>
#include "shader/Shader.h"

static Shader program;
static unsigned int textureID, VAO, VBO;
static bool useSkybox = true;

void Skybox::initialize() {

	program = Shader(project_directory + "\\shaders\\vshader_skybox.glsl", project_directory + "\\shaders\\fshader_skybox.glsl");

	std::vector<std::string> faces{
		"right",
		"left",
		"top",
		"bottom",
		"front",
		"back"
	};

	std::string cubemapName = "learnopengl_sky";
	std::string extensionType = "png";

	// Initialize textures.
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (int i = 0; i < faces.size(); ++i) {

		unsigned char* data = stbi_load((project_directory + "\\assets\\cubemaps\\" +
			cubemapName + "\\" + faces[i] + "." + extensionType).c_str(), &width, &height, &nrChannels, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else {
			std::cout << "Cubemap texture loading failed." << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Generate vao and vbo.
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

	// vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glBindVertexArray(0);
}

void Skybox::draw(const glm::mat4& projection, const glm::mat4& view) {
	if (useSkybox) {
		// Prepare.
		glUseProgram(program.getShaderID());
		program.setMat4("projection", projection);
		glm::mat4 newView = glm::mat4(glm::mat3(view));
		program.setMat4("view", newView);

		// Settings.
		glDepthFunc(GL_LEQUAL);
		// Other settings are already set in prepareFrame of SimpleRenderer.
		// If any other step added in between changes them they need to be reset.

		// Draw.
		glBindVertexArray(VAO);
		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
}

void Skybox::terminate() {
	// Delete buffers.
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	// Don't know if it is correct.
	// Might have to delete each face.
	glDeleteTextures(1, &textureID);
}

bool Skybox::getUseSkybox() {
	return useSkybox;
}

void Skybox::setUseSkybox(bool b) {
	useSkybox = b;
}