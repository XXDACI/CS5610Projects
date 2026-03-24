/// Name: YiChong Chen
/// Date: 22 Mar 2026
/// In this project, I will implement simple normal mapping and displacement mapping. 
/// This project will include using both tessellation shaders and geometry shaders.

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "cyMatrix.h"
#include "cyTriMesh.h"
#include "cyGL.h"
#include "lodepng.h"

cy::GLSLProgram planeProg;
cy::GLSLProgram edgeProg;
cy::GLSLProgram dispProg;
cy::GLSLProgram dispEdgeProg;

// edge display
bool showTriangulation = true;

// camera rotation
float camYaw = 0.0f;
float camPitch = 0.0f;
float camRadius = 3.0f;

// mouse movement and clicker variable
bool leftDown = false;
bool rightDown = false;
bool crtlDown = false;

int lastX = 0;
int lastY = 0;

// light new postion after rotating
int lightRotX = 0;
int lightRotY = 0;

int windowWidth = 1920;
int windowHeight = 1080;

// VAOs
GLuint normalTex = 0;
GLuint planeVAO = 0;
GLuint planeVBO = 0;
GLuint patchVAO = 0;
GLuint patchVBO = 0;

// textures
GLuint dispTex = 0;

// displacement control
bool useDisplacement = false;
float tessLevel = 16.0f;
float dispScale = 0.35f;

void LoadNormalMap(const char* filename)
{
    std::vector<unsigned char> img;
    unsigned w = 0, h = 0;

    unsigned err = lodepng::decode(img, w, h, filename);
    if (err != 0)
    {
        std::cerr << "Normal map load failed: " << filename
            << " err=" << err << " "
            << lodepng_error_text(err) << "\n";
        exit(1);
    }

    glGenTextures(1, &normalTex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, normalTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, img.data());

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    planeProg.Bind();
    planeProg["normalTex"] = 0;
}

void LoadDisplacementMap(const char* filename)
{
    std::vector<unsigned char> img;
    unsigned w = 0, h = 0;

    unsigned err = lodepng::decode(img, w, h, filename);
    if (err != 0)
    {
        std::cerr << "Displacement map load failed: " << filename
            << " err=" << err << " "
            << lodepng_error_text(err) << "\n";
        exit(1);
    }

    glGenTextures(1, &dispTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, dispTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, img.data());

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

// handle buffer swither and other display stuff
void Display()
{
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (!useDisplacement)
    {
        planeProg.Bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, normalTex);

        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (showTriangulation)
        {
            edgeProg.Bind();
            glBindVertexArray(planeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
    else
    {
        glPatchParameteri(GL_PATCH_VERTICES, 4);

        dispProg.Bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, normalTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dispTex);

        glBindVertexArray(patchVAO);
        glDrawArrays(GL_PATCHES, 0, 4);

        if (showTriangulation)
        {
            dispEdgeProg.Bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, normalTex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, dispTex);

            glBindVertexArray(patchVAO);
            glDrawArrays(GL_PATCHES, 0, 4);
        }
    }

    glBindVertexArray(0);
    glutSwapBuffers();
}

// redraw the frame
void Reshape(int w, int h)
{
    glViewport(0, 0, 1920, 1080);

    glutPostRedisplay();
}

/// <summary>
/// handle animation stuff
/// </summary>
void myIdle()
{
    float FOV_Deg = 40.0f;
    float FOV_Rad = FOV_Deg * (3.1415926f / 180.0f);

    cy::Matrix4f projMatrix = cy::Matrix4f::Perspective(
        FOV_Rad,
        float(windowWidth) / float(windowHeight),
        0.1f,
        1000.0f
    );

    cy::Matrix4f modelMatrix = cy::Matrix4f::Identity();

    float yawRad = camYaw * (3.1415926f / 180.0f);
    float pitchRad = camPitch * (3.1415926f / 180.0f);

    cy::Vec3f target(0.0f, 0.0f, 0.0f);
    cy::Vec3f up(0.0f, 1.0f, 0.0f);

    cy::Vec3f cameraPos;
    cameraPos.x = camRadius * cosf(pitchRad) * sinf(yawRad);
    cameraPos.y = camRadius * sinf(pitchRad);
    cameraPos.z = camRadius * cosf(pitchRad) * cosf(yawRad);

    cy::Matrix4f viewMatrix = cy::Matrix4f::View(cameraPos, target, up);

    planeProg.Bind();
    planeProg["mvp"] = projMatrix * viewMatrix * modelMatrix;
    planeProg["model"] = modelMatrix;
    planeProg["cameraPos"] = cameraPos;

    float lrx = lightRotX * (3.1415926f / 180.0f);
    float lry = lightRotY * (3.1415926f / 180.0f);
    cy::Matrix3f lightRot = cy::Matrix3f::RotationXYZ(lry, lrx, 0);
    cy::Vec3f defaultLightDir(1.0f, 1.0f, 1.0f);
    planeProg["lightPos"] = lightRot * defaultLightDir;

    edgeProg.Bind();
    edgeProg["mvp"] = projMatrix * viewMatrix * modelMatrix;

    dispProg.Bind();
    dispProg["mvp"] = projMatrix * viewMatrix * modelMatrix;
    dispProg["model"] = modelMatrix;
    dispProg["cameraPos"] = cameraPos;
    dispProg["lightPos"] = lightRot * defaultLightDir;
    dispProg["tessLevel"] = tessLevel;
    dispProg["dispScale"] = dispScale;
    dispProg["normalTex"] = 0;
    dispProg["dispTex"] = 1;

    dispEdgeProg.Bind();
    dispEdgeProg["mvp"] = projMatrix * viewMatrix * modelMatrix;
    dispEdgeProg["model"] = modelMatrix;
    dispEdgeProg["tessLevel"] = tessLevel;
    dispEdgeProg["dispScale"] = dispScale;
    dispEdgeProg["dispTex"] = 1;
    dispEdgeProg["edgeBias"] = 0.001f;

    glutPostRedisplay();
}

/// <summary>
/// Handle Keybaord
/// ESC - Exit the program or close the window
/// A - Rerendering the scene
/// </summary>
/// <param name="key"> </param>
/// <param name="x"> </param>
/// <param name="y"> </param>
void Keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
    {
        glutLeaveMainLoop();
        return;
    }

    if (key == ' ')
    {
        showTriangulation = !showTriangulation;
        glutPostRedisplay();
        return;
    }
}

/// <summary>
/// Changing tessellation level
/// left/right arrow keys to control the tessellation level
/// </summary>
/// <param name="key"> </param>
/// <param name="x"> </param>
/// <param name="y"> </param>
void SpecialKey(int key, int x, int y)
{
    if (key == GLUT_KEY_RIGHT)
    {
        tessLevel += 8.0f;
        if (tessLevel > 64.0f) tessLevel = 64.0f;
        std::cout << "tessLevel = " << tessLevel << std::endl;
    }

    if (key == GLUT_KEY_LEFT)
    {
        tessLevel -= 8.0f;
        if (tessLevel < 1.0f) tessLevel = 1.0f;
        std::cout << "tessLevel = " << tessLevel << std::endl;
    }

    glutPostRedisplay();
}

/// <summary>
/// To check which mouse button click. 
/// Mark and know system know 
/// </summary>
/// <param name="button"></param>
/// <param name="state"></param>
/// <param name="x"></param>
/// <param name="y"></param>
void MouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
        leftDown = (state == GLUT_DOWN);

    if (button == GLUT_RIGHT_BUTTON)
        rightDown = (state == GLUT_DOWN);

    lastX = x;
    lastY = y;
}

/// <summary>
/// tracking the mouse movement shuch as if it go up or down, left or right
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
void MouseMotion(int x, int y)
{
    int dx = x - lastX;
    int dy = y - lastY;

    int mods = glutGetModifiers();

    if (leftDown)
    {
        if ((mods & GLUT_ACTIVE_CTRL) != 0)
        {
            lightRotX += dx;
            lightRotY += dy;
        }
        else
        {
            camYaw -= dx * 0.5f;
            camPitch += dy * 0.5f;

            if (camPitch > 89.0f) camPitch = 89.0f;
            if (camPitch < -89.0f) camPitch = -89.0f;
        }
    }

    if (rightDown)
    {
        camRadius += dy * 0.02f;

        if (camRadius < 1.0f) camRadius = 1.0f;
        if (camRadius > 20.0f) camRadius = 20.0f;
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
    glutCreateWindow("Project8_Tessellation");

    glewInit();

    // build normal-mapping shader
    planeProg.BuildFiles("plane_shader.vert", "plane_shader.frag");
    planeProg.Bind();
    planeProg["normalTex"] = 0;

    // build triangulation edge shader
    edgeProg.BuildFiles("edge_shader.vert", "edge_shader.frag", "edge_shader.geom");
    edgeProg.Bind();
    edgeProg["edgeBias"] = 0.01f;

    // build displacement shader
    dispProg.BuildFiles("disp_shader.vert",
        "disp_shader.frag",
        nullptr,
        "disp_shader.tcs",
        "disp_shader.tes");

    // build displacement edge shader
    dispEdgeProg.BuildFiles("disp_edge_shader.vert",
        "disp_edge_shader.frag",
        "disp_edge_shader.geom",
        "disp_edge_shader.tcs",
        "disp_edge_shader.tes");


    // load normal map and displacement map
    const char* normalFile = (argc >= 2) ? argv[1] : "teapot_normal.png";
    LoadNormalMap(normalFile);

    if (argc >= 3)
    {
        useDisplacement = true;
        const char* displacementFile = argv[2];
        LoadDisplacementMap(displacementFile);
    }
    else
    {
        useDisplacement = false;
    }

    float planeVerts[] = {
        -1, -1, 0, 0, 0,
         1, -1, 0, 1, 0,
         1,  1, 0, 1, 1,
        -1, -1, 0, 0, 0,
         1,  1, 0, 1, 1,
        -1,  1, 0, 0, 1,
    };

    float patchVerts[] = {
        -1, -1,  0,  0, 0,
         1, -1,  0,  1, 0,
         1,  1,  0,  1, 1,
        -1,  1,  0,  0, 1
    };

    //////////////////////////////////////////////////////////////////////////////////

    glGenVertexArrays(1, &planeVAO);
    glBindVertexArray(planeVAO);

    glGenBuffers(1, &planeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVerts), planeVerts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    //////////////////////////////////////////////////////////////////////////////////

    glGenVertexArrays(1, &patchVAO);
    glBindVertexArray(patchVAO);

    glGenBuffers(1, &patchVBO);
    glBindBuffer(GL_ARRAY_BUFFER, patchVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(patchVerts), patchVerts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    //////////////////////////////////////////////////////////////////////////////////

    glBindVertexArray(0);

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

