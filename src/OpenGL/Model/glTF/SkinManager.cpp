#include "SkinManager.h"
#include <iostream>

SkinManager::~SkinManager()
{
	for (const auto& skin : mSkins)
	{
		if (skin.jointMatrixBuffer != GL_NONE)
			glDeleteBuffers(1, &skin.jointMatrixBuffer);
	}
	mSkins.clear();
}

void SkinManager::Load(const fastgltf::Asset& asset)
{
	mSkins.reserve(asset.skins.size());
	std::unordered_map<size_t, size_t> nodeParentMap;

	for (size_t id = 0; id < asset.nodes.size(); id++)
	{
		const auto& node = asset.nodes[id];
		for (const auto& cID : node.children)
		{
			if (cID < asset.nodes.size())
				nodeParentMap[cID] = id;
			else
				std::cerr << "[SkinManager] Warning! Node " << id << " has invalid child node " << cID << "\n";
		}
	}

	for (const auto& skin : asset.skins)
	{
		Skin loadSkin;

		if (!skin.inverseBindMatrices.has_value())
		{
			std::cerr << "[SkinManager] Error! Skin is missing inverseBindMatrices accessor! \n";
			continue;
		}

		const size_t ibmAccessId = skin.inverseBindMatrices.value();
		if (ibmAccessId >= asset.accessors.size())
		{
			std::cerr << "[SkinManager] Error! Invalid inverseBindMatrices accessor index: " << ibmAccessId << "\n";
			continue;
		}

		const auto& ibmAccessor = asset.accessors[ibmAccessId];
		if (ibmAccessor.type != fastgltf::AccessorType::Mat4 || ibmAccessor.componentType != fastgltf::ComponentType::Float)
		{
			std::cerr << "[SkinManager] Error! InverseBindMatrices Accessor has invalid type or component type! \n";
			continue;
		}

		if (ibmAccessor.count != skin.joints.size())
		{
			std::cerr << "[SkinManager] Error! Mismatch between joint count [" << skin.joints.size() << "] and inverse bind matrix count [" << ibmAccessor.count << "]\n";
			continue;
		}

		std::vector<glm::mat4> inverseBindMatrices;
		inverseBindMatrices.resize(ibmAccessor.count, glm::mat4(1.0f));

		try
		{
			std::vector<fastgltf::math::fmat4x4> rawMatrices(ibmAccessor.count);
			fastgltf::copyFromAccessor<fastgltf::math::fmat4x4>(asset, ibmAccessor, rawMatrices.data());
			inverseBindMatrices.resize(ibmAccessor.count);

			for (size_t i = 0; i < ibmAccessor.count; i++)
			{
				inverseBindMatrices[i] = glm::make_mat4(rawMatrices[i].data());
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "[SkinManager] Error! Loading inverseBindMatrices " << e.what() << "\n";
			continue;
		}

		// Create joints & get init transforms
		loadSkin.joints.reserve(skin.joints.size());
		bool jointFailedToLoad = false;

		for (size_t i = 0; i < skin.joints.size(); i++)
		{
			size_t jointNodeIndex = skin.joints[i];

			if (jointNodeIndex >= asset.nodes.size())
			{
				std::cerr << "[SkinManager] Error! Invalid joint node index " << jointNodeIndex << "\n";
				jointFailedToLoad = true;
				break;
			}

			const auto& node = asset.nodes[jointNodeIndex];
			Joint joint;
			joint.nodeIndex = jointNodeIndex;
			joint.name = node.name.empty() ? ("joint_" + std::to_string(jointNodeIndex)) : node.name.c_str();
			joint.index = static_cast<int>(i);
			joint.inverseBindMatrix = inverseBindMatrices[i];

			try
			{
				std::visit(fastgltf::visitor
					{
						[&](const fastgltf::TRS& trs)
						{
							if (trs.scale[0] == 0.0f || trs.scale[1] == 0.0f || trs.scale[2] == 0.0f)
							{
								std::cerr << "[SkinManager] Warning! Joint " << joint.name << " [" << jointNodeIndex << "] has a zero scale component in TRS. Defaulting to identity \n";
								joint.localTransform = glm::mat4(1.0f);
								joint.bindPose = glm::mat4(1.0f);
								return;
							}

							glm::vec3 trans = { trs.translation[0], trs.translation[1], trs.translation[2] };
							glm::quat rot = { trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2] };
							glm::vec3 scale = { trs.scale[0], trs.scale[1], trs.scale[2] };

							// Check if quat is normalized
							if (std::abs(1.0f - glm::length(rot)) > 0.001f)
							{
								std::cerr << "[SkinWarning] Warning! Joint " << joint.name << " [" << jointNodeIndex << "] has non-normalized rotation quaternion\n";
								rot = glm::normalize(rot);
							}

							glm::mat4 transMat = glm::translate(glm::mat4(1.0f), trans);
							glm::mat4 rotMat = glm::mat4_cast(rot);
							glm::mat4 scaleMat = glm::translate(glm::mat4(1.0f), scale);

							glm::mat4 bindMat = transMat * rotMat * scaleMat;
							joint.localTransform = bindMat;
							joint.bindPose = bindMat;
						},
						[&](const fastgltf::math::fmat4x4& matrix)
						{
							glm::mat4 bindMat = glm::make_mat4(matrix.data());

							// Check for degenerate
							if (glm::determinant(bindMat) == 0.0f)
							{
								std::cerr << "[SkinManager] Warning! Joint " << joint.name << " [" << jointNodeIndex << "] has degenerate transformation matrix. Defaulting to identity\n";
								joint.localTransform = glm::mat4(1.0f);
								joint.bindPose = glm::mat4(1.0f);
							}
							else
							{
								joint.localTransform = bindMat;
								joint.bindPose = bindMat;
							}
						}
					}, node.transform);
			}
			catch (const std::exception& e)
			{
				std::cerr << "[SkinManager] Error! Failed to process transform for joint " << joint.name << " [" << jointNodeIndex << "] " << e.what() << "\n";
				jointFailedToLoad = true;
				break;
			}

			loadSkin.joints.push_back(joint);
		}

		if (jointFailedToLoad)
		{
			// Clean up joints and continue to next skin
			loadSkin.joints.clear();
			continue;
		}

		// Build heirarchy 
		for (size_t i = 0; i < loadSkin.joints.size(); i++)
		{
			Joint& joint = loadSkin.joints[i];
			const auto& node = asset.nodes[joint.nodeIndex];

			// Parent index
			joint.parentIndex = -1;
			auto parentIt = nodeParentMap.find(joint.nodeIndex);
			if (parentIt != nodeParentMap.end())
			{
				size_t parentNodeId = parentIt->second;
				auto skinParentIt = std::find_if(loadSkin.joints.begin(), loadSkin.joints.end(), [&](const Joint& j)
					{
						return j.nodeIndex == parentNodeId;
					});

				if (skinParentIt != loadSkin.joints.end())
					joint.parentIndex = static_cast<int>(std::distance(loadSkin.joints.begin(), skinParentIt));
			}

			// Children indices
			joint.children.clear();
			for (auto childNodeIndex : node.children)
			{
				auto skinChildIt = std::find_if(loadSkin.joints.begin(), loadSkin.joints.end(), [&](const Joint& j)
					{
						return j.nodeIndex == childNodeIndex;
					});

				if (skinChildIt != loadSkin.joints.end())
					joint.children.push_back(static_cast<int>(std::distance(loadSkin.joints.begin(), skinChildIt)));
			}
		}

		// Alloc GPU buffer
		size_t jointCount = loadSkin.joints.size();
		if (jointCount > 0)
		{
			loadSkin.jointMatrices.resize(jointCount, glm::mat4(1.0f));

			glCreateBuffers(1, &loadSkin.jointMatrixBuffer);
			glNamedBufferStorage(loadSkin.jointMatrixBuffer, sizeof(glm::mat4)* jointCount, nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
		}
		else
		{
			std::cerr << "[SkinManager] Warning! Skin loaded with 0 joints! \n";
			loadSkin.jointMatrixBuffer = GL_NONE;
		}

		mSkins.push_back(std::move(loadSkin));
	}
}

void SkinManager::Upload(const glm::mat4& rootTransform)
{
	for (auto& skin : mSkins)
	{
		if (skin.joints.empty() || skin.jointMatrixBuffer == GL_NONE)
			continue; // No joints or buffer.

		// Update heirarchy. start from root joints
		for (auto& joint : skin.joints)
		{
			if (joint.parentIndex == -1)
			{
				Propagate(skin, joint, rootTransform);
			}
		}

		size_t buffSize = sizeof(glm::mat4) * skin.jointMatrices.size();
		glNamedBufferSubData(skin.jointMatrixBuffer, 0, buffSize, skin.jointMatrices.data());
	}
}

bool SkinManager::UpdateJointTransform(const std::string& jointName, const glm::mat4& transform)
{
	for (auto& skin : mSkins)
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

std::optional<glm::mat4> SkinManager::GetBindPose(const std::string& jointName) const
{
	for (const auto& skin : mSkins)
	{
		for (const auto& joint : skin.joints)
		{
			if (joint.name == jointName)
			{
				return joint.bindPose;
			}
		}
	}
	return std::nullopt;
}

void SkinManager::Propagate(Skin& skin, Joint& joint, const glm::mat4& parentMat)
{
	joint.globalTransform = parentMat * joint.localTransform;

	if (joint.index >= 0 && static_cast<size_t>(joint.index) < skin.jointMatrices.size())
		skin.jointMatrices[joint.index] = joint.globalTransform * joint.inverseBindMatrix;
	else
	{
		std::cerr << "[SkinManager] Error! Invalid joint index " << joint.index << " encountered during skin propagation!\n";
	}

	 // Update children
	for (int childId : joint.children)
	{
		if (childId >= 0 && static_cast<size_t>(childId) < skin.joints.size())
			Propagate(skin, skin.joints[childId], joint.globalTransform);
		else
			std::cerr << "[SkinManager] Error! Invalid child index " << childId << " for joint " << joint.name << " [" << joint.index << "]\n";
	}
}
