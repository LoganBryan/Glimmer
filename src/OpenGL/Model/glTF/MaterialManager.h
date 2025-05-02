#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <type_traits>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include <OpenGL/Model/GltfLoaderData.h>
#include <stb_image.h>

#include "TextureManager.h"

class MaterialManager
{
public:
	MaterialManager();
	~MaterialManager();

	size_t AddMaterials(const fastgltf::Asset& asset, const std::filesystem::path& path, ResourceCache& cache, TextureManager& texManager);

	inline const GLuint GetMaterialSSBO() const { return mMaterialSSBO; }
	inline const size_t GetMaterialCount() const { return mMaterialsData.size(); }

	void UpdateMaterialTextureHandle(size_t globalMaterialIndex, const std::string& texPath, uint64_t newHandle);

	std::optional<size_t> GetGlobalMaterialIndex(const std::string& key, size_t assetMaterialId) const;

	std::unordered_map<std::string, std::vector<std::pair<size_t, TextureType>>> textureUsageMap;

private:
	void EnsureSSBOSize(size_t reqCapacity);
	GLuint mMaterialSSBO = 0;
	size_t mSSBOCapacity = 0;

	std::vector<MaterialUniforms> mMaterialsData;

	std::unordered_map<std::string, std::pair<size_t, size_t>> mAssetMatMap;
};
