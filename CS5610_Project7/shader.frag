#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2DShadow shadow;

in vec3 normDir;
in vec4 lightpos;

void main()
{
    vec3 lightColor = vec3(1,1,1);
    vec3 meshColor= vec3(0.745f,0,0);

    float maxLightIntensity = 1.0f;

    float ambientLightIntensity = 0.2f;

    vec3 viewDir = vec3(1, 1, 1);

    vec3 lightDir = normalize(vec3(1, 1, 1));

    vec3 n = normalize(normDir);
    vec3 h = normalize(lightDir + viewDir);

    float nDoth = max(dot(n,h),0);
    float nDotl = max(dot(n, lightDir), 0);

    vec3 diffuse  = nDotl * meshColor * lightColor;
    vec3 specular = lightColor * pow(nDoth, 10);
    vec3 ambient = ambientLightIntensity * meshColor;

    float shadowFactor = textureProj(shadow, lightpos);
    vec3 lit = maxLightIntensity * (diffuse + specular) + ambient;

    color = vec4(lit * shadowFactor, 1.0);
}