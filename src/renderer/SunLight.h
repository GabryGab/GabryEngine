#pragma once
#include <glm/glm.hpp>

enum class SunLightUpdate {
	FALSE, POSITION, AMBIENT, DIFFUSE, SPECULAR, MULTIPLE
};

class SunLight {
	typedef glm::vec3 vec3;
private:
	vec3 position, ambient, diffuse, specular;
	SunLightUpdate update;

	void updateUpdate(SunLightUpdate lu) {
		if (update == SunLightUpdate::FALSE)
			update = lu;
		else
			update = SunLightUpdate::MULTIPLE;
	}
public:
	SunLight() : position(vec3(0, 0, 0)), ambient(vec3(0.1f, 0.1f, 0.1f)),
		diffuse(vec3(1.0f, 1.0f, 1.0f)), specular(vec3(0.5f, 0.5f, 0.5f)),
		update(SunLightUpdate::MULTIPLE) { }

	SunLight(const vec3& pos, const vec3& amb, const vec3& dif, const vec3& spec) :
		position(pos), ambient(amb), diffuse(dif), specular(spec),
		update(SunLightUpdate::MULTIPLE) { }

	SunLight(const SunLight& l) : position(l.position), ambient(l.ambient),
		diffuse(l.diffuse), specular(l.specular),
		update(SunLightUpdate::MULTIPLE) { }

	SunLight& operator=(const SunLight& l) {
		position = l.position;
		ambient = l.ambient;
		diffuse = l.diffuse;
		specular = l.specular;
		return *this;
	}

	vec3 getPosition() { return position; }
	vec3 getAmbient() { return ambient; }
	vec3 getDiffuse() { return diffuse; }
	vec3 getSpecular() { return specular; }
	SunLightUpdate getUpdate() { return update; }

	void setPosition(const vec3& pos) {
		position = pos;
		updateUpdate(SunLightUpdate::POSITION);
	}
	void setAmbient(const vec3& amb) {
		ambient = amb;
		updateUpdate(SunLightUpdate::AMBIENT);
	}
	void setDiffuse(const vec3& dif) {
		diffuse = dif;
		updateUpdate(SunLightUpdate::DIFFUSE);
	}
	void setSpecular(const vec3& spe) {
		specular = spe;
		updateUpdate(SunLightUpdate::SPECULAR);
	}
	void resetUpdate() {
		update = SunLightUpdate::FALSE;
	}
	void setUpdate(const SunLightUpdate& lu) {
		update = lu;
	}
	void print();
};
