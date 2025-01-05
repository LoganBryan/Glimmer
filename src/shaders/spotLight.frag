#version 460 core
out vec4 fragColor;

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	sampler2D emission;

	float emissionShift;
	float shininess;
};

struct Light {
	vec3 position;
	vec3 direction;
	float cutOff;
	float outerCutOff;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;
};

in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

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
	// Ambient
	vec3 ambientLight = light.ambient * texture(material.diffuse, texCoords).rgb;

	vec3 lightDir = normalize(light.position - fragPos);

	// Diffuse
	vec3 norm = normalize(normal);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = light.diffuse * diff * texture(material.diffuse, texCoords).rgb;

	// Specular
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = light.specular * spec * texture(material.specular, texCoords).rgb;


	// Spotlight
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.cutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
	diffuse *= intensity;
	specular *= intensity;

	// Light Attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	ambientLight *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	// Emission
	vec3 emission = vec3(texture(material.emission, texCoords));
	emission.rgb = hueShift(emission);

	vec3 result = ambientLight + diffuse + specular;
	fragColor = vec4(result, 1.0);
	//fragColor = vec4(vec3(theta), 1.0);
}