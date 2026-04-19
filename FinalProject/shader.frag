#version 330 core

in vec3 vWorldPos;
out vec4 FragColor;

uniform vec3  baseColor;
uniform float emissiveStrength;
uniform float tileScale;
uniform int   test;

uniform vec3 diffuseColor;
uniform vec3 specularColor;

uniform vec3  cameraPos;
uniform vec3  lightCenter;
uniform float lightWidth;
uniform float lightHeight;
uniform float lightIntensity;
uniform float roughness;
uniform bool  twoSided;

uniform sampler2D ltc_mat;
uniform sampler2D ltc_mag;

const float LUT_SIZE  = 64.0;
const float LUT_SCALE = (LUT_SIZE - 1.0) / LUT_SIZE;
const float LUT_BIAS  = 0.5 / LUT_SIZE;
const float pi = 3.14159265;
const float gamma = 2.2;

vec3 PowVec3(vec3 v, float p)
{
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

vec3 ToLinear(vec3 v) { return PowVec3(v, gamma); }
vec3 ToSRGB(vec3 v)   { return PowVec3(v, 1.0 / gamma); }

vec3 mul(mat3 m, vec3 v) { return m * v; }
mat3 mul(mat3 a, mat3 b) { return a * b; }

mat3 transpose3(mat3 v)
{
    mat3 tmp;
    tmp[0] = vec3(v[0].x, v[1].x, v[2].x);
    tmp[1] = vec3(v[0].y, v[1].y, v[2].y);
    tmp[2] = vec3(v[0].z, v[1].z, v[2].z);
    return tmp;
}

float IntegrateEdge(vec3 v1, vec3 v2)
{
    float cosTheta = clamp(dot(v1, v2), -1.0, 1.0);
    float theta = acos(cosTheta);
    float s = sin(theta);
    return cross(v1, v2).z * ((theta > 0.001 && abs(s) > 0.001) ? theta / s : 1.0);
}

void ClipQuadToHorizon(inout vec3 L[5], out int n)
{
    int config = 0;
    if (L[0].z > 0.0) config += 1;
    if (L[1].z > 0.0) config += 2;
    if (L[2].z > 0.0) config += 4;
    if (L[3].z > 0.0) config += 8;

    n = 0;

    if (config == 0)
    {
    }
    else if (config == 1)
    {
        n = 3;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 2)
    {
        n = 3;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 3)
    {
        n = 4;
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
        L[3] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 4)
    {
        n = 3;
        L[0] = -L[3].z * L[2] + L[2].z * L[3];
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
    }
    else if (config == 5)
    {
        n = 0;
    }
    else if (config == 6)
    {
        n = 4;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 7)
    {
        n = 5;
        L[4] = -L[3].z * L[0] + L[0].z * L[3];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 8)
    {
        n = 3;
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
        L[1] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = L[3];
    }
    else if (config == 9)
    {
        n = 4;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[2].z * L[3] + L[3].z * L[2];
    }
    else if (config == 10)
    {
        n = 0;
    }
    else if (config == 11)
    {
        n = 5;
        L[4] = L[3];
        L[3] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 12)
    {
        n = 4;
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
    }
    else if (config == 13)
    {
        n = 5;
        L[4] = L[3];
        L[3] = L[2];
        L[2] = -L[1].z * L[2] + L[2].z * L[1];
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
    }
    else if (config == 14)
    {
        n = 5;
        L[4] = -L[0].z * L[3] + L[3].z * L[0];
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
    }
    else if (config == 15)
    {
        n = 4;
    }

    if (n == 3) L[3] = L[0];
    if (n == 4) L[4] = L[0];
}

vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4], bool twoSidedLight)
{
    vec3 T1 = V - N * dot(V, N);
    float lenT1 = length(T1);
    if (lenT1 < 0.00001)
        T1 = vec3(1.0, 0.0, 0.0);
    else
        T1 /= lenT1;

    vec3 T2 = cross(N, T1);

    Minv = mul(Minv, transpose3(mat3(T1, T2, N)));

    vec3 L[5];
    L[0] = mul(Minv, points[0] - P);
    L[1] = mul(Minv, points[1] - P);
    L[2] = mul(Minv, points[2] - P);
    L[3] = mul(Minv, points[3] - P);

    int n;
    ClipQuadToHorizon(L, n);

    if (n == 0) return vec3(0.0);

    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);
    L[4] = normalize(L[4]);

    float sum = 0.0;
    sum += IntegrateEdge(L[0], L[1]);
    sum += IntegrateEdge(L[1], L[2]);
    sum += IntegrateEdge(L[2], L[3]);
    if (n >= 4) sum += IntegrateEdge(L[3], L[4]);
    if (n == 5) sum += IntegrateEdge(L[4], L[0]);

    sum = twoSidedLight ? abs(sum) : max(0.0, sum);
    return vec3(sum);
}

void InitRectPoints(out vec3 points[4])
{
    float hw = 0.5 * lightWidth;
    float hh = 0.5 * lightHeight;

    points[0] = lightCenter + vec3(-hw, -hh, 0.0);
    points[1] = lightCenter + vec3(-hw,  hh, 0.0);
    points[2] = lightCenter + vec3( hw,  hh, 0.0);
    points[3] = lightCenter + vec3( hw, -hh, 0.0);
}

void main()
{
    if (test == 1)
    {
        vec3 col = vec3(lightIntensity);
        col = col / (1.0 + col);
        FragColor = vec4(ToSRGB(col), 1.0);
        return;
    }

    vec3 P = vWorldPos;
    vec3 N = vec3(0.0, 1.0, 0.0);
    vec3 V = normalize(cameraPos - P);

    float ndv = clamp(dot(N, V), 0.0, 1.0);
    float theta = acos(ndv);

    vec2 uv = vec2(roughness, theta / (0.5 * pi));
    uv = uv * LUT_SCALE + LUT_BIAS;

    vec4 t = texture(ltc_mat, uv);

    mat3 Minv = mat3(
        vec3(1.0, 0.0, t.y),
        vec3(0.0, t.z, 0.0),
        vec3(t.w, 0.0, t.x)
    );

    vec3 points[4];
    InitRectPoints(points);

    vec3 spec = LTC_Evaluate(N, V, P, Minv, points, twoSided);
    spec *= texture(ltc_mag, uv).x;

    vec3 diff = LTC_Evaluate(N, V, P, mat3(1.0), points, twoSided);

    vec3 lcol = vec3(lightIntensity);
    vec3 dcol = ToLinear(diffuseColor);
    vec3 scol = ToLinear(specularColor);

    float specBoost = 1.0;
    float diffBoost = 1.2;

    vec3 softGlow = dcol * pow(max(diff.x, 0.0), 0.80) * 0.4;

    vec3 col = lcol * (scol * specBoost * spec + dcol * diffBoost * diff + softGlow);
    col /= (2.0 * pi);

    col = col / (1.0 + col);

    FragColor = vec4(ToSRGB(col), 1.0);
}