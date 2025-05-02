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

	std::shared_ptr<std::vector<MeshGPU>> GetOrLoadMeshes(const std::filesystem::path& path);
	size_t LoadMaterialsFromAsset(const std::filesystem::path& path);
	std::shared_ptr<Skin> GetOrLoadSkin(const std::filesystem::path& path);

	std::shared_ptr<fastgltf::Asset> GetParsedAsset(const std::filesystem::path& path);
	inline const std::vector<MeshGPU>* GetMeshesPtr(const std::string& key) const
	{
		auto it = mMeshMap.find(key);
		return (it != mMeshMap.end()) ? it->second.get() : nullptr;
	}

	void Clear();
	glTFParser parser;
	TextureManager texManager;
	MaterialManager matManager;
	SkinManager skinManager;

private:

	MeshUploader uploader;

	std::unordered_map<std::string, std::shared_ptr<fastgltf::Asset>> mParsedAssetCache;

	std::unordered_map<std::string, std::shared_ptr<std::vector<MeshGPU>>> mMeshMap;
	std::unordered_map<std::string, std::shared_ptr<Skin>> mSkinMap;
};

