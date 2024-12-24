#version 460 core

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D uTexture;
uniform sampler2D uTexture2;

void main()
{
	fragColor = mix(texture(uTexture, texCoord).rgba, texture(uTexture2, texCoord).rgba, 0.8f);
	//fragColor = vec4(texCoord, 0.0f, 1.0f);
}