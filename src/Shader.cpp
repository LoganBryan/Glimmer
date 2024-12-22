#include "Shader.h"

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
	// Recieve source from path
	std::string vertexSource;
	std::string fragmentSource;
	
	std::ifstream vertFile;
	std::ifstream fragFile;
	// Allow objects to throw exceptions
	vertFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fragFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		vertFile.open(vertexPath);
		fragFile.open(fragmentPath);
		std::stringstream vertStream, fragStream;
		// Read buffer content into streams
		vertStream << vertFile.rdbuf();
		fragStream << fragFile.rdbuf();
		vertFile.close();
		fragFile.close();
		// Stream to string
		vertexSource = vertStream.str();
		fragmentSource = fragStream.str();
	}
	catch(std::ifstream::failure e)
	{
		printf("Shader file failed to read!");
	}
	const char* vertexShader = vertexSource.c_str();
	const char* fragmentShader = fragmentSource.c_str();

	// Compile shaders
	unsigned int vertex, fragment;

	// Vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertexShader, NULL);
	glCompileShader(vertex);
	checkShaderCompilation(vertex, "Vertex");

	// Fragment shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragmentShader, NULL);
	glCompileShader(fragment);
	checkShaderCompilation(fragment, "Fragment");

	// Shader program
	programID = glCreateProgram();
	glAttachShader(programID, vertex);
	glAttachShader(programID, fragment);
	glLinkProgram(programID);
	checkProgramCompilation(programID);

	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

void Shader::checkShaderCompilation(unsigned int& shader, std::string shaderType)
{
	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		printf("%s shader failed to compile! %s\n", shaderType.c_str(), infoLog);
	}
}

void Shader::checkProgramCompilation(unsigned int& program)
{
	int success;
	char infoLog[512];
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		printf("Shader program failed! %s\n", infoLog);
	}

}