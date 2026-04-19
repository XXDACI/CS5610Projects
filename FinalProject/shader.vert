#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 texcoord;

uniform mat4 mvp;

out vec3 vWorldPos;

void main()
{
    vWorldPos = pos;
    gl_Position = mvp * vec4(pos, 1.0);
}