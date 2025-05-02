#include "SceneRenderer.h"

void SceneRenderer::Draw(Shader& shader, const InstanceManager& instances, ResourceCache& cache, MaterialManager& mats, const SkinManager& skins, unsigned int skyboxTexture)
{
	if (instances.Count() == 0) return;

	std::vector<std::string> loadedTextures = cache.texManager.ProcessUploadQueue();

	if (!loadedTextures.empty())
	{
		std::cout << "[SceneRenderer] Info! " << loadedTextures.size() << " textures finished loaded. Updating materials.. \n";

		for (const std::string& texPath : loadedTextures)
		{
			auto it = mats.textureUsageMap.find(texPath);
			if (it != mats.textureUsageMap.end())
			{
				uint64_t newHandle = cache.texManager.GetTextureHandle(texPath);
				if (newHandle != 0 && newHandle != cache.texManager.GetPlaceholderHandle())
				{
					for (const auto& use : it->second)
					{
						size_t globalMatIndex = use.first;
						mats.UpdateMaterialTextureHandle(globalMatIndex, texPath, newHandle);
					}
				}
				else
					std::cerr << "[SceneRenderer] Warning! Got invalid handle for loaded texture " << texPath << "\n";
			}
		}
	}

	shader.Use();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, instances.GetSSBO());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mats.GetMaterialSSBO());

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	shader.SetInt("skybox", 5);

	/*glBindTextureUnit(0, cache.texManager.GetTextureArrayID(TextureType::Albedo));
	glBindTextureUnit(1, cache.texManager.GetTextureArrayID(TextureType::MetallicRoughness));
	glBindTextureUnit(2, cache.texManager.GetTextureArrayID(TextureType::Normal));
	glBindTextureUnit(3, cache.texManager.GetTextureArrayID(TextureType::Emissive));
	glBindTextureUnit(4, cache.texManager.GetTextureArrayID(TextureType::Occlusion));*/

	// Material and skinning data to be bound here

	mCommandBuffer.clear();
	for (const auto& group : instances.GetMeshGroups())
	{
		auto meshGpuVecPtr = cache.GetMeshesPtr(group.meshPath);
		if (!meshGpuVecPtr) continue;

		for (const auto& gpu : *meshGpuVecPtr)
		{
			if (gpu.drawCount == 0) continue;

			glBindVertexArray(gpu.vao);

			if (mCommandBuffer.size() < gpu.drawCount) mCommandBuffer.resize(gpu.drawCount);

			// Get original commands
			glGetNamedBufferSubData(gpu.indirectBuffer, 0, gpu.drawCount * sizeof(IndirectDrawCommand), mCommandBuffer.data());

			// Modify commands
			for (uint32_t i = 0; i < gpu.drawCount; i++)
			{
				mCommandBuffer[i].instanceCount = group.instanceCount;
				mCommandBuffer[i].baseInstance = group.instanceOffset;
			}

			GLsizeiptr reqSize = gpu.drawCount * sizeof(IndirectDrawCommand);

			if (mDynamicIndirectBuffer == 0)
			{
				glCreateBuffers(1, &mDynamicIndirectBuffer);
				glNamedBufferData(mDynamicIndirectBuffer, reqSize, mCommandBuffer.data(), GL_DYNAMIC_DRAW);
				mDynamicIndirectBufferSize = reqSize;
			}
			else if (reqSize > mDynamicIndirectBufferSize)
			{
				// Realloc
				glNamedBufferData(mDynamicIndirectBuffer, reqSize, mCommandBuffer.data(), GL_DYNAMIC_DRAW);
				mDynamicIndirectBufferSize = reqSize;
			}
			else
			{
				// Update existing buffer
				glNamedBufferSubData(mDynamicIndirectBuffer, 0, reqSize, mCommandBuffer.data());
			}

			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, mDynamicIndirectBuffer);
			glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*)0, static_cast<GLsizei>(gpu.drawCount), 0);
		}
	}

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindVertexArray(0);
}
