#pragma once
#include <OpenGL/Application/Shader.h>

#include "ResourceCache.h"
#include "InstanceManager.h"
#include "SkinManager.h"
#include "MaterialManager.h"

class SceneRenderer
{
public:
	void Draw(Shader& shader, const std::vector<MeshGPU>& meshes, const MaterialManager& mats, const SkinManager& skins, const InstanceManager& instances);
};

