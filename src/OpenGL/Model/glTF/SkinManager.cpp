#include "SkinManager.h"

void SkinManager::Load(const fastgltf::Asset& asset)
{
}

void SkinManager::Upload(const Transform& objectTransform)
{
}

bool SkinManager::UpdateJointTransform(const std::string& jointName, const glm::mat4& transform)
{
	return false;
}

std::optional<glm::mat4> SkinManager::GetBindPose(const std::string& jointName) const
{
	return std::optional<glm::mat4>();
}

void SkinManager::Propagate(Skin& skin, Joint& joint, const glm::mat4& parentMat)
{
}
