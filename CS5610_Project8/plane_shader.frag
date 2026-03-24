#version 330 core
layout(location = 0) out vec4 color;

uniform sampler2D normalTex;
uniform vec3 cameraPos;
uniform vec3 lightPos;

in vec2 vUV;
in vec3 vWorldPos;
in mat3 vTBN;

void main()
{
    vec2 uv = vec2(vUV.x, 1.0 - vUV.y);

    vec3 tangentNormal = texture(normalTex, uv).rgb;
    tangentNormal = tangentNormal * 2.0 - 1.0;

    vec3 n = normalize(vTBN * tangentNormal);

    vec3 lightVec = lightPos - vWorldPos;
    vec3 l = normalize(lightVec);
    vec3 v = normalize(cameraPos - vWorldPos);
    vec3 h = normalize(l + v);

    float dist = length(lightVec);
    float attenuation = 1.0 / (1.0 + 0.35 * dist + 0.1 * dist * dist);

    float a = 0.04;
    float d = max(dot(n, l), 0.0);
    float s = pow(max(dot(n, h), 0.0), 32.0);

    vec3 ambient  = a * vec3(1.0);
    vec3 diffuse  = d * attenuation * vec3(0.75);
    vec3 specular = s * attenuation * vec3(0.85);

    vec3 finalColor = ambient + diffuse + specular;
    color = vec4(finalColor, 1.0);
}