#pragma once
#include "Model.h"
#include <glm/glm.hpp>

// Class used to represent an instance of a model.
// So things like what model to use, its position, its rotation...
class ModelInstance {
private:
	float posX, posY, posZ;
	glm::vec3 scale, rotation;

	// drawable is used to know if ModelInstance is usable for drawing.
	// It is true only if everything needed for rendering has been defined, most notably a Model.
	// Remember, used only to determine if it is drawable, 
	// so it doesn't mean everything is initialized, only the necessary things.
	bool drawable;
	const Model* model;
	bool checkDrawability();
public:
	ModelInstance() : posX(0), posY(0), posZ(0), model(nullptr),
		scale(glm::vec3(1.0f, 1.0f, 1.0f)), rotation(glm::vec3(0.0f, 0.0f, 0.0f)), drawable(false) { };
	ModelInstance(const Model* m) : posX(0), posY(0), posZ(0), model(m),
		scale(glm::vec3(1.0f, 1.0f, 1.0f)), rotation(glm::vec3(0.0f, 0.0f, 0.0f)), drawable(true) { };
	ModelInstance(float x, float y, float z, const glm::vec3& rot, const glm::vec3& sca, const Model* m) :
		posX(x), posY(y), posZ(z), model(m), scale(sca), rotation(rot), drawable(true) { };
	ModelInstance(float x, float y, float z) : posX(x), posY(y), posZ(z), model(nullptr),
		scale(glm::vec3(1.0f, 1.0f, 1.0f)), rotation(glm::vec3(0.0f, 0.0f, 0.0f)), drawable(false) { };
	float getX() const {
		return posX;
	}
	float getY() const {
		return posY;
	}
	float getZ() const {
		return posZ;
	}
	const Model* getModel() const {
		return model;
	}
	void setPosition(const glm::vec3& pos) {
		posX = pos.x;
		posY = pos.y;
		posZ = pos.z;
	}
	glm::vec3 getScale() const {
		return scale;
	}
	void setScale(const glm::vec3& s) {
		scale = s;
	}
	glm::vec3 getRotation() const {
		return rotation;
	}
	void setRotation(const glm::vec3& r) {
		rotation = r;

	}
	void setModel(const Model* m) {
		model = m;
		// After setting model, which is required for rendering, we check if the model is now drawable, and set it.
		drawable = checkDrawability();
	}
	bool isDrawable() const { return drawable; }
};