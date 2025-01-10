#version 460 core

in vec3 fragPos;
in vec3 normal;

in mat3 TBN;
in vec3 N; // Normal in view space 
in vec3 L; // Light dir in view space 
in vec3 V; // View dir in view space 
in vec2 texCoord;
in vec4 tangent;

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D aoMap;
layout(binding = 2) uniform sampler2D emissiveMap;
layout(binding = 3) uniform sampler2D metallicRoughnessMap;
layout(binding = 4) uniform sampler2D normalMap;

layout(binding = 5) uniform samplerCube skybox;



uniform float specularPower;
uniform bool gamma;

out vec4 fragColor;

const float PI = 3.14159265358979323846;

// TODO what happens if we're missing some textures like normal or metallic? should check for that

void main()
{
	// Albedo map
	vec3 baseColor = texture(albedoMap, texCoord).rgb;

	// Normal map transform
	vec3 tangentNormal = texture(normalMap, texCoord).rgb * 2.0 - 1.0;
	vec3 viewNormal = normalize(TBN * tangentNormal);

	// Metallic-Roughness map
	vec2 metallicRoughness = texture(metallicRoughnessMap, texCoord).gb;
	float ambientOcclusion = texture(aoMap, texCoord).r;
	float roughness = clamp(metallicRoughness.x, 0.05, 1.0);
	float metallic = clamp(metallicRoughness.r, 0.0, 1.0);

	// Emissive map
	vec3 emissiveColor = texture(emissiveMap, texCoord).rgb;

	// Light and view vectors 
	vec3 H = normalize(L + V); // Halfway vector
	vec3 R = reflect(-V, N); // Reflection vector

	// Fresnel-Schlick specular
	vec3 F0 = mix(vec3(0.04), baseColor, metallic); // Dielectric 0.04
	vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - max(dot(viewNormal, V), 0.0), 5.0);

	// Diffuse
	float lambertian = max(dot(viewNormal, L), 0.0);
	vec3 diffuse = (1.0 - metallic) * baseColor * lambertian;

	// Specular
	float specAngle = max(dot(viewNormal, H), 0.0);
	float specular = pow(specAngle, specularPower);

	// Diffuse + Specular
	vec3 lighting = diffuse + specular;

	// Skybox reflection
	vec3 reflectionColor = texture(skybox, R).rgb;

	// Final Color
	vec3 finalColor = mix(lighting, reflectionColor, roughness);
	finalColor += emissiveColor; // Apply emissive
	finalColor *= ambientOcclusion; // Apply AO

	fragColor = vec4(finalColor, 1.0);
}