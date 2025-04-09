#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <glm/gtc/quaternion.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

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

struct Vertex
{
	fastgltf::math::fvec3 position;
	fastgltf::math::fvec3 normal;
	fastgltf::math::fvec4 tangent;
	fastgltf::math::fvec2 uv;
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

struct MaterialUniforms
{
	fastgltf::math::fvec4 baseColorFactor;
	float alphaCutoff = 0.0f;
	float metallicFactor = 0.0f;
	float roughnessFactor = 0.0f;
	std::uint32_t flags = 0;

	fastgltf::math::fvec2 padding;
};

struct Viewer
{
	fastgltf::Asset asset;

	std::vector<Mesh> meshes;
	std::vector<Texture> textures;

	std::vector<MaterialUniforms> materials;
	std::vector<GLuint> materialBuffers;

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