#version 330 core

layout(location = 0) out vec4 color;

uniform vec3 cameraPos;
uniform vec3 lightDir;
uniform samplerCube env;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float alpha;
uniform float reflectStrength;

in vec3 worldPos;
in vec3 normDir;

void main()
{
    vec3 n = normalize(normDir);
    vec3 v = normalize(cameraPos - worldPos);
    vec3 l = normalize(lightDir);
    vec3 h = normalize(l + v);

    float ndotl = max(dot(n, l), 0.0);
    float ndoth = max(dot(n, h), 0.0);

    vec3 ambient = 0.08 * Kd;
    vec3 diffuse = Kd * ndotl;
    vec3 spec    = Ks * pow(ndoth, alpha);

    // reflection from cubemap
    vec3 r = reflect(-v, n);
    vec3 refl = texture(env, r).rgb;

    vec3 blinn = ambient + diffuse + spec;
    vec3 finalCol = blinn * (1.0 - reflectStrength) + refl * reflectStrength;

    color = vec4(finalCol, 1.0);
}