#version 460 core

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;

uniform sampler2D diffuseTex;
uniform samplerCube skybox;
uniform vec3 cameraPos;
uniform float reflectionStrength;
uniform int uIsCurve;

out vec4 fragColor;

float near = 0.1;
float far = 100.0;

float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 10; // Convert depth to NDC
	return (2.0 * near * far) / (far + near - z * (far - near));
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
	float ratio = 1.00 / 1.52;
	vec3 I = normalize(fragPos - cameraPos);
	vec3 R = refract(I, normalize(normal), ratio);

	vec4 diffuseColor = texture(diffuseTex, texCoord);
	vec4 reflectColor = texture(skybox, R);

	fragColor = vec4(texture(skybox, R).rgb, 1.0);

}