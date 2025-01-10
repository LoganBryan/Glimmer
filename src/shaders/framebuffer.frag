#version 460 core
out vec4 fragColor;

in vec2 texCoords;

layout(binding = 5) uniform sampler2D screenTexture;

const float offset = 1.0 / 300.0;
void main()
{
	vec2 offsets[9] = vec2[](
		vec2(-offset, offset),
		vec2(0.0, offset),
		vec2(offset, offset),
		vec2(-offset, 0.0),
		vec2(0.0, 0.0),
		vec2(offset, 0.0),
		vec2(-offset, -offset),
		vec2(0.0, -offset),
		vec2(offset, offset)
	);

//	float kernel[9] = float[](
//		1.0 / 16, 2.0 / 16, 1.0 / 16,
//		2.0 / 16, 4.0 / 16, 2.0 / 16,
//		1.0 / 16, 2.0 / 16, 1.0 / 16 
//	);
//
	float kernel[9] = float[](
		1, 1, 1,
		1, -8, 1,
		1, 1, 1
	);

	vec3 sampleTex[9];
	for (int i = 0; i < 9; i++)
	{
		sampleTex[i] = vec3(texture(screenTexture, texCoords.st + offsets[i]));
	}
	vec3 col = vec3(0.0);
	for (int i = 0; i < 9; i++)
	{
		col += sampleTex[i] * kernel[i];
	}
	fragColor = vec4(col, 1.0);

	fragColor = texture(screenTexture, texCoords);
}