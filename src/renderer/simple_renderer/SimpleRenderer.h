#pragma once
#include "model/ModelInstance.h"
#include <vector>
#include "shader/Shader.h"
#include "scene/Scene.h"

namespace SimpleRenderer {
	void render();
	void initRenderer();
	void terminateRenderer();
	void setUsePbr(bool);
	bool getUsePbr();
	const Shader& getProgram();
	float getNearPlane();
	float getFarPlane();
	float getFov();
	Scene& getScene();
	void setScene(Scene);
}