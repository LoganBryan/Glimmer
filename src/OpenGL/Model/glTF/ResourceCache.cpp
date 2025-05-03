#include "ResourceCache.h"

std::shared_ptr<std::vector<MeshGPU>> ResourceCache::GetOrLoadMeshes(const std::filesystem::path& path)
{
    std::string key = path.generic_string();
    auto it = mMeshMap.find(key);
    if (it != mMeshMap.end())
        return it->second;

    auto assetPtr = GetParsedAsset(path);
    if (!assetPtr)
        throw std::runtime_error("[ResourceCache] Failed to get parsed asset for mesh " + key);

    auto gpuList = uploader.UploadMeshes(*assetPtr);
    auto meshesPtr = std::make_shared<std::vector<MeshGPU>>(std::move(gpuList));
    mMeshMap[key] = meshesPtr;
    return meshesPtr;
}

size_t ResourceCache::LoadMaterialsFromAsset(const std::filesystem::path& path)
{
    std::string key = path.generic_string();

    auto assetPtr = GetParsedAsset(path);
    if (!assetPtr)
        throw std::runtime_error("[ResourceCache] Failed to get parsed asset for material " + key);

    return matManager.AddMaterials(*assetPtr, path, *this, texManager);
}

std::shared_ptr<Skin> ResourceCache::GetOrLoadSkin(const std::filesystem::path& path)
{
    auto key = path.generic_string();
    auto it = mSkinMap.find(key);
    if (it != mSkinMap.end())
        return it->second;

    auto asset = parser.Parse(key);
    if (!asset)
        return nullptr;

    skinManager.Load(*asset);
    auto skins = skinManager.GetSkins();

    if (skins.empty())
        return nullptr;

    auto ptr = std::make_shared<Skin>(skins.front());
    mSkinMap[key] = ptr;

    return ptr;
}

void ResourceCache::Clear()
{
    mParsedAssetCache.clear();
    mMeshMap.clear();
    mSkinMap.clear();
}

std::shared_ptr<fastgltf::Asset> ResourceCache::GetParsedAsset(const std::filesystem::path& path)
{
    std::string key = path.generic_string();
    auto it = mParsedAssetCache.find(key);
    if (it != mParsedAssetCache.end())
        return it->second;

    auto assetOpt = parser.Parse(path);
    if (!assetOpt)
    {
        throw std::runtime_error("[ResourceCache] Failed to parse glTF asset: " + key);
    }

    auto assetPtr = std::make_shared<fastgltf::Asset>(std::move(assetOpt.value()));
    mParsedAssetCache[key] = assetPtr;
    
    return assetPtr;
}
