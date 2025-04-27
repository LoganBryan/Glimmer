#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <iostream>
#include <thread>

class TextureManager
{
public:
	GLuint CreatePlaceholder();
	void AsyncLoad(GLuint textureID, const std::string& path);
};
