#version 330 core

layout(location=0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 uv;

uniform mat4 mvp;
uniform mat4 modelWorldPos;
uniform mat4 shadowMtx;

out vec3 normDir;
out vec4 lightpos;


void main()
{
	gl_Position = mvp * vec4(pos,1);
	normDir = mat3(transpose(inverse(modelWorldPos))) * normal;
	lightpos= shadowMtx * vec4(pos, 1);
}