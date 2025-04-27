#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include <OpenGL/Model/GltfLoaderData.h>

class MaterialManager
{
public:
	void BuildMaterials(const fastgltf::Asset& asset);
	inline const std::vector<GLuint> GetUBOs() const { return mUBOs; };

private:
	std::vector<GLuint> mUBOs;
};
