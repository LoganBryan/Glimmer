#include "InstanceManager.h"

void InstanceManager::SetSceneObjects(const std::vector<SceneObject>& objs)
{
	mObjects = objs;
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
	std::string currentMeshPath = "";
	uint32_t currentBaseInstance = 0;

	for (const auto& obj : objs)
	{
		if (mMeshGroups.empty() || obj.meshPath != currentMeshPath)
		{
			currentMeshPath = obj.meshPath;
			mMeshGroups.push_back({ currentMeshPath, currentBaseInstance, 0 });
		}

		SceneObjectGPU gpuObj;

		gpuObj.instanceTransform.position = obj.transform.position;
		gpuObj.instanceTransform.rotation = glm::vec4(obj.transform.rotation.x, obj.transform.rotation.y, obj.transform.rotation.z, obj.transform.rotation.w);
		gpuObj.instanceTransform.scale = obj.transform.scale;
		gpuObj.nodeTransform = obj.transform.GetMatrix();

		// TODO: TEMP! This needs to be set correctly!
		gpuObj.materialIndex = 0;
		gpuObj.skinIndex = 0;
		gpuObj.flags = 0;

		mGpuObjects.push_back(gpuObj);

		mMeshGroups.back().instanceCount++;
		currentBaseInstance++;
	}

	if (instanceSSBO == 0)
		glCreateBuffers(1, &instanceSSBO);

	glNamedBufferStorage(instanceSSBO, mGpuObjects.size() * sizeof(SceneObjectGPU), mGpuObjects.data(), GL_DYNAMIC_STORAGE_BIT);
}

void InstanceManager::CullingCompute(const Camera& cam)
{
	// TODO: soon:tm:
}
