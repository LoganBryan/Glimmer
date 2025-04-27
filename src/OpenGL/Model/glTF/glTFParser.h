#pragma once
#include <filesystem>
#include <iostream>

#include <OpenGL/Model/GltfLoaderData.h>


class glTFParser
{
public:
	std::optional<fastgltf::Asset> Parse(const std::filesystem::path& p);
};