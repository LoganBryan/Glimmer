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
	defaultMaterial.metallicFactor = 0.0f;
	defaultMaterial.roughnessFactor = 0.0f;
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
	for (auto& skin : viewer.asset.skins)
	{
		LoadSkin(skin);
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

	return true;
}

void GltfLoader::DrawModel(Shader& shader, glm::mat4& objectTransform)
{
	if (!viewer.asset.scenes.empty() && viewer.sceneIndex < viewer.asset.scenes.size())
	{
		// TODO: Probably should use fastgltfs matrix transform (for camera and objects etc)
		fastgltf::iterateSceneNodes(viewer.asset, viewer.sceneIndex, fastgltf::math::fmat4x4(), [&](fastgltf::Node& node, fastgltf::math::fmat4x4 matrix)
			{
				glm::mat4 transform = glm::make_mat4(matrix.data());

				if (node.meshIndex.has_value())
				{
					bool hasSkin = node.skinIndex.has_value();
					shader.Use();
					shader.SetBool("uHasSkinning", hasSkin);
					if (hasSkin && node.skinIndex.value() < viewer.skins.size())
					{
						Skin& skin = viewer.skins[node.skinIndex.value()];
						glBindBufferBase(GL_UNIFORM_BUFFER, 1, skin.jointMatrixBuffer);
					}
					glm::mat4 finalModel = objectTransform * transform;
					shader.SetMatrix4("model", finalModel);
					DrawMesh(*node.meshIndex);
				}
			});
	}
}

void GltfLoader::UpdateSkins(glm::mat4& objectTransform) {
	for (auto& skin : viewer.skins) 
	{
		for (auto& joint : skin.joints) 
		{
			joint.globalTransform = joint.localTransform;
		}

		// Update hierarchy starting from roots
		for (auto& joint : skin.joints) 
		{
			if (joint.parentIndex == -1) 
			{
				joint.globalTransform = objectTransform * joint.localTransform;
				UpdateJointHeirarchy(skin, joint);
			}
		}

		glNamedBufferSubData(
			skin.jointMatrixBuffer,
			0,
			sizeof(glm::mat4) * skin.jointMatrices.size(),
			skin.jointMatrices.data()
		);
	}
}

void GltfLoader::UpdateJointHeirarchy(Skin& skin, Joint& joint) {
	if (joint.parentIndex != -1) {
		if (joint.parentIndex >= skin.joints.size()) 
		{
			std::cerr << "Invalid parent index " << joint.parentIndex << " for joint " << joint.index << std::endl;
			return;
		}
		joint.globalTransform = skin.joints[joint.parentIndex].globalTransform * joint.localTransform;
	}
	else 
	{
		joint.globalTransform = joint.localTransform;
	}

	if (joint.index >= skin.jointMatrices.size()) 
	{
		std::cerr << "Joint index out of range: " << joint.index << " (max " << skin.jointMatrices.size() << ")" << std::endl;
		return;
	}
	skin.jointMatrices[joint.index] = joint.globalTransform * joint.inverseBindMatrix;

	for (int childIndex : joint.children) 
	{
		if (childIndex < 0 || childIndex >= skin.joints.size()) 
		{
			std::cerr << "Invalid child index " << childIndex << " for joint " << joint.index << std::endl;
			continue;
		}
		UpdateJointHeirarchy(skin, skin.joints[childIndex]);
	}
}

bool GltfLoader::UpdateJointTransform(const std::string& jointName, const glm::mat4& transform)
{
	for (auto& skin : viewer.skins)
	{
		for (auto& joint : skin.joints)
		{
			if (joint.name == jointName)
			{
				joint.localTransform = joint.bindPose * transform;
				return true;
			}
		}
	}
	return false;
}

glm::mat4& GltfLoader::GetBindPoseTranslation(const std::string& jointName)
{
	glm::mat4 outPose(1.0f);
	for (auto& skin : viewer.skins)
	{
		for (auto& joint : skin.joints)
		{
			if (joint.name == jointName)
			{
				return joint.bindPose;
			}
		}
	}
	return outPose;
}

bool GltfLoader::LoadFromPath(std::filesystem::path filePath)
{
	if (!std::filesystem::exists(filePath))
	{
		wprintf(L"Failed to find %s!\n", filePath.wstring().c_str());
		return false;
	}

	wprintf(L"Loading %s...\n", filePath.wstring().c_str());

	// Determine file type on extension, gltf or glb - it might be better to read the header for magic bytes, but this is fine for now
	const auto ext = filePath.extension().string();
	bool isGLB = (ext == ".glb" || ext == ".GLB");

	fastgltf::Parser parser(supportedExtensions);

	decltype(viewer.asset) loadedAsset;
	auto loadedFromPath = fastgltf::MappedGltfFile::FromPath(filePath);

	if (isGLB)
	{
		if (!bool(loadedFromPath))
		{
			std::string_view err = fastgltf::getErrorMessage(loadedFromPath.error());
			printf("Error reading GLB file: %.*s!\n", static_cast<int>(err.size()), err.data());
			return false;
		}

		auto asset = parser.loadGltfBinary(loadedFromPath.get(), filePath.parent_path(), gltfOptions);
		if (asset.error() != fastgltf::Error::None)
		{
			std::string_view err = fastgltf::getErrorMessage(asset.error());
			printf("Error parsing GLB asset: %.*s!\n", static_cast<int>(err.size()), err.data());
			return false;
		}

		loadedAsset = std::move(asset.get());
	}
	else
	{
		if (!bool(loadedFromPath))
		{
			std::string_view err = fastgltf::getErrorMessage(loadedFromPath.error());
			printf("Error reading glTF file: %.*s\n", static_cast<int>(err.size()), err.data());
			return false;
		}

		auto asset = parser.loadGltf(loadedFromPath.get(), filePath.parent_path(), gltfOptions);
		if (asset.error() != fastgltf::Error::None)
		{
			std::string_view err = fastgltf::getErrorMessage(asset.error());
			printf("%.*s\n", static_cast<int>(err.size()), err.data());

			return false;
		}

		loadedAsset = std::move(asset.get());

	}

	viewer.asset = std::move(loadedAsset);
	return true;
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

		if (const auto* jointsIt = it->findAttribute("JOINTS_0"); jointsIt != it->attributes.end())
		{
			auto& jointsAccessor = asset.accessors[jointsIt->accessorIndex];
			if (!jointsAccessor.bufferViewIndex.has_value()) continue;

			auto componentType = jointsAccessor.componentType;
			auto* vertices = static_cast<Vertex*>(glMapNamedBuffer(primitive.vertexBuffer, GL_WRITE_ONLY));

			fastgltf::iterateAccessorWithIndex<fastgltf::math::uvec4>(asset, jointsAccessor, [&](fastgltf::math::uvec4 joints, size_t id)
				{
					vertices[id].joints = joints;
				});
			glUnmapNamedBuffer(primitive.vertexBuffer);

			glEnableVertexArrayAttrib(VAO, 4);
			glVertexArrayAttribIFormat(VAO, 4, 4, GL_UNSIGNED_INT, offsetof(Vertex, joints));
			glVertexArrayAttribBinding(VAO, 4, 0);
			glVertexArrayVertexBuffer(VAO, 4, primitive.vertexBuffer, offsetof(Vertex, joints), sizeof(Vertex));
		}

		if (const auto* weightsIt = it->findAttribute("WEIGHTS_0"); weightsIt != it->attributes.end())
		{
			auto& weightsAccessor = asset.accessors[weightsIt->accessorIndex];
			if (!weightsAccessor.bufferViewIndex.has_value()) continue;

			auto* vertices = static_cast<Vertex*>(glMapNamedBuffer(primitive.vertexBuffer, GL_WRITE_ONLY));
			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, weightsAccessor, [&](fastgltf::math::fvec4 weights, size_t id)
				{
					vertices[id].weights = weights;
				});
			glUnmapNamedBuffer(primitive.vertexBuffer);

			glEnableVertexArrayAttrib(VAO, 5);
			glVertexArrayAttribFormat(VAO, 5, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, weights));
			glVertexArrayAttribBinding(VAO, 5, 0);
			glVertexArrayVertexBuffer(VAO, 5, primitive.vertexBuffer, offsetof(Vertex, weights), sizeof(Vertex));
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
	//uniforms.metallicFactor = material.pbrData.metallicFactor;
	//uniforms.roughnessFactor = material.pbrData.roughnessFactor;
	if (material.pbrData.baseColorTexture.has_value())
	{
		uniforms.flags |= MaterialUniformFlags::HasBaseColorTexture;
	}

	if (material.pbrData.metallicRoughnessTexture.has_value())
	{
		uniforms.flags |= MaterialUniformFlags::HasMetallicRoughnessTexture;
	}

	if (material.normalTexture.has_value())
	{
		uniforms.flags |= MaterialUniformFlags::HasNormalTexture;
	}

	if (material.occlusionTexture.has_value())
	{
		uniforms.flags |= MaterialUniformFlags::HasOcclusionTexture;
	}

	if (material.emissiveTexture.has_value())
	{
		uniforms.flags |= MaterialUniformFlags::HasEmissiveTexture;
	}

	viewer.materials.emplace_back(uniforms);
	return true;
}

bool GltfLoader::LoadSkin(fastgltf::Skin& skin)
{
	Skin newSkin;

	std::cout << "Loading skin with " << skin.joints.size() << " joints\n";

	if (!skin.inverseBindMatrices)
	{
		printf("Skin missing inverse bind matrices accessor!\n");
		return false;
	}

	const auto ibmAccessorIndex = skin.inverseBindMatrices.value();
	if (ibmAccessorIndex >= viewer.asset.accessors.size())
	{
		printf("Invalid inverse bind matrices accessor index: %zu!\n", ibmAccessorIndex);
		return false;
	}

	auto& ibmAccessor = viewer.asset.accessors[ibmAccessorIndex];
	if (ibmAccessor.type != fastgltf::AccessorType::Mat4 || ibmAccessor.componentType != fastgltf::ComponentType::Float)
	{
		printf("Invalid inverse bind matrices accessor type!\n");
		return false;
	}

	// Load inverse bind matrices
	try
	{
		newSkin.jointMatrices.reserve(ibmAccessor.count);
		fastgltf::iterateAccessor<fastgltf::math::fmat4x4>(viewer.asset, ibmAccessor, [&](const fastgltf::math::fmat4x4& matrix)
			{
				newSkin.jointMatrices.emplace_back(glm::make_mat4(matrix.data()));
			});
	}
	catch (const std::exception& e)
	{
		std::cerr << "Failed to load inverse bind matrices: " << e.what() << std::endl;
		return false;
	}

	if (newSkin.jointMatrices.size() != skin.joints.size())
	{
		std::cerr << "Mismatch between joint count (" << skin.joints.size() << ") and inverse bind matrix count (" << newSkin.jointMatrices.size() << ")" << std::endl;
		return false;
	}

	// Create joints
	for (auto& jointIndex : skin.joints)
	{
		if (jointIndex >= viewer.asset.nodes.size())
		{
			printf("Invalid joint node index: %zu!\n", jointIndex);
			return false;
		}

		auto& node = viewer.asset.nodes[jointIndex];
		Joint joint;
		joint.nodeIndex = jointIndex;
		joint.name = node.name.empty() ? "unnamed_joint" + std::to_string(jointIndex) : node.name.c_str();

		// Get transform from node
		try 
		{

			std::visit(fastgltf::visitor{
				[&](const fastgltf::TRS& trs)
				{
					if (trs.scale[0] == 0.0f || trs.scale[1] == 0.0f || trs.scale[2] == 0.0f)
						throw std::runtime_error("Invalid zero scale in TRS!");

					// Construct matrix from TRS
					glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(trs.translation[0], trs.translation[1], trs.translation[2]));

					glm::quat rotation(trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]);
					if (abs(1.0f - glm::length(rotation)) > 0.001f)
						throw std::runtime_error("Non-normalized rotation quaternion!");

					glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(trs.scale[0], trs.scale[1], trs.scale[2]));

					glm::mat4 bindMatrix = translation * glm::mat4_cast(rotation) * scale;
					joint.localTransform = bindMatrix;
					joint.bindPose = bindMatrix;

				},
				[&](const fastgltf::math::fmat4x4& matrix)
				{
					// Use matrix from variant directly
					const glm::mat4 m = glm::make_mat4(matrix.data());
					if (glm::determinant(m) == 0.0f)
						throw std::runtime_error("Degenerate transformation matrix!");

					joint.localTransform = m;
					joint.bindPose = m;
				}
			}, node.transform);

			newSkin.joints.push_back(joint);
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error processing joint " << jointIndex << " (" << joint.name << "): " << e.what() << std::endl;
			return false;
		}
	}

	std::unordered_map<size_t, size_t> nodeParentMap;

	// Create parent map for all nodes
	for (size_t nodeId = 0; nodeId < viewer.asset.nodes.size(); nodeId++)
	{
		const auto& node = viewer.asset.nodes[nodeId];
		for (const auto& childId : node.children)
			nodeParentMap[childId] = nodeId;
	}

	for (size_t i = 0; i < skin.joints.size(); i++)
	{
		newSkin.joints[i].inverseBindMatrix = newSkin.jointMatrices[i];
	}

	for (size_t i = 0; i < newSkin.joints.size(); i++)
	{
		auto& joint = newSkin.joints[i];
		const auto& node = viewer.asset.nodes[joint.nodeIndex];

		newSkin.joints[i].index = static_cast<int>(i);

		// Find parent in skin joints
		if (auto parentIt = nodeParentMap.find(joint.nodeIndex); parentIt != nodeParentMap.end())
		{
			// Is parent node part of skin joint
			auto skinParentIt = std::find(skin.joints.begin(), skin.joints.end(), parentIt->second);

			if (skinParentIt != skin.joints.end())
			{
				joint.parentIndex = std::distance(skin.joints.begin(), skinParentIt);
			}
		}

		// Find children in skin joints
		for (auto childNodeIndex : node.children)
		{
			auto skinChildIt = std::find(skin.joints.begin(), skin.joints.end(), childNodeIndex);

			if (skinChildIt != skin.joints.end())
			{
				int childSkinIndex = std::distance(skin.joints.begin(), skinChildIt);
				joint.children.push_back(childSkinIndex);
			}
		}

	}

	// Create Buffer for joint matrices
	glCreateBuffers(1, &newSkin.jointMatrixBuffer);
	glNamedBufferStorage(newSkin.jointMatrixBuffer, sizeof(glm::mat4) * MAX_JOINTS, nullptr, GL_DYNAMIC_STORAGE_BIT);

	viewer.skins.push_back(newSkin);

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
