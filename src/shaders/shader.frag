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
const uint HAS_METALLIC_ROUGHNESS = 2;
const uint HAS_NORMAL_MAP = 4;
const uint HAS_EMISSIVE = 8;
const uint HAS_OCCLUSION = 16;

const float PI = 3.1415926;

// TODO: these are temp and need to be set as uniforms!
const float exposure = 0.8;
const vec3 lightColor = vec3(1.0);
const float lightIntensity = 5.0;
const float environmentIntensity = 0.2;


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

// Trowbridge-Reitz GGX
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);

	return a2 / (PI * pow((NdotH * NdotH * (a2 - 1.0) + 1.0), 2.0));
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;
	return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) * GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

// ACES tonemapping
vec3 ACESFilm(vec3 x) 
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
	vec4 baseColor = material.baseColorFactor;
	if ((material.flags & HAS_BASE_COLOR) == HAS_BASE_COLOR)
	{
		vec4 texColor = texture(albedoTexture, transformUV(texCoord));

		float factor = (rand(gl_FragCoord.xy) - 0.5) / 8;

		if (baseColor.a < material.alphaCutoff + factor)
			discard;

		baseColor *= vec4(pow(texColor.rgb, vec3(2.2)), texColor.a); 
	}
	else
	{
		baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
	}

	// Normal map transform
	vec3 viewNormal = vec3(N); // Default to vertex normal
    if ((material.flags & HAS_NORMAL_MAP) == HAS_NORMAL_MAP) {
        vec3 tangentNormal = texture(normalTexture, transformUV(texCoord)).rgb;
		tangentNormal.g = 1.0 - tangentNormal.g;
		tangentNormal = tangentNormal * 2.0 - 1.0;
		viewNormal = normalize(TBN * tangentNormal);
    }

	// Metallic-Roughness map
	float roughness = clamp(material.roughnessFactor, 0.05, 1.0);
	float metallic = clamp(material.metallicFactor, 0.0, 1.0);

	if ((material.flags & HAS_METALLIC_ROUGHNESS) == HAS_METALLIC_ROUGHNESS)
	{
		vec4 metRoughSample = texture(metallicRoughnessTexture, transformUV(texCoord));
		roughness = clamp(metRoughSample.g * material.roughnessFactor, 0.05, 1.0);
		metallic = clamp(metRoughSample.b * material.metallicFactor, 0.05, 1.0);
	}

	// Emissive map
	vec4 emissiveColor = vec4(0.0);
	if ((material.flags & HAS_EMISSIVE) == HAS_EMISSIVE) {
		emissiveColor = texture(emissiveTexture, transformUV(texCoord));
	}

	float ambientOcclusion = 1.0;
	if ((material.flags & HAS_OCCLUSION) == HAS_OCCLUSION) {
		ambientOcclusion = texture(occlusionTexture, transformUV(texCoord)).r;
	}

	// Fresnel
	float reflectance = mix(0.05, 0.17, roughness);
	vec3 F0 = mix(vec3(reflectance), baseColor.rgb, metallic);
	vec3 H = normalize(L + V);
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

	// Cook-Torrance BRDF
	float NDF = DistributionGGX(viewNormal, H, roughness);
	float G = GeometrySmith(viewNormal, V, L, roughness);
	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(viewNormal, V), 0.0) * max(dot(viewNormal, L), 0.0);
	vec3 specular = numerator / max(denominator, 0.001);

	// Add energy compensation for specular
	vec3 specularEnergyComp = 1.0 + F * (1.0 / max(NDF, 0.001) - 1.0);
	specular *= specularEnergyComp;

	// Energy Compensation
	float E = 1.0 / (roughness*roughness + 0.1);
	specular *= 1.0 + F * (E - 1.0);

	// Distance-based attenuation
	float dist = length(L);
	float attenuation = 1.0 / (dist * dist + 0.0001);
	vec3 radiance = lightColor * lightIntensity * attenuation;

	// Diffuse
	vec3 kD = (1.0 - F) * (1.0 - metallic);
	vec3 diffuse = kD * baseColor.rgb / PI;

	vec3 lighting = (diffuse + specular) * radiance * max(dot(viewNormal, L), 0.0);

	// Simplified IBL
	vec3 R = reflect(-V, viewNormal);
	vec3 reflection = textureLod(skybox, R, roughness * 4.0).rgb * environmentIntensity;
	lighting += reflection * F * ambientOcclusion;

	// Ambient
	vec3 irradiance = texture(skybox, viewNormal).rgb * environmentIntensity;
	vec3 ambient = (kD * irradiance + reflection * F) * environmentIntensity * ambientOcclusion;
	ambient *= mix(1.0, 2.5, 1.0 - metallic); // Ambient boost for non metal

	lighting += ambient;

	// Final Color
	vec3 finalColor = lighting * exposure; 
	finalColor = ACESFilm(finalColor);
	finalColor = pow(finalColor, vec3(1.0/2.2)); // Gamma correct
	finalColor += emissiveColor.rgb;

	fragColor = vec4(finalColor, baseColor.a);
}