#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;

uniform mat4 mvp;
uniform mat4 model;

out vec2 vUV;
out vec3 vWorldPos;
out mat3 vTBN;

void main()
{
    gl_Position = mvp * vec4(pos, 1.0);
    vUV = uv;

    vec3 T = normalize((model * vec4(1.0, 0.0, 0.0, 0.0)).xyz);
    vec3 B = normalize((model * vec4(0.0, 1.0, 0.0, 0.0)).xyz);
    vec3 N = normalize((model * vec4(0.0, 0.0, 1.0, 0.0)).xyz);

    vTBN = mat3(T, B, N);
    vWorldPos = (model * vec4(pos, 1.0)).xyz;
}