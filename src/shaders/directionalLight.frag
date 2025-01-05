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
	vec3 direction;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
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

	// Diffuse
	vec3 norm = normalize(normal);
	vec3 lightDir = normalize(-light.direction);
	
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = light.diffuse * diff * texture(material.diffuse, texCoords).rgb;

	// Specular
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = light.specular * spec * texture(material.specular, texCoords).rgb;

	// Emission
	vec3 emission = vec3(texture(material.emission, texCoords));
	emission.rgb = hueShift(emission);

	vec3 result = ambientLight + diffuse + specular + emission;
	fragColor = vec4(result, 1.0);
}