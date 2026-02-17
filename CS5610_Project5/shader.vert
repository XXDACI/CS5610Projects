#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 st;

uniform mat4 mvp;
uniform mat4 modelWorldPos;

out vec3 worldPos;
out vec3 normDir;
out vec2 texCoord;

void main()
{
	gl_Position = mvp * vec4(pos,1);
	worldPos = vec3(modelWorldPos * vec4(pos, 1.0));
	normDir = mat3(transpose(inverse(modelWorldPos))) * normal;
	texCoord = st;
}