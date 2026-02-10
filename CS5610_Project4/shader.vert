#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;   // 暂时不用，但留着避免C++侧报错
layout(location=2) in vec2 st;

uniform mat4 mvp;

out vec2 texCoord;

void main()
{
    gl_Position = mvp * vec4(pos, 1.0);
    texCoord = st;
}
