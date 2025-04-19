#version 460 core

in vec3 fragPosView;
in vec2 texCoord;
in mat3 TBN;
in vec3 N; // Normal in view space 
in vec3 V; // View dir in view space 
in vec3 normal;
in vec4 tangent;

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedoMetallic;
layout(location = 3) out vec2 gRoughAO;
layout(location = 4) out vec3 gEmissive;

uniform vec2 uvOffset;
uniform vec2 uvScale;
uniform float uvRotation;

const uint HAS_BASE_COLOR = 1;
const uint HAS_METALLIC_ROUGHNESS = 2;
const uint HAS_NORMAL_MAP = 4;
const uint HAS_EMISSIVE = 8;
const uint HAS_OCCLUSION = 16;

const float PI = 3.1415926;

layout(location = 0) uniform sampler2D albedoTexture;
layout(binding = 0, std140) uniform MaterialUniforms {
	vec4 baseColorFactor;
    float alphaCutoff;
    float metallicFactor;
    float roughnessFactor;
    uint flags;
} material;

layout(location = 1) uniform sampler2D metallicRoughnessTexture;
layout(location = 2) uniform sampler2D normalTexture;
layout(location = 3) uniform sampler2D emissiveTexture;
layout(location = 4) uniform sampler2D occlusionTexture;
layout(location = 5) uniform samplerCube skybox;

float rand(vec2 co) 
{
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 transformUV(vec2 uv) 
{
	mat2 rotationMat = mat2(cos(uvRotation), -sin(uvRotation), sin(uvRotation), cos(uvRotation));

	return rotationMat * uv * uvScale + uvOffset;
}

void main()
{
    // Base Color
    vec4 baseColor = material.baseColorFactor;
    if ((material.flags & HAS_BASE_COLOR) != 0)
    {
        vec4 texColor = texture(albedoTexture, transformUV(texCoord));
        if (baseColor.a < material.alphaCutoff) discard;
        baseColor.rgb *= pow(texColor.rgb, vec3(2.2));
    }

    // Normal map
	vec3 viewNormal = vec3(N); // Default to vertex normal
    if ((material.flags & HAS_NORMAL_MAP) == HAS_NORMAL_MAP) {
        vec3 tangentNormal = texture(normalTexture, transformUV(texCoord)).rgb;
		tangentNormal.g = 1.0 - tangentNormal.g;
		tangentNormal = tangentNormal * 2.0 - 1.0;
		viewNormal = normalize(TBN * tangentNormal);
    }

 	// Metallic-Roughness map
	float roughness = material.roughnessFactor;
	float metallic = material.metallicFactor;

	if ((material.flags & HAS_METALLIC_ROUGHNESS) == HAS_METALLIC_ROUGHNESS)
	{
		vec4 metRoughSample = texture(metallicRoughnessTexture, transformUV(texCoord));
        roughness *= metRoughSample.g;
        metallic *= metRoughSample.b;
	}

    // Occlusion Map
    float ambientOcclusion = 1.0;
	if ((material.flags & HAS_OCCLUSION) == HAS_OCCLUSION) {
		ambientOcclusion = texture(occlusionTexture, transformUV(texCoord)).r;
	}

    // Emissive Map
    if ((material.flags & HAS_EMISSIVE) == HAS_EMISSIVE)
    {
        vec3 emissiveTex = texture(emissiveTexture, transformUV(texCoord)).rgb;
        gEmissive = pow(emissiveTex, vec3(2.2));
    }
    else
        gEmissive = vec3(0.0);

    // Write to G-Buffer
    gPosition = vec4(fragPosView, 1.0);
    gNormal = vec4(normalize(viewNormal), 1.0);
    gAlbedoMetallic = vec4(baseColor.rgb, metallic);
    gRoughAO = vec2(roughness, ambientOcclusion);
}
