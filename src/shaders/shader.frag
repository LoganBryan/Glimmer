#version 460 core

in mat3 TBN;
in vec3 N; // Normal in view space 
in vec3 L; // Light dir in view space 
in vec3 V; // View dir in view space 
in vec2 texCoord;
in vec3 normal;
in vec4 tangent;

layout(location = 0) out vec4 fragColor;

uniform vec2 uvOffset;
uniform vec2 uvScale;
uniform float uvRotation;

const uint HAS_BASE_COLOR = 1;

layout(location = 0) uniform sampler2D albedoTexture;
layout(binding = 0, std140) uniform MaterialUniforms {
	vec4 baseColorFactor;
	float alphaCutoff;
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
	vec4 baseColor = material.baseColorFactor;
	if ((material.flags & HAS_BASE_COLOR) == HAS_BASE_COLOR)
	{
		baseColor *= texture(albedoTexture, transformUV(texCoord));
	}
	float factor = (rand(fragColor.xy) - 0.5) / 8;

	if (baseColor.a < material.alphaCutoff + factor)
		discard;

	// Normal map transform
	vec3 tangentNormal = texture(normalTexture, transformUV(texCoord)).rgb * 2.0 - 1.0;
	vec3 viewNormal = normalize(TBN * tangentNormal);

	// Metallic-Roughness map
	vec2 metallicRoughness = texture(metallicRoughnessTexture, transformUV(texCoord)).gb;
	float ambientOcclusion = texture(occlusionTexture, transformUV(texCoord)).r;
	float roughness = clamp(metallicRoughness.x, 0.05, 1.0);
	float metallic = clamp(metallicRoughness.r, 0.0, 1.0);

	// Emissive map
	vec4 emissiveColor = texture(emissiveTexture, transformUV(texCoord));

	// Light and view vectors 
	vec3 H = normalize(L + V); // Halfway vector
	vec3 R = reflect(-V, N); // Reflection vector

	// Fresnel-Schlick specular
	//vec4 F0 = mix(vec4(0.04), baseColor, metallic); // Dielectric 0.04
	//vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - max(dot(viewNormal, V), 0.0), 5.0);

	// Diffuse
	float lambertian = max(dot(viewNormal, L), 0.0);
	vec4 diffuse = (1.0 - metallic) * baseColor * lambertian;

	float specularPower = 32.0; // TODO: Change this to a uniform!

	// Specular
	float specAngle = max(dot(viewNormal, H), 0.0);
	float specular = pow(specAngle, specularPower);

	// Diffuse + Specular
	vec4 lighting = diffuse + specular;

	// Skybox reflection 
	vec4 reflectionColor = texture(skybox, R);

	// Final Color
	vec4 finalColor = mix(lighting, reflectionColor, roughness);
	finalColor += emissiveColor; // Apply emissive
	finalColor *= ambientOcclusion; // Apply AO

	fragColor = finalColor;
}