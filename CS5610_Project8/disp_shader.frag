#version 410 core
layout(location = 0) out vec4 color;

uniform sampler2D normalTex;
uniform vec3 cameraPos;
uniform vec3 lightPos;

in vec2 fUV;
in vec3 fWorldPos;
in mat3 fTBN;

void main()
{
    vec2 uv = fUV;

    vec3 tangentNormal = texture(normalTex, uv).rgb;
    tangentNormal = tangentNormal * 2.0 - 1.0;

    vec3 n = normalize(fTBN * tangentNormal);

    vec3 lightVec = lightPos - fWorldPos;
    vec3 l = normalize(lightVec);
    vec3 v = normalize(cameraPos - fWorldPos);
    vec3 h = normalize(l + v);

    float dist = length(lightVec);
    float attenuation = 1.0 / (1.0 + 0.6 * dist + 0.25 * dist * dist);

    float ambientStrength = 0.03;
    float diffuseStrength = max(dot(n, l), 0.0);
    float specularStrength = pow(max(dot(n, h), 0.0), 32.0);

    vec3 ambient  = ambientStrength * vec3(1.0);
    vec3 diffuse  = diffuseStrength * attenuation * vec3(0.75);
    vec3 specular = specularStrength * attenuation * vec3(0.85);

    color = vec4(ambient + diffuse + specular, 1.0);
}