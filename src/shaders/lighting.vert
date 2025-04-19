#version 460 core

out vec2 TexCoord;

void main() 
{
	vec2 pos = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1.0;
	gl_Position = vec4(pos, 0.0, 1.0);
	TexCoord = pos * 0.5 + 0.5;
}