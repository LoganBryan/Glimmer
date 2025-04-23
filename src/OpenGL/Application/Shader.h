#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <glm/gtc/type_ptr.hpp>

#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


class Shader
{
public:
	Shader();
	~Shader()
	{
		Delete();
	}
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	void Load(const char* vertexPath, const char* fragmentPath);
	void Load(const char* vertexPath, const char* geometryPath, const char* fragmentPath);

	inline void Use() const { glUseProgram(programID); }
	inline void End() const { glUseProgram(0); }

	inline void Delete() 
	{ 
		if (programID != 0)
		{
			glDeleteProgram(programID);
			programID = 0;
		}
	}

	inline void SetBool(const std::string& name, bool value) { glUniform1i(GetUniformLocation(name), value); }
	inline void SetInt(const std::string& name, int value) { glUniform1i(GetUniformLocation(name), value); }
	inline void SetFloat(const std::string& name, float value) { glUniform1f(GetUniformLocation(name), (float)(value)); }

	inline void SetVec2(const std::string& name, const glm::vec3& value) { glUniform2fv(GetUniformLocation(name), 1, &value[0]); }
	inline void SetVec2(const std::string& name, float x, float y) { glUniform2f(GetUniformLocation(name), x, y); }

	inline void SetVec3(const std::string& name, const glm::vec3& value) { glUniform3fv(GetUniformLocation(name), 1, &value[0]); }
	inline void SetVec3(const std::string& name, float x, float y, float z) { glUniform3f(GetUniformLocation(name), x, y, z); }

	inline void SetVec4(const std::string& name, const glm::vec4& value) { glUniform4fv(GetUniformLocation(name), 1, &value[0]); }
	inline void SetVec4(const std::string& name, float x, float y, float z, float w) { glUniform4f(GetUniformLocation(name), x, y, z, w); }

	inline void SetMatrix4(const std::string& name, glm::mat4& value) {glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);}
	
	inline void SetBlockBinding(const std::string& name, GLuint bindingPoint) const 
	{ 
		GLuint blockIndex = glGetUniformBlockIndex(programID, name.c_str()); 
		if (blockIndex != GL_INVALID_INDEX)
			glUniformBlockBinding(programID, blockIndex, bindingPoint);
	}

	inline unsigned int GetID() const { return programID; }

private:
	void CompileAndLink(const char* vPath, const char* fPath, const char* gPath = nullptr);
	void CheckShaderCompilation(unsigned int& shader, std::string shaderType);
	void CheckProgramCompilation(unsigned int& program);

	std::unordered_map<std::string, GLint> uniformLocationCache;
	inline GLint GetUniformLocation(const std::string& name)
	{
		if (uniformLocationCache.find(name) != uniformLocationCache.end())
			return uniformLocationCache[name];
		GLint location = glGetUniformLocation(programID, name.c_str());
		uniformLocationCache[name] = location;
		return location;
	}

private:
	unsigned int programID;

};

#endif // !SHADER_H
