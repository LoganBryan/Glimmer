#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include <OpenGL/Model/GltfLoaderData.h>

#include <iostream>

struct MeshGPU
{
	GLuint vao;
	GLuint vbo;
	GLuint ibo;
	GLuint indirectBuffer;
	size_t drawCount;
};

class MeshUploader
{
public:
	std::vector<MeshGPU> UploadMeshes(const fastgltf::Asset& asset);
};
