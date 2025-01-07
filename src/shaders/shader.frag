#version 460 core

//in vec3 fragPos;
//in vec3 normal;

in vec3 N; // Normal in view space 
in vec3 L; // Light dir in view space 
in vec3 V; // View dir in view space 
in vec2 texCoord;

uniform sampler2D diffuseTex;
uniform samplerCube skybox;
uniform vec3 cameraPos;
uniform vec3 inDiffuseColor;
uniform float reflectionStrength;
uniform float specularPower;
uniform bool gamma;
uniform int uIsCurve;
uniform float alpha;

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
//	if (uIsCurve > 0)
//	{
//		// Apply tex col to fragment
//		fragColor = texture(diffuseTex, texcoord);
//	}
//	else
//	{
//		// Compute based on normal
//		fragColor = vec4(normalize(normal) * 0.5 + 0.5, 1.0);
//	}
	
	// Depth buffer debug
	//float depth = LinearizeDepth(gl_FragCoord.z) / far;
	//fragColor = vec4(vec3(depth), 1.0);

	// Debug Tex
	//fragColor = vec4(fragPos, 1.0); 
	//fragColor = vec4(normalize(normal) * 0.5 + 0.5, 1.0); 


	// Output Tex
	//fragColor = texture(diffuseTex, texCoord);

	// Reflect skybox
//	float ratio = 1.00 / 1.52;
//	vec3 I = normalize(fragPos - cameraPos);
//	vec3 R = refract(I, normalize(normal), ratio);
//
//	vec4 diffuseColor = texture(diffuseTex, texCoord);
//	vec4 reflectColor = texture(skybox, R);
//
//	fragColor = vec4(texture(skybox, R).rgb, 1.0);


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

	// Fresnel 
	float fresnel = pow(1.0 - max(dot(V, N), 0.0), 5.0);

	// Final Color
	vec3 oColor = ambientColor;
	oColor += diffuseColor * lambertian;
	oColor += specularColor * specular;

	// Reflection
	vec3 reflectedColor = texture(skybox, R).rgb;

	vec3 finalColor = mix(oColor, reflectedColor, 0.4); 

	fragColor = vec4(GammaCorrect(finalColor), 1.0);
}