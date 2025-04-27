#include "ResourceCache.h"

std::shared_ptr<std::vector<MeshGPU>> ResourceCache::GetOrLoadMeshes(const std::filesystem::path& path)
{
    auto key = path.string();
    auto it = meshMap.find(key);
    if (it != meshMap.end())
        return it->second;

    auto assetOpt = parser.Parse(path);
    if (!assetOpt.has_value())
        throw std::runtime_error("Failed to parse glTF: " + key);
    const auto& asset = assetOpt.value();

    auto gpuList = uploader.UploadMeshes(asset);
    auto meshesPtr = std::make_shared<std::vector<MeshGPU>>(std::move(gpuList));
    meshMap[key] = meshesPtr;
    return meshesPtr;
}

GLuint ResourceCache::GetOrLoadTexture(const std::filesystem::path& path)
{
    auto key = path.generic_string();
    auto it = textureMap.find(key);
    if (it != textureMap.end())
        return it->second;

    GLuint placeHolder = texManager.CreatePlaceholder();
    textureMap[key] = placeHolder;

    texManager.AsyncLoad(placeHolder, key);
    return placeHolder;
}

std::vector<GLuint> ResourceCache::GetOrLoadMaterials(const std::filesystem::path& path)
{
    auto key = path.generic_string();
    auto it = materialsUBOsMap.find(key);
    if (it != materialsUBOsMap.end())
        return it->second;
    
    auto asset = parser.Parse(key);
    if (!asset)
        return {};

    matManager.BuildMaterials(*asset);

    auto uboList = matManager.GetUBOs();
    materialsUBOsMap[key] = uboList;

    return uboList;
}

std::shared_ptr<Skin> ResourceCache::GetOrLoadSkin(const std::filesystem::path& path)
{
    auto key = path.generic_string();
    auto it = skinMap.find(key);
    if (it != skinMap.end())
        return it->second;

    auto asset = parser.Parse(key);
    if (!asset)
        return nullptr;

    skinManager.Load(*asset);
    auto skins = skinManager.GetSkins();

    if (skins.empty())
        return nullptr;

    auto ptr = std::make_shared<Skin>(skins.front());
    skinMap[key] = ptr;

    return ptr;
}

void ResourceCache::Clear()
{
    meshMap.clear();
    textureMap.clear();
    materialsUBOsMap.clear();
    skinMap.clear();
}
