#pragma once
#include <glm/glm.hpp>

enum class LightUpdate {
	FALSE, POSITION, AMBIENT, DIFFUSE, SPECULAR, CONSTANT, LINEAR, QUADRATIC, MULTIPLE
};

class Light {
	typedef glm::vec3 vec3;
private:
	vec3 position, ambient, diffuse, specular;
	float constant, linear, quadratic;
	LightUpdate update;

	void updateUpdate(LightUpdate lu) {
		if (update == LightUpdate::FALSE)
			update = lu;
		else
			update = LightUpdate::MULTIPLE;
	}
public:
	Light(const vec3& pos) : position(pos), ambient(vec3(0.1f, 0.1f, 0.1f)),
		diffuse(vec3(1.0f, 1.0f, 1.0f)), specular(vec3(0.5f, 0.5f, 0.5f)),
		update(LightUpdate::MULTIPLE), constant(1.0f), linear(0.7f), quadratic(1.8f) { }

	Light() : position(vec3(0, 0, 0)), ambient(vec3(0.1f, 0.1f, 0.1f)),
		diffuse(vec3(1.0f, 1.0f, 1.0f)), specular(vec3(0.5f, 0.5f, 0.5f)),
		update(LightUpdate::MULTIPLE), constant(1.0f), linear(0.7f), quadratic(1.8f) { }

	Light(const vec3& amb, const vec3& dif, const vec3& spec,
		const float& con, const float& lin, const float& qua) :
		position(vec3(0, 0, 0)), ambient(amb), diffuse(dif), specular(spec),
		update(LightUpdate::MULTIPLE), constant(con), linear(lin), quadratic(qua) { }

	Light(const vec3& pos, const vec3& amb, const vec3& dif, const vec3& spec,
		const float& con, const float& lin, const float& qua) :
		position(pos), ambient(amb), diffuse(dif), specular(spec),
		update(LightUpdate::MULTIPLE), constant(con), linear(lin), quadratic(qua) { }

	Light(const Light& l) : position(l.position), ambient(l.ambient),
		diffuse(l.diffuse), specular(l.specular),
		update(LightUpdate::MULTIPLE), constant(l.constant),
		linear(l.linear), quadratic(l.quadratic) { }

	Light& operator=(const Light& l) {
		position = l.position;
		ambient = l.ambient;
		diffuse = l.diffuse;
		specular = l.specular;
		constant = l.constant;
		linear = l.linear;
		quadratic = l.quadratic;
		return *this;
	}

	vec3 getPosition() const { return position; }
	vec3 getAmbient() const { return ambient; }
	vec3 getDiffuse() const { return diffuse; }
	vec3 getSpecular() const { return specular; }
	float getConstant() const { return constant; }
	float getLinear() const { return linear; }
	float getQuadratic() const { return quadratic; }
	LightUpdate getUpdate() const { return update; }

	void setPosition(const vec3& pos) {
		position = pos;
		updateUpdate(LightUpdate::POSITION);
	}
	void setAmbient(const vec3& amb) {
		ambient = amb;
		updateUpdate(LightUpdate::AMBIENT);
	}
	void setDiffuse(const vec3& dif) {
		diffuse = dif;
		updateUpdate(LightUpdate::DIFFUSE);
	}
	void setSpecular(const vec3& spe) {
		specular = spe;
		updateUpdate(LightUpdate::SPECULAR);
	}
	void setConstant(const float& con) {
		constant = con;
		updateUpdate(LightUpdate::CONSTANT);
	}
	void setLinear(const float& lin) {
		linear = lin;
		updateUpdate(LightUpdate::LINEAR);
	}
	void setQuadratic(const float& qua) {
		quadratic = qua;
		updateUpdate(LightUpdate::QUADRATIC);
	}
	void resetUpdate() {
		update = LightUpdate::FALSE;
	}
	void setUpdate(const LightUpdate& lu) {
		update = lu;
	}
	void print();
};