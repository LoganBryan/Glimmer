#include "Shader.h"

Shader::Shader()
{
}

void Shader::Load(const char* vertexPath, const char* fragmentPath)
{
	CompileAndLink(vertexPath, fragmentPath, nullptr);
}

void Shader::Load(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
{
	CompileAndLink(vertexPath, fragmentPath, geometryPath);
}

void Shader::CompileAndLink(const char* vPath, const char* fPath, const char* gPath)
{
	auto readFile = [](const char* path)
		{
			std::ifstream file(path);
			if (!file.is_open())
			{
				std::cerr << "Failed to open shader file: " << path << "!" << std::endl;
				return std::string{};
			}
			std::stringstream ss;
			ss << file.rdbuf();
			return ss.str();
		};

	std::string vCode = readFile(vPath);
	std::string fCode = readFile(fPath);
	std::string gCode;
	if (gPath)
		gCode = readFile(gPath);

	const char* vSrc = vCode.c_str();
	const char* fSrc = fCode.c_str();
	const char* gSrc = gPath ? gCode.c_str() : nullptr;

	unsigned int vertComp = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fragComp = glCreateShader(GL_FRAGMENT_SHADER);
	unsigned int geomComp = 0;

	glShaderSource(vertComp, 1, &vSrc, nullptr);
	glCompileShader(vertComp);
	CheckShaderCompilation(vertComp, "Vertex");

	glShaderSource(fragComp, 1, &fSrc, nullptr);
	glCompileShader(fragComp);
	CheckShaderCompilation(fragComp, "Fragment");

	if (gSrc)
	{
		geomComp = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geomComp, 1, &gSrc, nullptr);
		glCompileShader(geomComp);
		CheckShaderCompilation(geomComp, "Geometry");
	}

	programID = glCreateProgram();
	glAttachShader(programID, vertComp);
	glAttachShader(programID, fragComp);
	if (gSrc)
		glAttachShader(programID, geomComp);
	glLinkProgram(programID);
	CheckProgramCompilation(programID);

	glDeleteShader(vertComp);
	glDeleteShader(fragComp);
	if (gSrc)
		glDeleteShader(geomComp);
}

void Shader::CheckShaderCompilation(unsigned int& shader, std::string shaderType)
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

void Shader::CheckProgramCompilation(unsigned int& program)
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