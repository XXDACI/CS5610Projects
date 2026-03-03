#version 330 core

layout(location = 0) in vec3 pos;

uniform mat4 lightmvp;

void main()
{
    gl_Position = lightmvp * (vec4(pos,1));
}