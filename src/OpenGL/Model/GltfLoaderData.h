#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <glm/gtc/quaternion.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#define MAX_JOINTS 128

static constexpr auto supportedExtensions = 
	fastgltf::Extensions::KHR_mesh_quantization | 
	fastgltf::Extensions::KHR_texture_transform | 
	fastgltf::Extensions::KHR_materials_variants | 
	fastgltf::Extensions::MSFT_packing_occlusionRoughnessMetallic;

static constexpr auto gltfOptions = 
	fastgltf::Options::DontRequireValidAssetMember | 
	fastgltf::Options::AllowDouble | 
	fastgltf::Options::LoadGLBBuffers | 
	fastgltf::Options::LoadExternalBuffers | 
	fastgltf::Options::LoadExternalImages | 
	fastgltf::Options::GenerateMeshIndices;

struct IndirectDrawCommand
{
	std::uint32_t count;
	std::uint32_t instanceCount;
	std::uint32_t firstIndex;
	std::int32_t baseVertex;
	std::uint32_t baseInstance;
};

struct Joint
{
	size_t nodeIndex;
	int parentIndex = -1;
	int index = -1;
	std::vector<int> children;
	glm::mat4 inverseBindMatrix;
	glm::mat4 localTransform;
	glm::mat4 globalTransform;
	std::string name;

	glm::mat4 bindPose;
};

struct Skin
{
	std::vector<Joint> joints;
	std::vector<glm::mat4> jointMatrices;
	GLuint jointMatrixBuffer;
};

struct Vertex
{
	fastgltf::math::fvec3 position;
	fastgltf::math::fvec3 normal;
	fastgltf::math::fvec4 tangent;
	fastgltf::math::fvec2 uv;
	fastgltf::math::uvec4 joints;
	fastgltf::math::fvec4 weights;
};

struct Primitive
{
	IndirectDrawCommand draw;
	GLenum primitiveType;
	GLenum indexType;
	GLuint vertexArray;

	GLuint vertexBuffer;
	GLuint indexBuffer;

	std::size_t materialUniformsIndex;
	GLuint albedoTexture;
	GLuint metallicRoughnessTexture;
	GLuint emissiveTexture;
	GLuint occlusionTexture;
	GLuint normalTexture;
};

struct Mesh
{
	GLuint drawBuffer;
	std::vector<Primitive> primitives;
};

struct Texture
{
	GLuint texture;
};

enum MaterialUniformFlags : std::uint32_t
{
	None = 0 << 0,
	HasBaseColorTexture = 1 << 0,
	HasMetallicRoughnessTexture = 1 << 1,
	HasNormalTexture = 1 << 2,
	HasEmissiveTexture = 1 << 3,
	HasOcclusionTexture = 1 << 4
};

struct alignas(16) MaterialUniforms
{
	fastgltf::math::fvec4 baseColorFactor = fastgltf::math::fvec4(1.0f);
	fastgltf::math::fvec3 emissiveFactor = fastgltf::math::fvec3(0.0f);

	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
	float normalScale = 1.0f;
	float occlusionStrength = 1.0f;
	float alphaCutoff = 0.5f;

	uint64_t baseColorTextureHandle = 0;
	uint64_t metallicRoughnessTextureHandle = 0;
	uint64_t normalTextureHandle = 0;
	uint64_t occlusionTextureHandle = 0;
	uint64_t emissiveTextureHandle = 0;

	uint32_t flags = 0;
	uint32_t _padding = 0;

	//std::uint32_t _padding_flags = 0;

	//std::uint32_t baseColorLayerIndex = 0;
	//std::uint32_t metallicRoughnessLayerIndex = 0;
	//std::uint32_t normalLayerIndex = 0;
	//std::uint32_t occlusionLayerIndex = 0;
	//std::uint32_t emissiveLayerIndex = 0;

	//std::uint32_t _padding = 0;
	//std::uint32_t _padding_2 = 0;
};

struct Viewer
{
	fastgltf::Asset asset;

	std::vector<Mesh> meshes;
	std::vector<Texture> textures;

	std::vector<MaterialUniforms> materials;
	std::vector<GLuint> materialBuffers;

	std::vector<Skin> skins;

	GLint uvOffsetUniform = GL_NONE;
	GLint uvScaleUniform = GL_NONE;
	GLint uvRotationUniform = GL_NONE;

	std::size_t sceneIndex = 0;
	std::size_t materialVariant = 0;
};

struct Transform
{
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;

	glm::mat4 GetMatrix() const{
		glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 rot = glm::mat4_cast(rotation);
		glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);
		return trans * rot * sca;
	}
};

struct TransformGPU
{
	glm::vec3 position;
	float _pad1;
	glm::vec4 rotation;
	glm::vec3 scale;
	float _pad2;
};

struct SceneObjectGPU
{
	TransformGPU instanceTransform;
	glm::mat4 nodeTransform;

	uint32_t materialIndex;
	uint32_t skinIndex;
	uint32_t flags;
	uint32_t _pad;
};