#include "MaterialManager.h"

MaterialManager::MaterialManager()
{
	mMaterialsData.emplace_back();
	EnsureSSBOSize(1);
	glNamedBufferSubData(mMaterialSSBO, 0, sizeof(MaterialUniforms), &mMaterialsData[0]);
}

MaterialManager::~MaterialManager()
{
	if (mMaterialSSBO != 0)
		glDeleteBuffers(1, &mMaterialSSBO);
}

size_t MaterialManager::AddMaterials(const fastgltf::Asset& asset, const std::filesystem::path& path, ResourceCache& cache, TextureManager& texManager)
{
	std::string key = path.generic_string();
	if (mAssetMatMap.count(key))
		return mAssetMatMap[key].first;

	size_t baseId = mMaterialsData.size();
	size_t matCount = asset.materials.size();
	size_t reqTotalMats = baseId + matCount;

	EnsureSSBOSize(reqTotalMats);

	std::vector<MaterialUniforms> assetMats;
	assetMats.reserve(matCount);

	uint64_t placeholderHandle = texManager.GetPlaceholderHandle();

	for (size_t i = 0; i < matCount; i++)
	{
		const auto& mat = asset.materials[i];
		MaterialUniforms uni{};

		// Factors
		uni.baseColorFactor = mat.pbrData.baseColorFactor;
		uni.emissiveFactor = mat.emissiveFactor;

		uni.metallicFactor = mat.pbrData.metallicFactor;
		uni.roughnessFactor = mat.pbrData.roughnessFactor;
		uni.normalScale = mat.normalTexture ? mat.normalTexture->scale : 1.0f;
		uni.occlusionStrength = mat.occlusionTexture ? mat.occlusionTexture->strength : 1.0f;
		uni.alphaCutoff = mat.alphaCutoff;

		// Get texture layer indices
		auto ResolveAndGetLayerId = [&](const auto& texInfoOpt, TextureType type, bool srgb) -> uint64_t
			{

				if (!texInfoOpt.has_value()) return placeholderHandle;
				const auto& texInfo = texInfoOpt.value();


				if (texInfo.textureIndex >= asset.textures.size()) return placeholderHandle;
				const auto& tex = asset.textures[texInfo.textureIndex];

				if (!tex.imageIndex || *tex.imageIndex >= asset.images.size()) return placeholderHandle;

				const auto& image = asset.images.at(*tex.imageIndex);
				size_t imageIndex = *tex.imageIndex;

				// Resolve path
				std::string imageIdentifier;
				bool handledSuccessfully = false;
				uint64_t resolvedHandle = placeholderHandle;

				std::visit(fastgltf::visitor{
					[&](const fastgltf::sources::URI& uri)
					{
						std::filesystem::path textureFilePath;
						bool pathResolved = false;
						if (uri.uri.isLocalPath() && !uri.uri.path().empty())
						{
							textureFilePath = path.parent_path() / std::filesystem::path(uri.uri.path().begin(), uri.uri.path().end()).lexically_normal();
							imageIdentifier = textureFilePath.generic_string();
							resolvedHandle = texManager.RequestTexture(textureFilePath, srgb, cache);
							pathResolved = true;
						}
						else
						{ 
							imageIdentifier = "unsupported_uri_" + std::string(uri.uri.string());
							std::cerr << "[MaterialManager] Warning! Unsupported URI " << imageIdentifier << "\n";
						}

						if (pathResolved)
						{
							uint64_t handle = texManager.RequestTexture(textureFilePath, srgb, cache);
							if (handle == placeholderHandle)
								textureUsageMap[imageIdentifier].push_back({ baseId + i, type });

							handledSuccessfully = true;
						}

					},
					[&](const fastgltf::sources::BufferView& buffView)
					{
						imageIdentifier = key + "_img" + std::to_string(imageIndex) + "_bv" + std::to_string(buffView.bufferViewIndex);

						auto it = texManager.mTextureCache.find(imageIdentifier);
						if (it != texManager.mTextureCache.end())
						{
							if (it->second.handle != 0 || it->second.isLoading)
							{
								if (it->second.handle == 0)
									textureUsageMap[imageIdentifier].push_back({ baseId + i, type });

								resolvedHandle = texManager.GetTextureHandle(imageIdentifier);
								handledSuccessfully = true;
								return;
							}
							it->second.isLoading = true;
						}
						else
							texManager.mTextureCache[imageIdentifier] = TextureInfo{ .isLoading = true };

						if (buffView.bufferViewIndex >= asset.bufferViews.size())
						{
							std::cerr << "[MaterialManager] Error! Invalid bufferViewIndex " << buffView.bufferViewIndex << " for image " << imageIndex << "\n";
							texManager.mTextureCache[imageIdentifier].isLoading = false;
							handledSuccessfully = false;
							return;
						}
						auto& viewDefinition = asset.bufferViews[buffView.bufferViewIndex];

						if (viewDefinition.bufferIndex >= asset.buffers.size())
						{
							std::cerr << "[MaterialManager] Error! Invalid bufferIndex " << viewDefinition.bufferIndex << " for image " << imageIndex << "\n";
							texManager.mTextureCache[imageIdentifier].isLoading = false;
							handledSuccessfully = false;
							return;
						}
						auto& bufferObject = asset.buffers[viewDefinition.bufferIndex];

						bool bufferProcessed = false;
						std::visit(fastgltf::visitor{
							[&](auto& dataSource)
							{
								fastgltf::span<const std::byte> entireBufferSpan;
								const char* realName = typeid(dataSource).name();

								using SourceType = std::decay_t<decltype(dataSource)>;

								if constexpr (std::is_same_v<SourceType, fastgltf::sources::Vector>)
									entireBufferSpan = fastgltf::span<const std::byte>(dataSource.bytes);
								else if constexpr (std::is_same_v<SourceType, fastgltf::sources::ByteView>)
									entireBufferSpan = dataSource.bytes;
								else if constexpr (std::is_same_v<SourceType, fastgltf::sources::Array>)
									entireBufferSpan = fastgltf::span<const std::byte>(dataSource.bytes);
								else
								{
									std::cerr << "[MaterialManager] Error! Unsupported buffer data source type\n";
									bufferProcessed = false;
									return;
								}

								// Check if span is valid
								if (entireBufferSpan.empty() && viewDefinition.byteLength > 0)
								{
									std::cerr << "[MaterialManager] Error! Buffer data is empty or inaccessible " << viewDefinition.bufferIndex << "\n";
									bufferProcessed = false;
									return;
								}

								// Check view bounds
								if (viewDefinition.byteOffset >= entireBufferSpan.size() || viewDefinition.byteOffset + viewDefinition.byteLength > entireBufferSpan.size())
								{
									std::cerr << "[MaterialManager] Error! BufferView range [" << viewDefinition.byteOffset << " + " << viewDefinition.byteLength << "] exceeds buffer size (" << entireBufferSpan.size() << ") for bufferView " << buffView.bufferViewIndex << "\n";
									bufferProcessed = false;
									return;
								}

								const unsigned char* imageDataPtr = reinterpret_cast<const unsigned char*>(entireBufferSpan.data() + viewDefinition.byteOffset);
								int imageDataLength = static_cast<int>(viewDefinition.byteLength);

								// Load img from mem
								int width = 0, height = 0, channels = 0;
								unsigned char* pixelDataFromMemory = stbi_load_from_memory(imageDataPtr, imageDataLength, &width, &height, &channels, 4);

								if (!pixelDataFromMemory)
								{
									std::cerr << "[MaterialManager] Error! stbi_load_from_memory FAILED for " << imageIdentifier << "\nWith Reason: " << stbi_failure_reason() << "\n";
									resolvedHandle = placeholderHandle;
									bufferProcessed = false;
								}
								else
								{
									texManager.QueueLoadedTexture(imageIdentifier, width, height, channels, pixelDataFromMemory, srgb);
									resolvedHandle = placeholderHandle;
									textureUsageMap[imageIdentifier].push_back({ baseId + i, type });
									bufferProcessed = true;
								}
							}
						}, bufferObject.data);

						handledSuccessfully = bufferProcessed;
						if (!handledSuccessfully)
							texManager.mTextureCache[imageIdentifier].isLoading = false;

					},
					[&](const fastgltf::sources::Array& arr)
					{
						imageIdentifier = key + "_img" + std::to_string(imageIndex) + "_arr";
						std::cerr << "[MaterialManager] Warning! Attempt to load from Array Source! This is currently unsupported! " << imageIdentifier << "\n";
						resolvedHandle = placeholderHandle;
						handledSuccessfully = false;

					},
					[](const auto& other) {}
					}, image.data);

					if (handledSuccessfully && !imageIdentifier.empty() && resolvedHandle == placeholderHandle)
					{
						uint64_t currentHandle = texManager.GetTextureHandle(imageIdentifier);
						if (currentHandle == placeholderHandle)
						{
							textureUsageMap[imageIdentifier].push_back({ baseId + i, type });
						}
						else
							resolvedHandle = currentHandle;
					}

					if (handledSuccessfully && resolvedHandle != placeholderHandle)
						return resolvedHandle;
					
					return placeholderHandle;
			};

			uni.baseColorTextureHandle = ResolveAndGetLayerId(mat.pbrData.baseColorTexture, TextureType::Albedo, true);
			uni.metallicRoughnessTextureHandle = ResolveAndGetLayerId(mat.pbrData.metallicRoughnessTexture, TextureType::MetallicRoughness, false);
			uni.normalTextureHandle = ResolveAndGetLayerId(mat.normalTexture, TextureType::Normal, false);
			uni.occlusionTextureHandle = ResolveAndGetLayerId(mat.occlusionTexture, TextureType::Occlusion, false);
			uni.emissiveTextureHandle = ResolveAndGetLayerId(mat.emissiveTexture, TextureType::Emissive, true);

			assetMats.push_back(uni);
		}
		
	// Upload to SSBO + store 
	mMaterialsData.insert(mMaterialsData.end(), assetMats.begin(), assetMats.end());
	size_t uploadByteOff = baseId * sizeof(MaterialUniforms);
	size_t uploadByteSize = assetMats.size() * sizeof(MaterialUniforms);
	if (uploadByteSize > 0)
		glNamedBufferSubData(mMaterialSSBO, uploadByteOff, uploadByteSize, assetMats.data());
	mAssetMatMap[key] = { baseId, matCount };

	return baseId;
}

void MaterialManager::UpdateMaterialTextureHandle(size_t globalMaterialIndex, const std::string& texPath, uint64_t newHandle)
{
	if (globalMaterialIndex >= mMaterialsData.size())
	{
		std::cerr << "[MaterialManager] Error! Attempt to update invalid material index " << globalMaterialIndex << "\n";
		return;
	}
	if (newHandle == 0)
	{
		std::cerr << "[MaterialManager] Warning! Attempt to update with an invalid handle (0) for texture " << texPath << "\n";
		return;
	}

	auto it = textureUsageMap.find(texPath);
	if (it == textureUsageMap.end()) return;

	TextureType updateTexture = TextureType::Albedo;
	bool found = false;
	for (const auto& use : it->second)
	{
		if (use.first == globalMaterialIndex)
		{
			updateTexture = use.second;
			found = true;
			break;
		}
	}

	if (!found)
	{
		std::cerr << "[MaterialManager] Warning! Could not find usage type for material " << globalMaterialIndex << " and texture " << texPath << "\n";
		return;
	}

	MaterialUniforms& mat = mMaterialsData[globalMaterialIndex];
	bool updated = false;

	switch (updateTexture)
	{
	case TextureType::Albedo:
		mat.baseColorTextureHandle = newHandle;
		updated = true;
		break;
	case TextureType::Normal:
		mat.normalTextureHandle = newHandle;
		updated = true;
		break;
	case TextureType::MetallicRoughness:
		mat.metallicRoughnessTextureHandle = newHandle;
		updated = true;
		break;
	case TextureType::Occlusion:
		mat.occlusionTextureHandle = newHandle;
		updated = true;
		break;
	case TextureType::Emissive:
		mat.emissiveTextureHandle = newHandle;
		updated = true;
		break;
	default:
		break;
	}

	if (updated)
	{
		// Calc offset and size for handle
		size_t fieldOffset = 0;
		switch (updateTexture)
		{
		case TextureType::Albedo:
			fieldOffset = offsetof(MaterialUniforms, baseColorTextureHandle);
			break;
		case TextureType::Normal:
			fieldOffset = offsetof(MaterialUniforms, normalTextureHandle);
			break;
		case TextureType::MetallicRoughness:
			fieldOffset = offsetof(MaterialUniforms, metallicRoughnessTextureHandle);
			break;
		case TextureType::Occlusion:
			fieldOffset = offsetof(MaterialUniforms, occlusionTextureHandle);
			break;
		case TextureType::Emissive:
			fieldOffset = offsetof(MaterialUniforms, emissiveTextureHandle);
			break;
		default:
			break;
		}

		size_t bufferOffset = globalMaterialIndex * sizeof(MaterialUniforms) + fieldOffset;
		size_t handleSize = sizeof(uint64_t);

		glNamedBufferSubData(mMaterialSSBO, bufferOffset, handleSize, &newHandle);
	}
}

std::optional<size_t> MaterialManager::GetGlobalMaterialIndex(const std::string& key, size_t assetMaterialId) const
{
	auto it = mAssetMatMap.find(key);
	if (it != mAssetMatMap.end())
	{
		const auto& mapInfo = it->second;
		if (assetMaterialId < mapInfo.second)
			return mapInfo.first + assetMaterialId;
	}

	return std::nullopt;
}

void MaterialManager::EnsureSSBOSize(size_t reqCapacity)
{
	if (reqCapacity <= mSSBOCapacity)
		return;

	size_t newCap = (mSSBOCapacity == 0) ? std::max(reqCapacity, (size_t)16) : mSSBOCapacity;
	while (newCap < reqCapacity)
	{
		newCap *= 2;
	}

	GLuint newSSBO;
	glCreateBuffers(1, &newSSBO);
	glNamedBufferStorage(newSSBO, newCap * sizeof(MaterialUniforms), nullptr, GL_DYNAMIC_STORAGE_BIT);

	if (mMaterialSSBO != 0 && !mMaterialsData.empty())
	{
		size_t copySize = std::min(mMaterialsData.size(), mSSBOCapacity) * sizeof(MaterialUniforms);
		if (copySize > 0)
			glCopyNamedBufferSubData(mMaterialSSBO, newSSBO, 0, 0, copySize);
		glDeleteBuffers(1, &mMaterialSSBO);
	}

	mMaterialSSBO = newSSBO;
	mSSBOCapacity = newCap;
}

