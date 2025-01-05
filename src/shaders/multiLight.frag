#version 460 core
#define N_POINTLIGHTS 4

out vec4 fragColor;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	sampler2D emission;

	float emissionShift;
	float shininess;
};
uniform Material material;

struct DirectionalLight {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
uniform DirectionalLight directionalLight;
vec3 CalcDirectional(in DirectionalLight light, in vec3 normal, in vec3 viewDir);

struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
uniform PointLight pointLights[N_POINTLIGHTS];
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

uniform vec3 viewPos;
//uniform Light light;

const mat3 YIQMAT = mat3(
	0.299, 0.596, 0.211,
	0.587, -0.274, -0.523,
	0.114, -0.322, 0.312
);
const mat3 RGBMAT = mat3(
	1.0, 1.0, 1.0,
	0.956, -0.272, -1.106,
	0.621, -0.647, 1.703
);

vec3 toYIQ(in vec3 color)
{
	return YIQMAT * color;
}
vec3 toRGB(in vec3 color)
{
	return RGBMAT * color;
}

vec3 hueShift(in vec3 color)
{
	vec3 YIQ = toYIQ(color);

	mat2 rotationalMat = mat2(
		cos(material.emissionShift), -sin(material.emissionShift),
		sin(material.emissionShift), cos(material.emissionShift)
	);
	YIQ.yz *= rotationalMat;

	return toRGB(YIQ);
}

void main()
{
	vec3 norm = normalize(normal);
	vec3 viewDir = normalize(viewPos - fragPos);

	// Directional
	vec3 result = CalcDirectional(directionalLight, norm, viewDir);

	// Point lights
	for (int i = 0; i < N_POINTLIGHTS; i++)
	{
		result += CalcPointLight(pointLights[i], norm, fragPos, viewDir);
	}

	fragColor = vec4(result, 1.0);
}

vec3 CalcDirectional(in DirectionalLight light, in vec3 normal, in vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);

	// Diffuse
	float diff = max(dot(normal, lightDir), 0.0);

	// Specular 
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

	// Combine 
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, texCoords));
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, texCoords));
	vec3 specular = light.specular * spec * vec3(texture(material.specular, texCoords));

	return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - fragPos);

	// Diffuse
	float diff = max(dot(normal, lightDir), 0.0);

	// Specular 
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

	// Attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	// Combine
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, texCoords));
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, texCoords));
	vec3 specular = light.specular * spec * vec3(texture(material.specular, texCoords));

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	return ambient + diffuse + specular;
}