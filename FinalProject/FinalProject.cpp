#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <string>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include "cyMatrix.h"
#include "cyGL.h"

cy::GLSLProgram prog;

// camera
float camYaw = 0.0f;
float camPitch = 15.0f;
float camRadius = 20.0f;

// mouse
bool leftDown = false;
bool rightDown = false;
int lastX = 0;
int lastY = 0;

// window
int windowWidth = 1920;
int windowHeight = 1080;

// scene controls
float lightIntensity = 2.0f;
float lightWidth = 2.5f;
float lightHeight = 2.5f;
float roughness = 0.25f;

// geometry
GLuint floorVAO = 0;
GLuint floorVBO = 0;
GLuint lightVAO = 0;
GLuint lightVBO = 0;

// LUT textures
GLuint ltcMatTex = 0;
GLuint ltcAmpTex = 0;

// matrices
cy::Matrix4f gProj;
cy::Matrix4f gView;
cy::Vec3f    gCameraPos;

// scene placement
cy::Vec3f sceneTarget(0.0f, 1.0f, 0.0f);
cy::Vec3f lightCenter(0.0f, 2.5f, -2.5f);

// color and the seting for color animation
struct ColorAnim
{
    float r, g, b;
    bool br, bg, bb;
    bool playing;
};

ColorAnim diffuseAnim = { 1.0f, 1.0f, 1.0f, false, true, false, false };
ColorAnim specularAnim = { 1.0f, 1.0f, 1.0f, true, false, true, false };

float ClampFloat(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void StepChannel(float& c, bool& dir)
{
    if (dir)
    {
        c += 0.01f;
        if (c >= 1.0f)
        {
            c = 1.0f;
            dir = false;
        }
    }
    else
    {
        c -= 0.01f;
        if (c <= 0.0f)
        {
            c = 0.0f;
            dir = true;
        }
    }
}

void UpdateColorAnim(ColorAnim& a)
{
    if (!a.playing) return;

    StepChannel(a.r, a.br);
    StepChannel(a.g, a.bg);
    StepChannel(a.b, a.bb);
}

#pragma pack(push, 1)
struct DDS_PIXELFORMAT
{
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
};

struct DDS_HEADER
{
    uint32_t        dwSize;
    uint32_t        dwFlags;
    uint32_t        dwHeight;
    uint32_t        dwWidth;
    uint32_t        dwPitchOrLinearSize;
    uint32_t        dwDepth;
    uint32_t        dwMipMapCount;
    uint32_t        dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t        dwCaps;
    uint32_t        dwCaps2;
    uint32_t        dwCaps3;
    uint32_t        dwCaps4;
    uint32_t        dwReserved2;
};

struct DDS_HEADER_DXT10
{
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};
#pragma pack(pop)

GLuint LoadDDSTextureDX10(const char* filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cout << "Failed to open DDS: " << filename << std::endl;
        return 0;
    }

    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t));
    if (magic != 0x20534444)
    {
        std::cout << "Not a DDS file: " << filename << std::endl;
        return 0;
    }

    DDS_HEADER hdr{};
    file.read(reinterpret_cast<char*>(&hdr), sizeof(DDS_HEADER));

    DDS_HEADER_DXT10 dx10{};
    file.read(reinterpret_cast<char*>(&dx10), sizeof(DDS_HEADER_DXT10));

    const uint32_t width = hdr.dwWidth;
    const uint32_t height = hdr.dwHeight;

    GLenum internalFormat = 0;
    GLenum uploadFormat = 0;
    size_t bytesPerTexel = 0;

    if (dx10.dxgiFormat == 2)
    {
        internalFormat = GL_RGBA32F;
        uploadFormat = GL_RGBA;
        bytesPerTexel = 16;
    }
    else if (dx10.dxgiFormat == 16)
    {
        internalFormat = GL_RG32F;
        uploadFormat = GL_RG;
        bytesPerTexel = 8;
    }
    else
    {
        std::cout << "Unsupported DDS DXGI format in " << filename
            << ": " << dx10.dxgiFormat << std::endl;
        return 0;
    }

    std::vector<unsigned char> data(width * height * bytesPerTexel);
    file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));

    if (!file)
    {
        std::cout << "Failed reading DDS pixel data: " << filename << std::endl;
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        uploadFormat,
        GL_FLOAT,
        data.data()
    );

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Loaded DDS: " << filename
        << " (" << width << "x" << height
        << ", dxgi=" << dx10.dxgiFormat << ")" << std::endl;

    return tex;
}

void BuildFloorGeometry()
{
    float floorVerts[] =
    {
        -50.0f, 0.0f, -50.0f, 0.0f, 0.0f,
         50.0f, 0.0f, -50.0f, 1.0f, 0.0f,
         50.0f, 0.0f,  50.0f, 1.0f, 1.0f,

        -50.0f, 0.0f, -50.0f, 0.0f, 0.0f,
         50.0f, 0.0f,  50.0f, 1.0f, 1.0f,
        -50.0f, 0.0f,  50.0f, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &floorVAO);
    glBindVertexArray(floorVAO);

    glGenBuffers(1, &floorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVerts), floorVerts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void UpdateLightGeometry()
{
    float hw = 0.5f * lightWidth;
    float hh = 0.5f * lightHeight;

    float x0 = lightCenter.x - hw;
    float x1 = lightCenter.x + hw;
    float y0 = lightCenter.y - hh;
    float y1 = lightCenter.y + hh;
    float z = lightCenter.z;

    float lightVerts[] =
    {
        x0, y0, z, 0.0f, 0.0f,
        x1, y0, z, 1.0f, 0.0f,
        x1, y1, z, 1.0f, 1.0f,

        x0, y0, z, 0.0f, 0.0f,
        x1, y1, z, 1.0f, 1.0f,
        x0, y1, z, 0.0f, 1.0f
    };

    if (lightVAO == 0)
    {
        glGenVertexArrays(1, &lightVAO);
        glBindVertexArray(lightVAO);

        glGenBuffers(1, &lightVBO);
        glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lightVerts), lightVerts, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lightVerts), lightVerts);
    }
}

void DrawString(float x, float y, const std::string& text)
{
    glRasterPos2f(x, y);
    for (char c : text)
    {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, c);
    }
}

void DrawOverlay()
{
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // background panel
    glColor4f(0.08f, 0.08f, 0.08f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(15.0f, windowHeight - 15.0f);
    glVertex2f(430.0f, windowHeight - 15.0f);
    glVertex2f(430.0f, windowHeight - 285.0f);
    glVertex2f(15.0f, windowHeight - 285.0f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);

    char buffer[256];
    float x = 28.0f;
    float y = windowHeight - 38.0f;
    float line = 22.0f;

    DrawString(x, y, "Area Light (LTC)");
    y -= line;

    std::snprintf(buffer, sizeof(buffer), "Intensity (PageDown / PageUp): %.2f", lightIntensity);
    DrawString(x, y, buffer);
    y -= line;

    std::snprintf(buffer, sizeof(buffer), "Width     (Left / Right): %.2f", lightWidth);
    DrawString(x, y, buffer);
    y -= line;

    std::snprintf(buffer, sizeof(buffer), "Height    (Up / Down): %.2f", lightHeight);
    DrawString(x, y, buffer);
    y -= line;

    std::snprintf(buffer, sizeof(buffer), "Roughness ([ / ]): %.2f", roughness);
    DrawString(x, y, buffer);
    y -= line;

    y -= 8.0f;

    std::snprintf(buffer, sizeof(buffer), "Cam Yaw   (Left Mouse Drag): %.2f", camYaw);
    DrawString(x, y, buffer);
    y -= line;

    std::snprintf(buffer, sizeof(buffer), "Cam Pitch (Left Mouse Drag): %.2f", camPitch);
    DrawString(x, y, buffer);
    y -= line;

    std::snprintf(buffer, sizeof(buffer), "Cam Depth (Right Mouse Drag): %.2f", camRadius);
    DrawString(x, y, buffer);
    y -= line;

    y -= 8.0f;

    std::snprintf(buffer, sizeof(buffer), "Diffuse Anim (1): %s", diffuseAnim.playing ? "ON" : "OFF");
    DrawString(x, y, buffer);
    y -= line;

    std::snprintf(buffer, sizeof(buffer), "Specular Anim (2): %s", specularAnim.playing ? "ON" : "OFF");
    DrawString(x, y, buffer);
    y -= line;

    y -= 6.0f;

    DrawString(x, y, "Reset (r)");

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void Display()
{
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    prog.Bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ltcMatTex);
    prog["ltc_mat"] = 0;

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ltcAmpTex);
    prog["ltc_mag"] = 1;

    prog["cameraPos"] = gCameraPos;
    prog["lightCenter"] = lightCenter;
    prog["lightWidth"] = lightWidth;
    prog["lightHeight"] = lightHeight;
    prog["lightIntensity"] = lightIntensity;
    prog["roughness"] = roughness;
    prog["twoSided"] = 0;

    // floor
    prog["baseColor"] = cy::Vec3f(0.10f, 0.10f, 0.10f);
    prog["emissiveStrength"] = 0.0f;
    prog["tileScale"] = 1.0f;
    prog["test"] = 0;

    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // light panel
    prog["baseColor"] = cy::Vec3f(1.0f, 1.0f, 1.0f);
    prog["emissiveStrength"] = lightIntensity;
    prog["tileScale"] = 1.0f;
    prog["test"] = 1;

    prog["diffuseColor"] = cy::Vec3f(diffuseAnim.r, diffuseAnim.g, diffuseAnim.b);
    prog["specularColor"] = cy::Vec3f(specularAnim.r, specularAnim.g, specularAnim.b);

    glBindVertexArray(lightVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    DrawOverlay();
    glutSwapBuffers();
}

void Reshape(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, windowWidth, windowHeight);
    glutPostRedisplay();
}

void myIdle()
{
    float FOV_Deg = 40.0f;
    float FOV_Rad = FOV_Deg * (3.1415926f / 180.0f);

    gProj = cy::Matrix4f::Perspective(
        FOV_Rad,
        float(windowWidth) / float(windowHeight),
        0.1f,
        1000.0f
    );

    float yawRad = camYaw * (3.1415926f / 180.0f);
    float pitchRad = camPitch * (3.1415926f / 180.0f);

    cy::Vec3f up(0.0f, 1.0f, 0.0f);

    gCameraPos.x = sceneTarget.x + camRadius * cosf(pitchRad) * sinf(yawRad);
    gCameraPos.y = sceneTarget.y + camRadius * sinf(pitchRad);
    gCameraPos.z = sceneTarget.z + camRadius * cosf(pitchRad) * cosf(yawRad);

    gView = cy::Matrix4f::View(gCameraPos, sceneTarget, up);

    cy::Matrix4f identity = cy::Matrix4f::Identity();
    cy::Matrix4f mvp = gProj * gView * identity;

    prog.Bind();
    prog["mvp"] = mvp;

    UpdateColorAnim(diffuseAnim);
    UpdateColorAnim(specularAnim);

    glutPostRedisplay();
}

void Keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
    {
        glutLeaveMainLoop();
        return;
    }

    if (key == '1')
    {
        if (diffuseAnim.r == 1.0f && diffuseAnim.g == 1.0f && diffuseAnim.b == 1.0f)
            diffuseAnim = { 0.75f, 0.20f, 0.20f, true,  false, true,  false };
        diffuseAnim.playing = !diffuseAnim.playing;
        glutPostRedisplay();
        return;
    }

    if (key == '2')
    {
        if (specularAnim.r == 1.0f && specularAnim.g == 1.0f && specularAnim.b == 1.0f)
            specularAnim = { 0.20f, 0.80f, 0.80f, false, true,  false, false };
        specularAnim.playing = !specularAnim.playing;
        glutPostRedisplay();
        return;
    }

    if (key == 'r' || key == 'R')
    {
        camYaw = 0.0f;
        camPitch = 15.0f;
        camRadius = 20.0f;

        lightIntensity = 2.0f;
        lightWidth = 2.5f;
        lightHeight = 2.5f;
        roughness = 0.25f;
        UpdateLightGeometry();

        diffuseAnim = { 1.0f, 1.0f, 1.0f, false, true, false, false };
        specularAnim = { 1.0f, 1.0f, 1.0f, true, false, true, false };

        glutPostRedisplay();
        return;
    }

    if (key == '[')
    {
        roughness -= 0.02f;
        roughness = ClampFloat(roughness, 0.02f, 1.0f);
        glutPostRedisplay();
        return;
    }

    if (key == ']')
    {
        roughness += 0.02f;
        roughness = ClampFloat(roughness, 0.02f, 1.0f);
        glutPostRedisplay();
        return;
    }
}

void SpecialKey(int key, int x, int y)
{
    if (key == GLUT_KEY_LEFT)
    {
        lightWidth -= 0.1f;
        lightWidth = ClampFloat(lightWidth, 0.2f, 10.0f);
        UpdateLightGeometry();
    }

    if (key == GLUT_KEY_RIGHT)
    {
        lightWidth += 0.1f;
        lightWidth = ClampFloat(lightWidth, 0.2f, 10.0f);
        UpdateLightGeometry();
    }

    if (key == GLUT_KEY_DOWN)
    {
        lightHeight -= 0.1f;
        lightHeight = ClampFloat(lightHeight, 0.2f, 10.0f);
        UpdateLightGeometry();
    }

    if (key == GLUT_KEY_UP)
    {
        lightHeight += 0.1f;
        lightHeight = ClampFloat(lightHeight, 0.2f, 10.0f);
        UpdateLightGeometry();
    }

    if (key == GLUT_KEY_PAGE_DOWN)
    {
        lightIntensity -= 0.04f;
        lightIntensity = ClampFloat(lightIntensity, 0.0f, 6.0f);
    }

    if (key == GLUT_KEY_PAGE_UP)
    {
        lightIntensity += 0.04f;
        lightIntensity = ClampFloat(lightIntensity, 0.0f, 6.0f);
    }

    glutPostRedisplay();
}

void MouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
        leftDown = (state == GLUT_DOWN);

    if (button == GLUT_RIGHT_BUTTON)
        rightDown = (state == GLUT_DOWN);

    lastX = x;
    lastY = y;
}

void MouseMotion(int x, int y)
{
    int dx = x - lastX;
    int dy = y - lastY;

    if (leftDown)
    {
        camYaw -= dx * 0.5f;
        camPitch += dy * 0.5f;

        if (camPitch > 89.0f)
            camPitch = 89.0f;

        if (camPitch < 0.0f)
            camPitch = 0.0f;
    }

    if (rightDown)
    {
        camRadius += dy * 0.03f;
        camRadius = ClampFloat(camRadius, 2.0f, 30.0f);
    }

    lastX = x;
    lastY = y;

    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutCreateWindow("FinalProject_AreaLight_LTC");

    glewInit();

    prog.BuildFiles("shader.vert", "shader.frag");

    ltcMatTex = LoadDDSTextureDX10("ltc_mat.dds");
    ltcAmpTex = LoadDDSTextureDX10("ltc_amp.dds");

    if (ltcMatTex == 0 || ltcAmpTex == 0)
    {
        std::cout << "Failed to load LTC DDS textures." << std::endl;
    }

    BuildFloorGeometry();
    UpdateLightGeometry();

    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseMotion);
    glutDisplayFunc(Display);
    glutIdleFunc(myIdle);
    glutKeyboardFunc(Keyboard);
    glutSpecialFunc(SpecialKey);
    glutReshapeFunc(Reshape);

    glutMainLoop();
    return 0;
}
