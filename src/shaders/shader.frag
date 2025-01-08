#version 460 core

//in vec3 fragPos;
//in vec3 normal;

in vec3 N; // Normal in view space 
in vec3 L; // Light dir in view space 
in vec3 V; // View dir in view space 
in vec2 texCoord;

uniform sampler2D diffuseTex;
uniform samplerCube skybox;
uniform float specularPower;
uniform bool gamma;

out vec4 fragColor;

float near = 0.1;
float far = 100.0;

float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 10; // Convert depth to NDC
	return (2.0 * near * far) / (far + near - z * (far - near));
}

vec3 GammaCorrect(vec3 color)
{
	if (gamma)
	{
		return pow(color, vec3(1 / 2.2));
	}

	return color;
}

void main()
{
	// test blinn-phong
	float lambertian = max(dot(L, N), 0.0);

	// Halfway Vector 
	vec3 H = normalize(L + V);

	// Reflection vector
	vec3 R = reflect(-V, N);

	// Specular 
	float specAngle = max(dot(H, N), 0.0);
	float specular = pow(specAngle, specularPower);
	//specular = ((specularPower + 8) / 8) * specular; // Normalize specular

	// Material
	vec3 diffuseColor = texture(diffuseTex, texCoord).rgb;
	vec3 ambientColor = diffuseColor * 0.4;
	vec3 specularColor = vec3(1.0);

	// Final Color
	vec3 oColor = ambientColor;
	oColor += diffuseColor * lambertian;
	oColor += specularColor * specular;

	// Reflection
	vec3 reflectedColor = texture(skybox, R).rgb;

	vec3 finalColor = mix(oColor, reflectedColor, 0.4); 

	fragColor = vec4(GammaCorrect(finalColor), 1.0);
}