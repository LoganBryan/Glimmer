#version 460 core

in vec2 texCoord;

uniform sampler2D uTexture;
uniform sampler2D uTexture2;

out vec4 fragColor;

void main()
{
	fragColor = mix(texture(uTexture, texCoord).rgba, texture(uTexture2, texCoord).rgba, 0.8);
	//fragColor = vec4(texCoord, 0.0f, 1.0f);
}