#pragma once

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include <glm/glm.hpp>

#include <OpenGL/Model/GltfLoaderData.h>

class SkinManager
{
public:
	SkinManager() = default;

	void Load(const fastgltf::Asset& asset);
	void Upload(const Transform& objectTransform);

	bool UpdateJointTransform(const std::string& jointName, const glm::mat4& transform);

	std::optional<glm::mat4> GetBindPose(const std::string& jointName) const;

	inline const std::vector<Skin>& GetSkins() const { return mSkins; }

private:
	void Propagate(Skin& skin, Joint& joint, const glm::mat4& parentMat = glm::mat4(1.0f));

	SkinManager(const SkinManager&) = delete;
	SkinManager& operator=(const SkinManager&) = delete;
private:
	std::vector<Skin> mSkins;
};

