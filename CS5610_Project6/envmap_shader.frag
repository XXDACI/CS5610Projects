#version 330 core
in vec3 vDir;
out vec4 color;

uniform samplerCube env;

void main()
{
    color = texture(env, normalize(vDir));
}