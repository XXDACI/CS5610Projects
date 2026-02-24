#version 330 core
layout(location = 0) in vec3 pos;

out vec3 vDir;

uniform mat4 proj;
uniform mat3 viewRot;

void main()
{
    vec3 p = viewRot * pos;
    vDir = pos;

    vec4 clipPos = proj * vec4(p, 1.0);
    gl_Position = clipPos.xyww;
}