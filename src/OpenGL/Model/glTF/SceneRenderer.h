#pragma once
#include <OpenGL/Application/Shader.h>

#include "ResourceCache.h"
#include "InstanceManager.h"
#include "SkinManager.h"
#include "MaterialManager.h"

class SceneRenderer
{
public:
    SceneRenderer() {};
    ~SceneRenderer()
    {
        if (mDynamicIndirectBuffer != 0)
            glDeleteBuffers(1, &mDynamicIndirectBuffer);
    }

    void Draw(Shader& shader,const InstanceManager& instances, const ResourceCache& cache, const MaterialManager& mats, const SkinManager& skins);

private:
    GLuint mDynamicIndirectBuffer = 0;
    GLsizeiptr mDynamicIndirectBufferSize = 0;
    std::vector<IndirectDrawCommand> mCommandBuffer;
};

