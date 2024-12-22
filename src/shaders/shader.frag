#version 460 core

out vec4 fragColor;
in vec3 uColor;

void main()
{
	fragColor = vec4(uColor, 1.0f);
}