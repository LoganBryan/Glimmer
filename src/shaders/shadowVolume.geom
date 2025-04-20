#version 460 core

layout(triangles_adjacency) in;
layout(triangle_strip, max_vertices = 18) out;

in vec3 position[];
in vec3 normal[];

uniform vec4 LightPosition;
uniform vec4 ProjMatrix;

bool FacesLight(vec3 a, vec3 b, vec3 c)
{
	vec3 n = cross(b - a, c - a);
	vec3 da = LightPosition.xyz - a;
	vec3 db = LightPosition.xyz - b;
	vec3 dc = LightPosition.xyz - c;

	return dot(n, da) > 0 || dot(n, db) > 0 || dot(n, dc) > 0;
}

void EmitEdgeQuad(vec3 a, vec3 b)
{
	gl_Position = ProjMatrix * vec4(a, 1);
	EmitVertex();

	gl_Position = ProjMatrix * vec4(a - LightPosition.xyz, 0);
	EmitVertex();

	gl_Position = ProjMatrix * vec4(b, 1);
	EmitVertex();

	gl_Position = ProjMatrix * vec4(b - LightPosition.xyz, 0);
	EmitVertex();
	EndPrimitive();
}

void main()
{
	if (FacesLight(position[0], position[2], position[4]))
	{
		if (!FacesLight(position[0], position[1], position[2]))
			EmitEdgeQuad(position[0], position[2]);

		if (!FacesLight(position[2], position[3], position[4]))
			EmitEdgeQuad(position[2], position[4]);

		if (!FacesLight(position[4], position[5], position[0]))
			EmitEdgeQuad(position[4], position[0]);
	}
}