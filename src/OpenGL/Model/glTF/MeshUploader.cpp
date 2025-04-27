#include "MeshUploader.h"

std::vector<MeshGPU> MeshUploader::UploadMeshes(const fastgltf::Asset& asset)
{
	std::vector<MeshGPU> gpuMeshes;
	gpuMeshes.reserve(asset.meshes.size());

	for (const auto& mesh : asset.meshes)
	{
		MeshGPU gpu{};

		// Create and bind VAO
		glCreateVertexArrays(1, &gpu.vao);
		glBindVertexArray(gpu.vao);

		struct VertexLocal 
		{ 
			fastgltf::math::fvec3 position; 
			fastgltf::math::fvec3 normal; 
			fastgltf::math::fvec4 tangent; 
			fastgltf::math::fvec2 uv; 
			fastgltf::math::uvec4 joints; 
			fastgltf::math::fvec4 weights; 
		};
		std::vector<VertexLocal> vertices;
		std::vector<std::uint32_t> indices;
		std::vector<IndirectDrawCommand> cmds;
		vertices.reserve(1024);
		indices.reserve(1024);
		cmds.reserve(mesh.primitives.size());

		std::uint32_t vertexOffset = 0;
		std::uint32_t indexOffset = 0;

		for (const auto& prim : mesh.primitives)
		{
			auto posIt = prim.findAttribute("POSITION");
			assert(posIt != prim.attributes.end());
			const auto& posAcc = asset.accessors[posIt->accessorIndex];
			size_t vCount = posAcc.count;

			std::vector<fastgltf::math::fvec3> posData;
			std::vector<fastgltf::math::fvec3> normData;
			std::vector<fastgltf::math::fvec4> tangData;
			std::vector<fastgltf::math::fvec2> uvData;
			std::vector<fastgltf::math::uvec4> jointData;
			std::vector<fastgltf::math::fvec4> weightData;

			// Position
			fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, posAcc, [&](auto val)
				{
					posData.emplace_back(fastgltf::math::fvec3(val.x(), val.y(), val.z()));
				});

			// Normal
			if (auto normalIt = prim.findAttribute("NORMAL"); normalIt != prim.attributes.end())
			{
				const auto& acc = asset.accessors[normalIt->accessorIndex]; 
				normData.resize(vCount);
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, acc, [&](auto val, size_t i)
					{
						normData[i] = val;
					});
			}

			// Tangent
			if (auto tangentIt = prim.findAttribute("TANGENT"); tangentIt != prim.attributes.end())
			{
				const auto& acc = asset.accessors[tangentIt->accessorIndex];
				tangData.resize(vCount);
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, acc, [&](auto val, size_t i)
					{
						tangData[i] = val;
					});
			}

			// Texcoord
			if (auto texCoordIt = prim.findAttribute("TEXCOORD_0"); texCoordIt != prim.attributes.end())
			{
				const auto& acc = asset.accessors[texCoordIt->accessorIndex];
				uvData.resize(vCount);
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, acc, [&](auto val, size_t i)
					{
						uvData[i] = val;
					});
			}

			// Joints
			if (auto jointIt = prim.findAttribute("JOINTS_0"); jointIt != prim.attributes.end())
			{
				const auto& acc = asset.accessors[jointIt->accessorIndex];
				jointData.resize(vCount);
				fastgltf::iterateAccessorWithIndex<fastgltf::math::uvec4>(asset, acc, [&](auto val, size_t i)
					{
						jointData[i] = val;
					});
			}

			// Weights
			if (auto weightIt = prim.findAttribute("WEIGHTS_0"); weightIt != prim.attributes.end())
			{
				const auto& acc = asset.accessors[weightIt->accessorIndex];
				weightData.resize(vCount);
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, acc, [&](auto val, size_t i)
					{
						weightData[i] = val;
					});
			}

			// Build vertex list
			for (size_t i = 0; i < vCount; i++)
			{
				VertexLocal vert;
				vert.position = posData[i];
				vert.normal = (!normData.empty() ? normData[i] : fastgltf::math::fvec3(0));
				vert.tangent = (!tangData.empty() ? tangData[i] : fastgltf::math::fvec4(0));
				vert.uv = (!uvData.empty() ? uvData[i] : fastgltf::math::fvec2(0));
				vert.joints = (!jointData.empty() ? jointData[i] : fastgltf::math::uvec4(0));
				vert.weights = (!weightData.empty() ? weightData[i] : fastgltf::math::fvec4(0));
				vertices.push_back(vert);
			}

			// Read indices, adjust offset
			const auto& idAcc = asset.accessors[prim.indicesAccessor.value()];
			std::vector<std::uint32_t> primId(idAcc.count);
			fastgltf::copyFromAccessor<std::uint32_t>(asset, idAcc, primId.data());
			for (auto id : primId)
			{
				indices.push_back(id + vertexOffset);
			}

			// Setup indirect draw command
			IndirectDrawCommand cmd;
			cmd.count = static_cast<std::uint32_t>(idAcc.count);
			cmd.instanceCount = 1;
			cmd.firstIndex = indexOffset;
			cmd.baseVertex = vertexOffset;
			cmd.baseInstance = 0;
			cmds.push_back(cmd);

			vertexOffset += static_cast<std::uint32_t>(vCount);
			indexOffset += static_cast<std::uint32_t>(idAcc.count);
		}

		if (vertices.empty() || indices.empty() || cmds.empty()) continue;

		// Upload vertex buffer
		glCreateBuffers(1, &gpu.vbo);
		glNamedBufferData(gpu.vbo, static_cast<GLsizeiptr>(vertices.size() * sizeof(VertexLocal)), vertices.data(), GL_STATIC_DRAW);

		// Upload index buffer
		glCreateBuffers(1, &gpu.ibo);
		glNamedBufferData(gpu.ibo, static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)), indices.data(), GL_STATIC_DRAW);

		constexpr GLsizei stride = sizeof(VertexLocal);

		// Position
		glEnableVertexArrayAttrib(gpu.vao, 0);
		glVertexArrayAttribFormat(gpu.vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(VertexLocal, position));
		glVertexArrayAttribBinding(gpu.vao, 0, 0);
		// TexCoord
		glEnableVertexArrayAttrib(gpu.vao, 1);
		glVertexArrayAttribFormat(gpu.vao, 1, 2, GL_FLOAT, GL_FALSE, offsetof(VertexLocal, uv));
		glVertexArrayAttribBinding(gpu.vao, 1, 0);
		// Normal
		glEnableVertexArrayAttrib(gpu.vao, 2);
		glVertexArrayAttribFormat(gpu.vao, 2, 3, GL_FLOAT, GL_FALSE, offsetof(VertexLocal, normal));
		glVertexArrayAttribBinding(gpu.vao, 2, 0);
		// Tangent
		glEnableVertexArrayAttrib(gpu.vao, 3);
		glVertexArrayAttribFormat(gpu.vao, 3, 4, GL_FLOAT, GL_FALSE, offsetof(VertexLocal, tangent));
		glVertexArrayAttribBinding(gpu.vao, 3, 0);
		// Joints
		glEnableVertexArrayAttrib(gpu.vao, 4);
		glVertexArrayAttribIFormat(gpu.vao, 4, 4, GL_UNSIGNED_INT, offsetof(VertexLocal, joints));
		glVertexArrayAttribBinding(gpu.vao, 4, 0);
		// Weights
		glEnableVertexArrayAttrib(gpu.vao, 5);
		glVertexArrayAttribFormat(gpu.vao, 5, 4, GL_FLOAT, GL_FALSE, offsetof(VertexLocal, weights));
		glVertexArrayAttribBinding(gpu.vao, 5, 0);

		// BInd buffers to VAO
		glVertexArrayVertexBuffer(gpu.vao, 0, gpu.vbo, 0, stride);
		glVertexArrayElementBuffer(gpu.vao, gpu.ibo);

		// Upload indirect draw commands
		glCreateBuffers(1, &gpu.indirectBuffer);
		glNamedBufferData(gpu.indirectBuffer, static_cast<GLsizeiptr>(cmds.size() * sizeof(IndirectDrawCommand)), cmds.data(), GL_STATIC_DRAW);
		gpu.drawCount = cmds.size();

		gpuMeshes.push_back(gpu);
	}

	return gpuMeshes;
}