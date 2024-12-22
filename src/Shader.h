#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

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
	inline void SetFloat(const std::string& name, float value) const { glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)(value)); }

private:
	void checkShaderCompilation(unsigned int& shader, std::string shaderType);
	void checkProgramCompilation(unsigned int& program);

private:
	unsigned int programID;

};

#endif // !SHADER_H
