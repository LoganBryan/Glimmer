#pragma once
#include <OpenGL/Model/GltfLoaderData.h>
#include <OpenGL/Application/Camera.h>

struct MeshGroupInfo
{
	std::string meshPath;
	uint32_t instanceOffset;
	uint32_t instanceCount;
};

struct SceneObject
{
	std::string meshPath;
	Transform transform;

	// XR grabbing - might be temp
	glm::vec3 grabOffset;
	glm::quat grabRotationOffset;
	bool isGrabbed = false;
	int grabbedByHand = -1; // 0 = left. 1 = right
};

class InstanceManager
{
public:
	void SetSceneObjects(const std::vector<SceneObject>& objs);
	void CullingCompute(const Camera& cam);

	inline const Transform& GetTransform(size_t idx) const { return mObjects[idx].transform; }

	inline const GLuint GetSSBO() const { return instanceSSBO; }
	inline const size_t Count() const { return objectCount; }
	inline const std::vector<MeshGroupInfo>& GetMeshGroups() const { return mMeshGroups; }
private:
	std::vector<SceneObject> mObjects;
	GLuint instanceSSBO = 0;
	size_t objectCount = 0;

	std::vector<SceneObjectGPU> mGpuObjects;
	std::vector<MeshGroupInfo> mMeshGroups;
};

