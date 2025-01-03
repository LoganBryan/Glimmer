#version 460 core
out vec4 fragColor;

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	sampler2D emission;
	float shininess;
};

struct Light {
	vec3 position;
	
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

void main()
{
	// Ambient
	vec3 ambientLight = light.ambient * texture(material.diffuse, texCoords).rgb;

	// Diffuse
	vec3 norm = normalize(normal);
	vec3 lightDir = normalize(light.position - fragPos);
	
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = light.diffuse * diff * texture(material.diffuse, texCoords).rgb;

	// Specular
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
	vec3 specular = light.specular * spec * texture(material.specular, texCoords).rgb;

	// Emission
	vec3 emission = texture(material.emission, texCoords).rgb;

	vec3 result = ambientLight + diffuse + specular + emission;
	fragColor = vec4(result, 1.0f);
}