#include "ComputeShader.h"

void ComputeShader::CompileAndLink(const char* path)
{
	auto readFile = [](const char* path)
		{
			std::ifstream file(path);
			if (!file.is_open())
			{
				std::cerr << "Failed to open compute shader file: " << path << "!" << std::endl;
				return std::string{};
			}
			std::stringstream ss;
			ss << file.rdbuf();
			return ss.str();
		};

	std::string cCode = readFile(path);
	const char* cSrc = cCode.c_str();

	GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(shader, 1, &cSrc, nullptr);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char log[512];
		glGetShaderInfoLog(shader, 512, nullptr, log);
		std::cerr << "Compute shader compile error: " << log << std::endl;
	}

	programID = glCreateProgram();
	glAttachShader(programID, shader);
	glLinkProgram(programID);

	char infoLog[512];
	glGetProgramiv(programID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programID, 512, NULL, infoLog);
		printf("Compute shader program failed! %s\n", infoLog);
	}

	glDeleteShader(shader);
}

void ComputeShader::Dispatch(GLuint x, GLuint y, GLuint z) const
{
	glDispatchCompute(x, y, z);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}