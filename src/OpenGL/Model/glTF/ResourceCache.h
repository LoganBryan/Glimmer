#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <string>

#include "glTFParser.h"
#include "MeshUploader.h"
#include "TextureManager.h"
#include "MaterialManager.h"
#include "SkinManager.h"

class ResourceCache
{
public:
	ResourceCache() = default;
	~ResourceCache() = default;

	static ResourceCache& Get()
	{
		static ResourceCache instance;
		return instance;
	}
	ResourceCache(const ResourceCache&) = delete;
	ResourceCache& operator=(const ResourceCache&) = delete;
	ResourceCache(ResourceCache&&) = delete;
	ResourceCache& operator=(ResourceCache&&) = delete;

	std::shared_ptr<MeshGPU> GetOrLoadMesh(const std::filesystem::path& path);
	GLuint GetOrLoadTexture(const std::filesystem::path& path);
	std::vector<GLuint> GetOrLoadMaterials(const std::filesystem::path& path);
	std::shared_ptr<Skin> GetOrLoadSkin(const std::filesystem::path& path);

	void Clear();
	glTFParser parser;

private:

	MeshUploader uploader;
	TextureManager texManager;
	MaterialManager matManager;
	SkinManager skinManager;

	std::unordered_map<std::string, std::shared_ptr<MeshGPU>> meshMap;
	std::unordered_map<std::string, GLuint> textureMap;
	std::unordered_map<std::string, std::vector<GLuint>> materialsUBOsMap;
	std::unordered_map<std::string, std::shared_ptr<Skin>> skinMap;
};

