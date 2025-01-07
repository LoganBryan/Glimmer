#version 460 core

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;

uniform sampler2D diffuseTex;
uniform int uIsCurve;

out vec4 fragColor;

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
	
	// Debug
	//fragColor = vec4(fragPos, 1.0); 
	//fragColor = vec4(normalize(normal) * 0.5 + 0.5, 1.0); 

	// Output Tex
	fragColor = texture(diffuseTex, texCoord);
}