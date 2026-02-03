#version 330 core

layout(location = 0) out vec4 color;

uniform vec3 cameraPos;
uniform vec3 lightDir;

in vec3 worldPos;
in vec3 normDir;

void main()
{
    vec3 lightColor = vec3(1,1,1);
    vec3 meshColor= vec3(0.745f,0,0);

    float maxLightIntensity = 1.0f;

    float ambientLightIntensity = 0.2f;

    vec3 n = normalize(normDir);
    vec3 viewDir = normalize(cameraPos - worldPos);

    vec3 h = normalize(lightDir + viewDir);

    float nDoth = max(dot(n,h),0);
    float nDotl = max(dot(n, lightDir), 0);

    vec3 diffuse  = nDotl * meshColor * lightColor;

    vec3 specular = lightColor * pow(nDoth, 10);

    vec3 ambient = ambientLightIntensity * meshColor;

    color = vec4(maxLightIntensity * (diffuse + specular) + ambient, 1);
}