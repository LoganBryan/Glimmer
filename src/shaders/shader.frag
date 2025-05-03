#version 460 core
#extension GL_ARB_bindless_texture : require

in mat3 TBN;
in vec3 N; // Normal in view space 
in vec3 V; // View dir in view space 
in vec3 fragPosView;
in vec2 texCoord;

flat in uint MaterialIndex;

layout(location = 0) out vec4 fragColor;

uniform vec2 uvOffset;
uniform vec2 uvScale;
uniform float uvRotation;

const float PI = 3.1415926;

uniform vec3 sunDirection;
uniform float sunIntensity;
uniform float environmentIntensity;

// TODO: these are temp and need to be set as uniforms!
const float exposure = 0.3;
const vec3 ambientColor = vec3(0.2);
const vec3 sunColor = vec3(1.0, 0.9, 0.8);

// cluster-grid
uniform float zNear;
uniform float zFar;
uniform uvec3 gridSize; 
uniform uvec2 screenDim;      

uniform uvec2 u_placeholderTextureHandle;
struct MaterialUniforms
{
	vec4 baseColorFactor;
	vec3 emissiveFactor;

	float metallicFactor;
	float roughnessFactor;
	float normalScale;
	float occlusionStrength;
	float alphaCutoff;

	uvec2 baseColorTextureHandle;
	uvec2 metallicRoughnessTextureHandle;

	uvec2 normalTextureHandle;
	uvec2 occlusionTextureHandle;

	uvec2 emissiveTextureHandle;

	uint flags;
	uint _padding1;
};
layout(std430, binding = 3) readonly buffer MaterialBuffer {
	MaterialUniforms materials[];
} materialData;

layout(binding = 5) uniform samplerCube skybox;

struct Light
{
	uint type; // 0 - Dir, 1 - Point, 2 - Spot, 3 - Area
	vec3 _padding;

	vec4 color; // w - intensity
	vec4 position; // point/ spot/ area 
	vec4 direction; // dir/ spot
	vec4 cutoff; // spot. x - inner, y - outer 
	vec4 attenuation; // x - constant, y - linear, z - quadratic, w - radius
	vec4 axisU; // area lights
	vec4 axisV; // area lights
};

struct Cluster
{
	vec4 minPoint;
	vec4 maxPoint;
	uint count;
	uint lightIndices[100];
};

layout(binding = 0, std430) buffer LightBuffer 
{
	Light lights[1024];
	uint lightCount;
};

layout(binding = 1, std430) restrict buffer clusterBuffer
{
	Cluster clusters[];
};

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

// Lighting calculations
vec3 CalcDirectionalLight(Light light, vec3 N, vec3 V, vec4 baseColor, float roughness, float metallic)
{
	vec3 L = normalize(light.direction.xyz);
	vec3 H = normalize(L + V);
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);

	vec3 F0 = mix(vec3(0.04), baseColor.rgb, metallic);
	vec3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - max(dot(H, V), 0.0), 0.0, 1.0), 5.0); 

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	vec3 specular = numerator / max(denominator, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);

	float NdotL = max(dot(N, L), 0.0);
	vec3 radiance = light.color.rgb * light.color.w;

	vec3 diffuse = kD * baseColor.rgb / PI;
	return (diffuse + specular) * radiance * NdotL;
}

vec3 CalcPointLight(Light light, vec3 N, vec3 V, vec4 baseColor, float roughness, float metallic)
{
	vec3 L = normalize(light.position.xyz - fragPosView);
	float dist = length(light.position.xyz - fragPosView);
	float atten = 1.0 / (light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * dist * dist);

	vec3 H = normalize(L + V);
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);

	vec3 F0 = mix(vec3(0.04), baseColor.rgb, metallic);
	vec3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - max(dot(H, V), 0.0), 0.0, 1.0), 5.0); 

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	vec3 specular = numerator / max(denominator, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);

	vec3 diffuse = kD * baseColor.rgb / PI;

	vec3 radiance = light.color.rgb * light.color.w * atten;
	return (diffuse + specular) * radiance * max(dot(N, L), 0.0);
}

vec3 CalcSpotLight(Light light, vec3 N, vec3 V, vec4 baseColor, float roughness, float metallic)
{
	vec3 L = normalize(light.position.xyz - fragPosView);
	float theta = dot(L, normalize(-light.direction.xyz));
	float epsilon = light.cutoff.x - light.cutoff.y;
	float intensity = clamp((theta - light.cutoff.y) / epsilon, 0.0, 1.0);

	float dist = length(light.position.xyz - fragPosView);
	float atten = 1.0 / (light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * dist * dist);

	vec3 H = normalize(L + V);
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);

	vec3 F0 = mix(vec3(0.04), baseColor.rgb, metallic);
	vec3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - max(dot(H, V), 0.0), 0.0, 1.0), 5.0); 

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	vec3 specular = numerator / max(denominator, 0.001);
	vec3 kD = (1.0 - F) * (1.0 - metallic);

	vec3 diffuse = kD * baseColor.rgb / PI;

	vec3 radiance = light.color.rgb * light.color.w * atten * intensity;
	return (diffuse + specular) * radiance * max(dot(N, L), 0.0);
}

vec3 CalcAreaLight(Light light, vec3 N, vec3 V, vec4 baseColor, float roughness, float metallic)
{
	int samples = 4;
	vec3 result = vec3(0.0);

	for (int u = -1; u <= 1; u += 2)
	{
		for (int v = -1; v <= 1; v += 2)
		{
			vec3 samplePos = light.position.xyz + u * light.axisU.xyz + v * light.axisV.xyz;
			vec3 L = normalize(samplePos - fragPosView);
			float dist = length(samplePos - fragPosView);
			float atten = 1.0 / (dist * dist + 0.001);

			vec3 H = normalize(L + V);
			float NDF = DistributionGGX(N, H, roughness);
			float G = GeometrySmith(N, V, L, roughness);

			vec3 F0 = mix(vec3(0.04), baseColor.rgb, metallic);
			vec3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - max(dot(H, V), 0.0), 0.0, 1.0), 5.0); 

			vec3 numerator = NDF * G * F;
			float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
			vec3 specular = numerator / max(denominator, 0.001);
			vec3 kD = (1.0 - F) * (1.0 - metallic);

			vec3 diffuse = kD * baseColor.rgb / PI;

			result += (diffuse + specular) * (light.color.rgb * light.color.w * atten) * max(dot(N, L), 0.0);
		}
	}
	return result / float(samples);
}

vec3 CalculateLighting(Light light, vec3 N, vec3 V, vec4 baseColor, float roughness, float metallic)
{
	vec3 result = vec3(0.0);
	
	if (light.type == 0u)
		result = CalcDirectionalLight(light, N, V, baseColor, roughness, metallic);
	else if (light.type == 1u)
		result = CalcPointLight(light, N, V, baseColor, roughness, metallic);
	else if (light.type == 2u)
		result = CalcSpotLight(light, N, V, baseColor, roughness, metallic);
	else if (light.type == 3u)
		result = CalcAreaLight(light, N, V, baseColor, roughness, metallic);

	return result;
}

uint ComputeClusterIndex()
{
	// Z-Slice
	float z = -fragPosView.z;
	uint zTile = uint(clamp((log(z/zNear) / log(zFar/zNear)) * float(gridSize.z), 0.0, float(gridSize.z-1)));

	// X/Y tiles
	vec2 tileSize = vec2(screenDim) / vec2(gridSize.xy);
	uvec2 xyTile = uvec2(gl_FragCoord.xy / tileSize);

	return xyTile.x + xyTile.y * gridSize.x + zTile * (gridSize.x * gridSize.y);
}

void main()
{
	MaterialUniforms mat = materialData.materials[MaterialIndex];
	vec2 finalUV = transformUV(texCoord);

    vec4 baseColor = mat.baseColorFactor;

//	if (((mat.flags & 1u) != 0u) && (mat.baseColorTextureHandle != u_placeholderTextureHandle))
	if ((mat.baseColorTextureHandle != u_placeholderTextureHandle))
	{
		sampler2D baseColorSampler = sampler2D(mat.baseColorTextureHandle);
		vec4 texColor = texture(baseColorSampler, finalUV);
		baseColor *= texColor;
	}
	if (baseColor.a < mat.alphaCutoff) {
        discard;
    }

	// Normal map transform
	vec3 viewNormal = vec3(N); // Default to vertex normal
//	if (((mat.flags & 4u) != 0u) && (mat.normalTextureHandle != u_placeholderTextureHandle))
	if ((mat.normalTextureHandle != u_placeholderTextureHandle))
	{
		sampler2D normalSampler = sampler2D(mat.normalTextureHandle);
		vec3 tangentNormal = texture(normalSampler, finalUV).rgb * 2.0 - 1.0;
		viewNormal = normalize(TBN * tangentNormal);
	}
	
	// Metallic-Roughness map
	float roughness = clamp(mat.roughnessFactor, 0.05, 1.0);
	float metallic = clamp(mat.metallicFactor, 0.0, 1.0);

//	if (((mat.flags & 2u) != 0u) && (mat.metallicRoughnessTextureHandle != u_placeholderTextureHandle))
	if ((mat.metallicRoughnessTextureHandle != u_placeholderTextureHandle))
	{
		sampler2D metallicRoughnessSampler = sampler2D(mat.metallicRoughnessTextureHandle);
		vec4 mrSample = texture(metallicRoughnessSampler, finalUV);

		metallic = clamp(mrSample.b * mat.metallicFactor, 0.0, 1.0);
		roughness = clamp(mrSample.g * mat.roughnessFactor, 0.05, 1.0);
	}

	// Ambient Occlusion
	float ambientOcclusion = 1.0;

//	if (((mat.flags & 16u) != 0u) && (mat.occlusionTextureHandle != u_placeholderTextureHandle))
	if ((mat.occlusionTextureHandle != u_placeholderTextureHandle))
	{
		sampler2D occlusionSampler = sampler2D(mat.occlusionTextureHandle);
		float occlusionSample = texture(occlusionSampler, finalUV).r;

		ambientOcclusion = mix(1.0, occlusionSample, mat.occlusionStrength);
	}

	// Emissive map
    vec3 emissive = mat.emissiveFactor; 

//	if (((mat.flags & 8u) != 0u) && (mat.emissiveTextureHandle != u_placeholderTextureHandle))
	if ((mat.emissiveTextureHandle != u_placeholderTextureHandle))
	{
		sampler2D emissiveSampler = sampler2D(mat.emissiveTextureHandle);
		vec3 emissiveSample = texture(emissiveSampler, finalUV).rgb;

		emissive *= emissiveSample.rgb;
	}
	
	// Fetch Cluster
	vec3 lighting = vec3(0.0);
	uint clusterId = ComputeClusterIndex();
	Cluster c = clusters[clusterId];

	// Loop over lights that intersect
	for (uint i = 0u; i < c.count; i++)
	{
		uint li = c.lightIndices[i];
		Light l = lights[li];

		lighting += CalculateLighting(l, viewNormal, V, baseColor, roughness, metallic);
	}

	// Add sun light seperate (Doesn't get culled)
	Light sunLight;
	sunLight.type = 0u;
	sunLight._padding = vec3(0.0);
	sunLight.color = vec4(sunColor, sunIntensity);
	sunLight.position = vec4(0.0);
	sunLight.direction = vec4(sunDirection, 0.0);
	sunLight.cutoff = vec4(0.0);
	sunLight.attenuation = vec4(0.0);
	sunLight.axisU = vec4(0.0);
	sunLight.axisV = vec4(0.0);

	vec3 sunRadiance = sunColor * sunIntensity;
	vec3 sunContribution = CalcDirectionalLight(sunLight, viewNormal, V, baseColor, roughness * 0.5 + 0.5, metallic); // Roughness changes causes it to be less sharp decreasing the specular highlight
	lighting += sunRadiance * sunContribution;

	// Fresnel
	float NdotV = max(dot(viewNormal, V), 0.0);
	vec3 F0 = mix(vec3(0.04), baseColor.rgb, metallic);
	vec3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
	vec3 kD = (1.0 - F) * (1.0 - metallic);

	// Simplified IBL
	vec3 R = reflect(-V, viewNormal);
	vec3 reflection = textureLod(skybox, R, roughness * 4.0).rgb;
	lighting += reflection * F * ambientOcclusion;

	// Ambient
	vec3 irradiance = texture(skybox, viewNormal).rgb;
	vec3 indirectLighting = (kD * irradiance + reflection * F) * environmentIntensity;
	vec3 skyboxAmbient = indirectLighting * ambientOcclusion;
	vec3 staticAmbient = ambientColor * 0.1; // Ambient for unlit sections
	vec3 ambient = skyboxAmbient + staticAmbient;
	ambient *= mix(1.0, 2.5, 1.0 - metallic); // Ambient boost for non metal

	lighting += ambient;

	// Final Color
	vec3 finalColor = lighting * exposure; 
	finalColor = ACESFilm(finalColor);
	finalColor = pow(finalColor, vec3(1.0/2.2)); // Gamma correct
	finalColor += emissive;

	fragColor = vec4(finalColor, baseColor.a);
}