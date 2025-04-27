#include "InstanceManager.h"

void InstanceManager::SetSceneObjects(const std::vector<SceneObject>& objs)
{
	mObjects = objs;
	objectCount = objs.size();

	if (instanceSSBO == 0)
		glCreateBuffers(1, &instanceSSBO);

	glNamedBufferStorage(instanceSSBO, objectCount * sizeof(SceneObject), objs.data(), GL_DYNAMIC_STORAGE_BIT);
}

void InstanceManager::CullingCompute(const Camera& cam)
{
	// TODO: soon:tm:
}
