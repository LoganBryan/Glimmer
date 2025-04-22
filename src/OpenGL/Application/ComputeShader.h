#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

class ComputeShader
{
public:
	ComputeShader() : programID(0) {}
	~ComputeShader() { Delete(); }
	ComputeShader(const ComputeShader&) = delete;
	ComputeShader& operator=(const ComputeShader&) = delete;

	inline void Load(const char* computePath) { CompileAndLink(computePath); }
	void Use() const { glUseProgram(programID); }
	void Dispatch(GLuint x, GLuint y, GLuint z) const;
	inline void Delete()
	{
		if (programID)
		{
			glDeleteProgram(programID);
			programID = 0;
		}
	}

	inline void SetBool(const std::string& name, bool value) { glUniform1i(GetUniformLocation(name), value); }
	inline void SetInt(const std::string& name, int value) { glUniform1i(GetUniformLocation(name), value); }
	inline void SetFloat(const std::string& name, float value) { glUniform1f(GetUniformLocation(name), (float)(value)); }

	inline void SetVec3(const std::string& name, const glm::vec3& value) { glUniform3fv(GetUniformLocation(name), 1, &value[0]); }
	inline void SetVec3(const std::string& name, float x, float y, float z) { glUniform3f(GetUniformLocation(name), x, y, z); }

	inline void SetVec4(const std::string& name, const glm::vec4& value) { glUniform4fv(GetUniformLocation(name), 1, &value[0]); }
	inline void SetVec4(const std::string& name, float x, float y, float z, float w) { glUniform4f(GetUniformLocation(name), x, y, z, w); }

	inline void SetMatrix4(const std::string& name, glm::mat4& value) { glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]); }

	GLuint GetID() const { return programID; }

private:
	void CompileAndLink(const char* path);
	GLuint programID;

	std::unordered_map<std::string, GLint> uniformLocationCache;
	inline GLint GetUniformLocation(const std::string& name)
	{
		if (uniformLocationCache.find(name) != uniformLocationCache.end())
			return uniformLocationCache[name];
		GLint location = glGetUniformLocation(programID, name.c_str());
		uniformLocationCache[name] = location;
		return location;
	}

};

