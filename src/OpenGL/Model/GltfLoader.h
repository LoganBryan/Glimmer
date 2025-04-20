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
#include "OpenGL/Application/Shader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

// TODO: a lot of this class is slow/ inefficent with retrieving, setting, binding etc - need to improve most of it!
class GltfLoader
{
public:
	GltfLoader();
	~GltfLoader();

	bool LoadModel(std::filesystem::path filePath, Shader& shader);
	void DrawModel(Shader& shader, glm::mat4& objectTransform);
	void DrawShadowVolume(Shader& shader, glm::mat4& objectTransform);

	void UpdateSkins(glm::mat4& objectTransform);
	void UpdateJointHeirarchy(Skin& skin, Joint& joint);
	bool UpdateJointTransform(const std::string& jointName, const glm::mat4& transform);

	glm::mat4& GetBindPoseTranslation(const std::string& jointName);

	// TODO: eventually remove this from loader, it should be handled by a scene manager or object class. This class should only be responsible for model data
	inline Transform& GetTransform() { return transform; }
private:
	Viewer viewer;
	Transform transform;
	glm::mat4 relativeTransform;
	bool isGrabbed = false;

	bool LoadFromPath(std::filesystem::path filePath);
	bool LoadMeshData(fastgltf::Mesh& mesh);
	bool LoadMaterial(fastgltf::Material& material);

	bool LoadSkin(fastgltf::Skin& skin);

	bool LoadImage(fastgltf::Image& image);

	void DrawMesh(std::size_t meshIndex);
	void DrawShadowVolumeMesh(std::size_t meshIndex);
};

#endif // !GLTFLOADER_H

