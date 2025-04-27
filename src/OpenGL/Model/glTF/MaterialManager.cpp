#include "MaterialManager.h"

void MaterialManager::BuildMaterials(const fastgltf::Asset& asset)
{
	mUBOs.clear();
	mUBOs.reserve(asset.materials.size() + 1);

	// Default material
	GLuint ubo;
	glCreateBuffers(1, &ubo);
	MaterialUniforms def{};
	glNamedBufferStorage(ubo, sizeof(def), &def, GL_DYNAMIC_STORAGE_BIT);
	mUBOs.push_back(ubo);

	// Setup materials
	for (auto& mat : asset.materials)
	{
		MaterialUniforms uni{};
		uni.alphaCutoff = mat.alphaCutoff;
		uni.baseColorFactor = mat.pbrData.baseColorFactor;
		uni.metallicFactor = mat.pbrData.metallicFactor;
		uni.roughnessFactor = mat.pbrData.roughnessFactor;

		// Set flags
		if (mat.pbrData.baseColorTexture)
			uni.flags |= HasBaseColorTexture;
		if (mat.pbrData.metallicRoughnessTexture)
			uni.flags |= HasMetallicRoughnessTexture;
		if (mat.normalTexture)
			uni.flags |= HasNormalTexture;
		if (mat.emissiveTexture)
			uni.flags |= HasEmissiveTexture;
		if (mat.occlusionTexture)
			uni.flags |= HasOcclusionTexture;

		GLuint ubo;
		glCreateBuffers(1, &ubo);
		glNamedBufferStorage(ubo, sizeof(uni), &uni, GL_DYNAMIC_STORAGE_BIT);
		mUBOs.push_back(ubo);
	}
}
