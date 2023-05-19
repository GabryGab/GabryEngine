#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader {
private:
	unsigned int shaderID;
public:
	Shader(const std::string& vertexPath, const std::string& fragmentPath);
	Shader(const std::string& vertexPath, const std::string& geometryPath, const std::string& fragmentPath);
	Shader(const Shader& s) : shaderID(s.shaderID) {};
	Shader() = default;	
	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
	void setMat4(const std::string& name, const glm::mat4& mat) const;
	void setMat3(const std::string& name, const glm::mat3& mat) const;
	void setVec3(const std::string& name, const glm::vec3& vec) const;
	void setVec2(const std::string& name, const glm::vec2& vec) const;
	unsigned int getShaderID() const {
		return shaderID;
	}
};
