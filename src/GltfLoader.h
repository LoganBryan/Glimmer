#pragma once
#ifndef GLTFLOADER_H
#define GLTFLOADER_H

#include <filesystem>
#include <iostream>
#include <vector>
#include <chrono>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <stb_image.h>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include "GltfLoaderData.h"
#include "Shader.h"

class GltfLoader
{
public:
	GltfLoader();
	~GltfLoader();

	bool LoadModel(std::filesystem::path filePath, Shader& shader);
	void DrawModel();

private:
	Viewer viewer;

	bool LoadFromPath(std::filesystem::path filePath);
	bool LoadMeshData(fastgltf::Mesh& mesh);
	bool LoadMaterial(fastgltf::Material& material);

	bool LoadImage(fastgltf::Image& image);

	void DrawMesh(std::size_t meshIndex);
};

#endif // !GLTFLOADER_H

