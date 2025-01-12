#include "GltfLoader.h"

GltfLoader::GltfLoader() {}

GltfLoader::~GltfLoader() {}

bool GltfLoader::LoadModel(std::filesystem::path filePath, Shader& shader)
{
	auto start = std::chrono::high_resolution_clock::now();
	if (!LoadFromPath(filePath))
	{
		printf("Failed to parse gLTF!\n");
		return false;
	}

	// Create a default material
	auto& defaultMaterial = viewer.materials.emplace_back();
	defaultMaterial.baseColorFactor = fastgltf::math::fvec4(1.0f);
	defaultMaterial.alphaCutoff = 0.0f;
	defaultMaterial.flags = 0;

	// Load Images first
	for (auto& image : viewer.asset.images)
	{
		LoadImage(image);
	}
	for (auto& material : viewer.asset.materials)
	{
		LoadMaterial(material);
	}
	for (auto& mesh : viewer.asset.meshes)
	{
		LoadMeshData(mesh);
	}
	auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
	printf("fastGLTF loaded in %lld ms\n", diff.count());

	// Create material uniform buffer
	viewer.materialBuffers.resize(viewer.materials.size(), GL_NONE);
	glCreateBuffers(static_cast<GLsizei>(viewer.materials.size()), viewer.materialBuffers.data());
	for (auto i = 0UL; i < viewer.materialBuffers.size(); i++)
	{
		glNamedBufferStorage(viewer.materialBuffers[i], static_cast<GLsizeiptr>(sizeof(MaterialUniforms)), &viewer.materials[i], GL_MAP_WRITE_BIT);
	}

	// TODO: Could allow inputting shader data, incase the used shader doesn't have them
	viewer.uvOffsetUniform = glGetUniformLocation(shader.GetID(), "uvOffset");
	viewer.uvScaleUniform = glGetUniformLocation(shader.GetID(), "uvScale");
	viewer.uvRotationUniform = glGetUniformLocation(shader.GetID(), "uvRotation");

	shader.Use();
	shader.SetInt("albedoTexture", 0);
	shader.SetInt("metallicRoughnessTexture", 1);
	shader.SetInt("normalTexture", 2);
	shader.SetInt("emissiveTexture", 3);
	shader.SetInt("occlusionTexture", 4);
	shader.SetInt("skybox", 5);

	viewer.sceneIndex = viewer.asset.defaultScene.value_or(0);
}

void GltfLoader::DrawModel()
{
	if (!viewer.asset.scenes.empty() && viewer.sceneIndex < viewer.asset.scenes.size())
	{
		// TODO: Probably should use fastgltfs matrix transform (for camera and objects etc)
		fastgltf::iterateSceneNodes(viewer.asset, viewer.sceneIndex, fastgltf::math::fmat4x4(), [&](fastgltf::Node& node, fastgltf::math::fmat4x4 matrix) 
			{
				if (node.meshIndex.has_value())
					DrawMesh(*node.meshIndex);
			});
	}
}

bool GltfLoader::LoadFromPath(std::filesystem::path filePath)
{
	if (!std::filesystem::exists(filePath))
	{
		wprintf(L"Failed to find %s!\n", filePath.wstring().c_str());
		return false;
	}

	wprintf(L"Loading %s...\n", filePath.wstring().c_str());

	// Parse gLTF file
	{
		fastgltf::Parser parser(supportedExtensions);

		auto gltfFile = fastgltf::MappedGltfFile::FromPath(filePath);
		if (!bool(gltfFile))
		{
			std::string_view err = fastgltf::getErrorMessage(gltfFile.error());
			printf("%.*s\n", static_cast<int>(err.size()), err.data());

			return false;
		}

		auto asset = parser.loadGltf(gltfFile.get(), filePath.parent_path(), gltfOptions);
		if (asset.error() != fastgltf::Error::None)
		{
			std::string_view err = fastgltf::getErrorMessage(asset.error());
			printf("%.*s\n", static_cast<int>(err.size()), err.data());

			return false;
		}

		viewer.asset = std::move(asset.get());
	}
}

bool GltfLoader::LoadMeshData(fastgltf::Mesh& mesh)
{
	auto& asset = viewer.asset;
	Mesh outMesh = {};
	outMesh.primitives.resize(mesh.primitives.size());

	for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); it++)
	{
		auto* posIt = it->findAttribute("POSITION");
		assert(posIt != it->attributes.end()); // Mesh primitive is needed to hold pos attribute
		assert(it->indicesAccessor.has_value()); // GenerateMeshIndicies was specified, so we SHOULD have it

		// Generate VAO
		GLuint VAO = GL_NONE;
		glCreateVertexArrays(1, &VAO);

		std::size_t baseColorTexIndex = 0;

		// Get output primitive
		auto index = std::distance(mesh.primitives.begin(), it);
		auto& primitive = outMesh.primitives[index];
		primitive.primitiveType = fastgltf::to_underlying(it->type);
		primitive.vertexArray = VAO;

		if (it->materialIndex.has_value())
		{
			primitive.materialUniformsIndex = it->materialIndex.value() + 1;
			auto& material = viewer.asset.materials[it->materialIndex.value()];

			// TODO: move into a helper function instead of having all of them typed out
			auto& baseColorTex = material.pbrData.baseColorTexture;
			if (baseColorTex.has_value())
			{
				auto& texture = viewer.asset.textures[baseColorTex->textureIndex];

				if (!texture.imageIndex.has_value())
					printf("Couldn't find an albedo!\n");

				primitive.albedoTexture = viewer.textures[texture.imageIndex.value()].texture;

				if (baseColorTex->transform && baseColorTex->transform->texCoordIndex.has_value()) baseColorTexIndex = baseColorTex->transform->texCoordIndex.value();
				else baseColorTexIndex = material.pbrData.baseColorTexture->texCoordIndex;
			}

			auto& metallicRougnessTexture = material.pbrData.metallicRoughnessTexture;
			if (metallicRougnessTexture.has_value())
			{
				auto& texture = viewer.asset.textures[metallicRougnessTexture->textureIndex];

				if (!texture.imageIndex.has_value())
					printf("Couldn't find a metallic roughness!\n");

				primitive.metallicRoughnessTexture = viewer.textures[texture.imageIndex.value()].texture;
			}

			auto& normalColorTex = material.normalTexture;
			if (normalColorTex.has_value())
			{
				auto& texture = viewer.asset.textures[normalColorTex->textureIndex];

				if (!texture.imageIndex.has_value())
					printf("Couldn't find a normal!\n");

				primitive.normalTexture = viewer.textures[texture.imageIndex.value()].texture;
			}

			auto& emissiveTexture = material.emissiveTexture;
			if (emissiveTexture.has_value())
			{
				auto& texture = viewer.asset.textures[emissiveTexture->textureIndex];

				if (!texture.imageIndex.has_value())
					printf("Couldn't find an emissive!\n");

				primitive.emissiveTexture = viewer.textures[texture.imageIndex.value()].texture;
			}

			auto& occlusionTexture = material.occlusionTexture;
			if (occlusionTexture.has_value())
			{
				auto& texture = viewer.asset.textures[occlusionTexture->textureIndex];

				if (!texture.imageIndex.has_value())
					printf("Couldn't find an occlusion!\n");

				primitive.occlusionTexture = viewer.textures[texture.imageIndex.value()].texture;
			}

		}
		else primitive.materialUniformsIndex = 0;

		{
			// Position
			auto& positionAccesor = asset.accessors[posIt->accessorIndex];
			if (!positionAccesor.bufferViewIndex.has_value()) continue;

			// Create the vertex buffer for primitive, then copy into mapped buffer
			glCreateBuffers(1, &primitive.vertexBuffer);
			glNamedBufferData(primitive.vertexBuffer, positionAccesor.count * sizeof(Vertex), nullptr, GL_STATIC_DRAW);

			auto* vertices = static_cast<Vertex*>(glMapNamedBuffer(primitive.vertexBuffer, GL_WRITE_ONLY));
			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, positionAccesor, [&](fastgltf::math::fvec3 pos, std::size_t id)
				{
					vertices[id].position = fastgltf::math::fvec3(pos.x(), pos.y(), pos.z());
					vertices[id].uv = fastgltf::math::fvec2();
				});
			glUnmapNamedBuffer(primitive.vertexBuffer);

			glEnableVertexArrayAttrib(VAO, 0);
			glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
			glVertexArrayAttribBinding(VAO, 0, 0);

			glVertexArrayVertexBuffer(VAO, 0, primitive.vertexBuffer, 0, sizeof(Vertex));
		}

		auto texCoordAttr = std::string("TEXCOORD_") + std::to_string(baseColorTexIndex);
		if (const auto* texCoord = it->findAttribute(texCoordAttr); texCoord != it->attributes.end())
		{
			// Tex-Coord
			auto& texCoordAccessor = asset.accessors[texCoord->accessorIndex];
			if (!texCoordAccessor.bufferViewIndex.has_value()) continue;

			auto* vertices = static_cast<Vertex*>(glMapNamedBuffer(primitive.vertexBuffer, GL_WRITE_ONLY));
			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, texCoordAccessor, [&](fastgltf::math::fvec2 uv, std::size_t id)
				{
					vertices[id].uv = fastgltf::math::fvec2(uv.x(), uv.y());
				});
			glUnmapNamedBuffer(primitive.vertexBuffer);

			glEnableVertexArrayAttrib(VAO, 1);
			glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, 0);
			glVertexArrayAttribBinding(VAO, 1, 1);

			glVertexArrayVertexBuffer(VAO, 1, primitive.vertexBuffer, offsetof(Vertex, uv), sizeof(Vertex));
		}

		if (const auto* normalIt = it->findAttribute("NORMAL"); normalIt != it->attributes.end())
		{
			auto& normalAccessor = asset.accessors[normalIt->accessorIndex];
			if (!normalAccessor.bufferViewIndex.has_value()) continue;

			auto* vertices = static_cast<Vertex*>(glMapNamedBuffer(primitive.vertexBuffer, GL_WRITE_ONLY));
			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, normalAccessor, [&](fastgltf::math::fvec3 normal, std::size_t id)
				{
					vertices[id].normal = fastgltf::math::fvec3(normal.x(), normal.y(), normal.z());
				});
			glUnmapNamedBuffer(primitive.vertexBuffer);

			glEnableVertexArrayAttrib(VAO, 2);
			glVertexArrayAttribFormat(VAO, 2, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
			glVertexArrayAttribBinding(VAO, 2, 0);

			glVertexArrayVertexBuffer(VAO, 2, primitive.vertexBuffer, offsetof(Vertex, normal), sizeof(Vertex));
		}

		if (const auto* tangentIt = it->findAttribute("TANGENT"); tangentIt != it->attributes.end())
		{
			auto& tangentAccessor = asset.accessors[tangentIt->accessorIndex];
			if (!tangentAccessor.bufferViewIndex.has_value()) continue;

			auto* vertices = static_cast<Vertex*>(glMapNamedBuffer(primitive.vertexBuffer, GL_WRITE_ONLY));
			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, tangentAccessor, [&](fastgltf::math::fvec4 tangent, std::size_t id)
				{
					vertices[id].tangent = fastgltf::math::fvec4(tangent.x(), tangent.y(), tangent.z(), tangent.w());
				});
			glUnmapNamedBuffer(primitive.vertexBuffer);

			glEnableVertexArrayAttrib(VAO, 3);
			glVertexArrayAttribFormat(VAO, 3, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));
			glVertexArrayAttribBinding(VAO, 3, 0);

			glVertexArrayVertexBuffer(VAO, 3, primitive.vertexBuffer, offsetof(Vertex, tangent), sizeof(Vertex));
		}

		// Gen indirect draw command
		auto& draw = primitive.draw;
		draw.instanceCount = 1;
		draw.baseInstance = 0;
		draw.baseVertex = 0;
		draw.firstIndex = 0;

		auto& indexAccessor = asset.accessors[it->indicesAccessor.value()];
		if (!indexAccessor.bufferViewIndex.has_value()) return false;
		draw.count = static_cast<std::uint32_t>(indexAccessor.count);

		// Create index buffer, then copy indicies into it
		glCreateBuffers(1, &primitive.indexBuffer);
		if (indexAccessor.componentType == fastgltf::ComponentType::UnsignedByte || indexAccessor.componentType == fastgltf::ComponentType::UnsignedShort)
		{
			primitive.indexType = GL_UNSIGNED_SHORT;
			glNamedBufferData(primitive.indexBuffer, static_cast<GLsizeiptr>(indexAccessor.count * sizeof(std::uint16_t)), nullptr, GL_STATIC_DRAW);
			auto* indices = static_cast<std::uint16_t*>(glMapNamedBuffer(primitive.indexBuffer, GL_WRITE_ONLY));
			fastgltf::copyFromAccessor<std::uint16_t>(asset, indexAccessor, indices);
			glUnmapNamedBuffer(primitive.indexBuffer);
		}

		glVertexArrayElementBuffer(VAO, primitive.indexBuffer);
	}

	// Create buffer with all primitive structs
	glCreateBuffers(1, &outMesh.drawBuffer);
	glNamedBufferData(outMesh.drawBuffer, static_cast<GLsizeiptr>(outMesh.primitives.size() * sizeof(Primitive)), outMesh.primitives.data(), GL_STATIC_DRAW);

	viewer.meshes.emplace_back(outMesh);

	return true;
}

bool GltfLoader::LoadMaterial(fastgltf::Material& material)
{
	MaterialUniforms uniforms = {};
	uniforms.alphaCutoff = material.alphaCutoff;

	uniforms.baseColorFactor = material.pbrData.baseColorFactor;
	if (material.pbrData.baseColorTexture.has_value())
	{
		uniforms.flags |= MaterialUniformFlags::HasBaseColorTexture;
	}

	viewer.materials.emplace_back(uniforms);
	return true;
}

bool GltfLoader::LoadImage(fastgltf::Image& image)
{
	auto getLevelCount = [](int width, int height) -> GLsizei
		{
			return static_cast<GLsizei>(1 + floor(log2(width > height ? width : height)));
		};

	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);

	std::visit(fastgltf::visitor{
		[](auto& arg) {},
		[&](fastgltf::sources::URI& filePath)
		{
			assert(filePath.fileByteOffset == 0);
			assert(filePath.uri.isLocalPath());
			int width, height, nChannels;

			const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
			unsigned char* data = stbi_load(path.c_str(), &width, &height, &nChannels, 4);

			glTextureStorage2D(texture, getLevelCount(width, height), GL_RGBA8, width, height);
			glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		},
		[&](fastgltf::sources::Array& vector)
		{
			int width, height, nChannels;
			unsigned char* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()), static_cast<int>(vector.bytes.size()), &width, &height, &nChannels, 4);

			glTextureStorage2D(texture, getLevelCount(width, height), GL_RGBA8, width, height);
			glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		},
		[&](fastgltf::sources::BufferView& view)
		{
			auto& bufferView = viewer.asset.bufferViews[view.bufferViewIndex];
			auto& buffer = viewer.asset.buffers[bufferView.bufferIndex];

			// Load buffer data for texture
			std::visit(fastgltf::visitor{
				[](auto& arg) {},
				[&](fastgltf::sources::Array& vector)
				{
					int width, height, nChannels;
					unsigned char* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data() + bufferView.byteOffset), static_cast<int>(bufferView.byteLength), &width, &height, &nChannels, 4);

					glTextureStorage2D(texture, getLevelCount(width, height), GL_RGBA8, width, height);
					glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
					stbi_image_free(data);
				}
			}, buffer.data);
		}
	}, image.data);

	glGenerateTextureMipmap(texture);

	viewer.textures.emplace_back(Texture{ texture });
	return true;
}

void GltfLoader::DrawMesh(std::size_t meshIndex)
{
	auto& mesh = viewer.meshes[meshIndex];

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, mesh.drawBuffer);

	for (auto i = 0U; i < mesh.primitives.size(); i++)
	{
		auto& primitive = mesh.primitives[i];
		auto& gltfPrimitive = viewer.asset.meshes[meshIndex].primitives[i];

		std::size_t materialIndex;
		auto& mappings = gltfPrimitive.mappings;

		materialIndex = (!mappings.empty() && mappings[viewer.materialVariant].has_value()) ? mappings[viewer.materialVariant].value() + 1 : primitive.materialUniformsIndex;

		auto& material = viewer.materialBuffers[materialIndex];

		// Bind albedo texture
		glBindTextureUnit(0, primitive.albedoTexture);

		// Bind metallic roughness texture
		glBindTextureUnit(1, primitive.metallicRoughnessTexture);

		// Bind normal texture
		glBindTextureUnit(2, primitive.normalTexture);

		// Bind emissive texture
		glBindTextureUnit(3, primitive.emissiveTexture);

		// Bind occlusion texture
		glBindTextureUnit(4, primitive.occlusionTexture);

		// Bind material uniform buffer
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, material);
		glBindVertexArray(primitive.vertexArray);

		// Update texture transform uniforms
		glUniform2f(viewer.uvOffsetUniform, 0, 0);
		glUniform2f(viewer.uvScaleUniform, 1.0f, 1.0f);
		glUniform1f(viewer.uvRotationUniform, 0);

		if (materialIndex != 0)
		{
			auto& gltfMaterial = viewer.asset.materials[materialIndex - 1];
			if (gltfMaterial.pbrData.baseColorTexture.has_value() && gltfMaterial.pbrData.baseColorTexture->transform)
			{
				auto& transform = gltfMaterial.pbrData.baseColorTexture->transform;
				glUniform2f(viewer.uvOffsetUniform, transform->uvOffset[0], transform->uvOffset[1]);
				glUniform2f(viewer.uvScaleUniform, transform->uvOffset[0], transform->uvScale[1]);
				glUniform1f(viewer.uvRotationUniform, static_cast<float>(transform->rotation));
			}
		}

		glDrawElementsIndirect(primitive.primitiveType, primitive.indexType, reinterpret_cast<const void*>(i * sizeof(Primitive)));
	}
}
