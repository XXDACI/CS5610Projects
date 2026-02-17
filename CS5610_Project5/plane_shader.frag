#version 330 core
layout(location = 0) out vec4 color;

uniform sampler2D rt;
in vec2 vUV;

void main(){
	vec3 c = texture(rt, vUV).rgb;
	c = clamp(c + vec3(0.05), 0.0, 1.0);

	color = vec4(c, 1.0);
}