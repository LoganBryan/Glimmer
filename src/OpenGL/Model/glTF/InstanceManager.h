#pragma once
#include <OpenGL/Model/GltfLoaderData.h>
#include <OpenGL/Application/Camera.h>

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

	inline GLuint GetSSBO() const { return instanceSSBO; }
	inline size_t Count() const { return objectCount; }
private:
	std::vector<SceneObject> mObjects;
	GLuint instanceSSBO = 0;
	size_t objectCount = 0;
};

