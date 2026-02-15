#version 330 core

layout(location = 0) out vec4 color;

uniform vec3 cameraPos;
uniform vec3 lightDir;
uniform sampler2D tex0;
uniform sampler2D tex1;

in vec3 worldPos;
in vec3 normDir;
in vec2 texCoord;

void main()
{
    vec3 lightColor = vec3(1,1,1);
    vec3 meshColor= vec3(1,1,1);

    float maxLightIntensity = 1.0f;

    float ambientLightIntensity = 0.1f;

    vec3 n = normalize(normDir);
    vec3 viewDir = normalize(cameraPos - worldPos);
    vec3 h = normalize(lightDir + viewDir);

    float nDoth = max(dot(n,h),0);
    float nDotl = max(dot(n, lightDir), 0);

    vec3 diffuse  = nDotl * meshColor * lightColor;
    vec3 specular = lightColor * pow(nDoth, 10);
    vec3 ambient = ambientLightIntensity * meshColor;

    vec3 dAndaTexture = texture(tex0, texCoord).rgb;
    vec3 sTexture = texture(tex1, texCoord).rgb;

    color = vec4(maxLightIntensity * (diffuse * dAndaTexture + specular * sTexture) + ambient * dAndaTexture, 1);
}