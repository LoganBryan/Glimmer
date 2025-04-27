#include "SceneRenderer.h"

void SceneRenderer::Draw(Shader& shader, const std::vector<MeshGPU>& meshes, const MaterialManager& mats, const SkinManager& skins, const InstanceManager& instances)
{
	shader.Use();

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, instances.GetSSBO());

	for (const auto& gpu : meshes)
	{
		glBindVertexArray(gpu.vao);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gpu.indirectBuffer);

		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*)0, static_cast<GLsizei>(gpu.drawCount), 0);
	}
}
