#version 460 core
out vec4 fragColor;

struct Material {
	float shininess;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct Light {
	vec3 position;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

in vec3 fragPos;
in vec3 normal;

uniform vec3 viewPos;
uniform float shininess;
uniform Material material;
uniform Light light;


uniform vec3 lightColor;
uniform vec3 lightPos;

void main()
{
	vec3 tempObAmb = {1.0f, 0.5f, 0.31f};
	vec3 tempObDiff = {1.0f, 0.5f, 0.31f};
	vec3 tempObSpec = {0.5f, 0.5f, 0.5f};

	// Ambient
	vec3 ambientLight = light.ambient * material.ambient;

	// Diffuse
	vec3 norm = normalize(normal);
	vec3 lightDir = normalize(light.position - fragPos);
	
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = light.diffuse * (diff * material.diffuse);

	// Specular
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = reflect(-lightDir, norm);

	float spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
	vec3 specular = light.specular * (spec * material.specular);

	vec3 result = ambientLight + diffuse + specular;

	fragColor = vec4(result, 1.0f);
	
}