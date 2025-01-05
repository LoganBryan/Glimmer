#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


class Shader
{
public:
	Shader(const char* vertexPath, const char* fragmentPath);

	inline void Use() const { glUseProgram(programID); }
	inline void Delete() const { glDeleteProgram(programID); }

	inline void SetBool(const std::string& name, bool value) const { glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)(value)); }
	inline void SetInt(const std::string& name, int value) const { glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)(value)); }
	inline void SetFloat(const std::string& name, float value) const { glUniform1f(glGetUniformLocation(programID, name.c_str()), (int)(value)); }

	inline void SetVec3(const std::string& name, glm::vec3& value) const { glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]); }
	inline void SetVec3(const std::string& name, float x, float y, float z) const { glUniform3f(glGetUniformLocation(programID, name.c_str()), x, y, z); }

	inline void SetMatrix4(const std::string& name, glm::mat4& value) const {glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &value[0][0]);}
	inline unsigned int GetID() const { return programID; }

private:
	void checkShaderCompilation(unsigned int& shader, std::string shaderType);
	void checkProgramCompilation(unsigned int& program);

private:
	unsigned int programID;

};

#endif // !SHADER_H
