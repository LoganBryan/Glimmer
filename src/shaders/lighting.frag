#version 460 core

in vec2 TexCoord;
out vec4 fragColor;

// G-Buffer
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoMetallic;
uniform sampler2D gRoughAO;
uniform sampler2D gEmissive;

const float PI = 3.1415926;
vec3 fragPosView;

uniform vec3 sunDirection;
uniform float sunIntensity;
uniform float environmentIntensity;
uniform samplerCube skybox;

// TODO: these are temp and need to be set as uniforms!
const float exposure = 0.3;
const vec3 ambientColor = vec3(0.2);
const vec3 sunColor = vec3(1.0, 0.9, 0.8);

struct Light
{
	uint type; // 0 - Dir, 1 - Point, 2 - Spot, 3 - Area
	vec3 _padding;

	vec4 color; // w - intensity
	vec4 position; // point/ spot/ area 
	vec4 direction; // dir/ spot
	vec4 cutoff; // spot. x - inner, y - outer 
	vec4 attenuation; // x - constant, y - linear, z - quadratic
	vec4 axisU; // area lights
	vec4 axisV; // area lights
};

layout(binding = 0, std430) buffer LightBuffer 
{
	uint lightCount;
	Light lights[];
};

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

	float reflectance = mix(0.05, 0.17, roughness);
	vec3 F0 = mix(vec3(reflectance), baseColor.rgb, metallic);
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

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

	float reflectance = mix(0.05, 0.17, roughness);
	vec3 F0 = mix(vec3(reflectance), baseColor.rgb, metallic);
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

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

	float reflectance = mix(0.05, 0.17, roughness);
	vec3 F0 = mix(vec3(reflectance), baseColor.rgb, metallic);
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

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

			float reflectance = mix(0.05, 0.17, roughness);
			vec3 F0 = mix(vec3(reflectance), baseColor.rgb, metallic);
			vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

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

void main()
{
	fragPosView = texture(gPosition, TexCoord).rgb;
	vec3 normal = normalize(texture(gNormal, TexCoord).rgb);
	vec3 albedo = texture(gAlbedoMetallic, TexCoord).rgb;
	vec3 emissive = texture(gEmissive, TexCoord).rgb;
	float metallic = texture(gAlbedoMetallic, TexCoord).a;
	float roughness = texture(gRoughAO, TexCoord).r;
	float ambientOcclusion = texture(gRoughAO, TexCoord).g;

	vec4 baseColor = vec4(albedo, 1.0);

	// View direction
	vec3 V = normalize(-fragPosView);

	vec3 lighting = vec3(0.0);
	for (uint i = 0u; i < lightCount; i++)
	{
		lighting += CalculateLighting(lights[i], normal, V, baseColor, roughness, metallic);
	}

	Light sunLight;
	sunLight.type = 0u;
	sunLight.color = vec4(sunColor, sunIntensity);
	sunLight.direction = vec4(sunDirection, 0.0);

	vec3 sunRadiance = sunColor * sunIntensity;
	vec3 sunContribution = CalcDirectionalLight(sunLight, normal, V, baseColor, roughness, metallic);
	lighting += sunContribution;

	// Fresnel
	float NdotV = max(dot(normal, V), 0.0);
	float reflectance = mix(0.05, 0.17, roughness);
	vec3 F0 = mix(vec3(reflectance), baseColor.rgb, metallic);
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
	vec3 kD = (1.0 - F) * (1.0 - metallic);

	// Simplified IBL
	vec3 R = reflect(-V, normal);
	vec3 reflection = textureLod(skybox, R, roughness * 4.0).rgb;
	lighting += reflection * F * ambientOcclusion;

	// Ambient
	vec3 irradiance = texture(skybox, normal).rgb;
	vec3 indirectLighting = (kD * irradiance + reflection * F) * environmentIntensity;
	vec3 skyboxAmbient = indirectLighting * ambientOcclusion;
	vec3 staticAmbient = ambientColor * 0.1; // Ambient for unlit sections
	vec3 ambient = skyboxAmbient + staticAmbient;
	ambient *= mix(1.0, 2.5, 1.0 - metallic); // Ambient boost for non metal

	lighting += ambient;

	// Final Color
	vec3 finalColor = (lighting + emissive) * exposure; 
	finalColor = ACESFilm(finalColor);
	finalColor = pow(finalColor, vec3(1.0/2.2)); // Gamma correct

	fragColor = vec4(finalColor, 1.0);
}