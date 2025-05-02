#include "InstanceManager.h"

void InstanceManager::SetSceneObjects(const std::vector<SceneObject>& objs)
{
	objectCount = objs.size();

	mGpuObjects.clear();
	mMeshGroups.clear();

	if (objectCount == 0)
	{
		if (instanceSSBO != 0)
		{
			glDeleteBuffers(1, &instanceSSBO);
			instanceSSBO = 0;
		}
		return;
	}

	mGpuObjects.reserve(objectCount);
	ResourceCache& cache = ResourceCache::Get();

	std::string currentMeshPath = "";
	uint32_t currentBaseInstance = 0;
	size_t currentBaseMaterialIndex = 0;

	for (const auto& obj : objs)
	{
		if (mMeshGroups.empty() || obj.meshPath != currentMeshPath)
		{
			currentMeshPath = obj.meshPath;

			try
			{
				currentBaseMaterialIndex = cache.LoadMaterialsFromAsset(currentMeshPath);
			}
			catch (const std::runtime_error& e)
			{
				std::cerr << "[InstanceManager] Error! Loading materials for " << currentMeshPath << "\n What: " << e.what() << std::endl;
				currentBaseMaterialIndex = 0;
			}

			// TODO: Skin handling

			mMeshGroups.push_back({ currentMeshPath, currentBaseInstance, 0 });
		}

		SceneObjectGPU gpuObj;

		gpuObj.instanceTransform.position = obj.transform.position;
		gpuObj.instanceTransform.rotation = glm::vec4(obj.transform.rotation.x, obj.transform.rotation.y, obj.transform.rotation.z, obj.transform.rotation.w);
		gpuObj.instanceTransform.scale = obj.transform.scale;
		gpuObj.nodeTransform = obj.transform.GetMatrix();

		// TODO: TEMP! This needs to be set correctly!
		gpuObj.materialIndex = static_cast<uint32_t>(currentBaseMaterialIndex + obj.gltfMaterialIndex);  
		gpuObj.skinIndex = 0;
		gpuObj.flags = 0;

		mGpuObjects.push_back(gpuObj);

		mMeshGroups.back().instanceCount++;
		currentBaseInstance++;
	}

	if (instanceSSBO == 0)
		glCreateBuffers(1, &instanceSSBO);

	if (!mGpuObjects.empty())
		glNamedBufferStorage(instanceSSBO, mGpuObjects.size() * sizeof(SceneObjectGPU), mGpuObjects.data(), GL_DYNAMIC_STORAGE_BIT);
	else if (instanceSSBO != 0)
	{
		glDeleteBuffers(1, &instanceSSBO);
		instanceSSBO = 0;
	}
}

void InstanceManager::CullingCompute(const Camera& cam)
{
	// TODO: soon:tm:
}
