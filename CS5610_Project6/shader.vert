#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

uniform mat4 mvp;
uniform mat4 modelWorldPos;

out vec3 worldPos;
out vec3 normDir;

void main()
{
    gl_Position = mvp * vec4(pos,1);
    worldPos = vec3(modelWorldPos * vec4(pos, 1.0));
    normDir  = mat3(transpose(inverse(modelWorldPos))) * normal;
}