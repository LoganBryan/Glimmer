#include "SceneRenderer.h"

void SceneRenderer::Draw(Shader& shader, const InstanceManager& instances, const ResourceCache& cache, const MaterialManager& mats, const SkinManager& skins)
{
	if (instances.Count() == 0) return;

	shader.Use();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, instances.GetSSBO());

	// Material and skinning data to be bound here

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
