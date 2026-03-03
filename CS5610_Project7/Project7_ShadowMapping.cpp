/// Name: YiChong Chen
/// Date: 1 Mar 2026

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include "cyMatrix.h"
#include "cyTriMesh.h"
#include "cyGL.h"

cy::GLSLProgram prog;
cy::GLSLProgram shadowProg;

// the object 
cy::TriMesh mesh;
bool success = mesh.LoadFromFileObj("teapot.obj");

GLuint meshvao;

// total number of the vertex
int num_face = mesh.NV();

// transform vector which using in matrix
float modelRotX = 0;
float modelRotY = 0;
float modelPosZ = -100;

// center point
cy::Vec3f objCenter(0, 0, 0);

// mouse movement and clicker variable
bool leftDown = false;
bool rightDown = false;
bool crtlDown = false;

// previous rotating position
int lastX = 0;
int lastY = 0;

// camera rotation control
static float rotX = 0.0f;
static float rotY = 0.0f;
static float posZ = 50;
static float camDist = 6.0f;
static cy::Vec3f camera_target(0, 0, 0);

// light new postion after rotating
int lightRotX = 0;
int lightRotY = 0;

// plane data
static GLuint vaoPlane;
static GLuint vboPlane;
static int planeVerts = 6;

// shadow
GLuint vaoShadow;

cy::GLRenderDepth2D shadowMap;

// handle buffer swither and other display stuff
void Display()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    shadowMap.Initialize(true, 1000, 1000);
    shadowMap.SetTextureFilteringMode(GL_LINEAR, GL_LINEAR);
    shadowMap.Bind();
    shadowProg.Bind();

    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glBindVertexArray(meshvao);

    // draw the teapot
    glDrawArrays(GL_TRIANGLES, 0, mesh.NF() * 3);

    glBindVertexArray(vaoPlane);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    shadowMap.Unbind();
    shadowMap.BuildTextureMipmaps();
    shadowMap.BindTexture(0);
    prog["shadow"] = 0;
    prog.Bind();


    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glBindVertexArray(meshvao);
    prog.Bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);

    glBindVertexArray(meshvao);

    glDrawArrays(GL_TRIANGLES, 0, mesh.NF() * 3);

    glBindVertexArray(vaoPlane);
    glDrawArrays(GL_TRIANGLES, 0, 6);




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
    float FOV_Rad = FOV_Deg * (3.1415926 / 180.0);

    // calculate rotate angle
    float rx = modelRotX * (3.14159 / 180.0f);
    float ry = modelRotY * (3.14159 / 180.0f);

    float radius = posZ;
    cy::Matrix3f rot = cy::Matrix3f::RotationY(rotX) * cy::Matrix3f::RotationX(rotY);
    cy::Vec3f view = rot * cy::Vec3f(0, 0, radius);

    cy::Vec3f up = rot * cy::Vec3f(0, 1, 0);

    cy::Matrix4f view_matrix = cy::Matrix4f::View(view, cy::Vec3f(0, 0, 0), up);

    // calculate the distance which object move
    cy::Vec3f transVector = cy::Vec3f(0, 0, 0);

    // rotation matrix
    cy::Matrix3f rotMatrix = cy::Matrix3f::RotationXYZ(modelRotY, modelRotX, 0);
    // perspective matrix
    cy::Matrix4f projMatrix = cy::Matrix4f::Perspective(FOV_Rad, float(1920) / float(1080), 0.1, 1000.0f);
    // translation matrix
    cy::Matrix4f transMatrix = cy::Matrix4f::Translation(transVector);
    // center object matrix
    cy::Matrix4f centerMatrix = cy::Matrix4f::Translation(-objCenter);
    
    cy::Matrix4f meshModel = transMatrix * rotMatrix * centerMatrix;

    // calculate model-view-projection matrix and pass the mvp matrix to shader
    prog["mvp"] = projMatrix * view_matrix * meshModel;

    // calculate model's world coordinate and 
    prog["modelWorldPos"] = meshModel;

    // calculate the camera's position
    cy::Vec3f cameraPos(0, 0, -modelPosZ);
    prog["cameraPos"] = cameraPos;

    ////////////////////////////// MLP matrix //////////////////////////////////
    cy::Matrix3f lightRotMatrix = cy::Matrix3f::RotationXYZ(0, 0, 0);
    cy::Matrix4f lightProjMatrix = cy::Matrix4f::Perspective(FOV_Rad, float(960) / float(540), 0.1, 1000.0f);
    cy::Vec3f lightUp = cy::Vec3f(0, 1, 0);

    cy::Vec3f lightTransVector = cy::Vec3f(0, 0, 0);
    cy::Matrix4f lightTransMatrix = cy::Matrix4f::Translation(lightTransVector);

    cy::Matrix4f lightM = lightTransMatrix * lightRotMatrix;

    cy::Matrix4f lightView = cy::Matrix4f::View(cy::Vec3f(45, 45, 45), cy::Vec3f(0, 0, 0), lightUp);

    cy::Matrix4f lightmvp = lightProjMatrix * lightView * lightM;
    shadowProg["lightmvp"] = lightmvp;

    cy::Matrix4f scaleMatrix = cy::Matrix4f::Scale(0.5);

    float Bias = 0.00005f;
    cy::Matrix4f shadowTrans = cy::Matrix4f::Translation(cy::Vec3f(0.5, 0.5, 0.5 - Bias));

    prog["shadowMtx"] = (shadowTrans * scaleMatrix * lightmvp);

    //////////////////////////////////////////////////////////////////////////////


    //Tell GLUT to redraw
    glutPostRedisplay();
}

/// <summary>
/// center point calculation
/// </summary>
void center()
{
    // create the to vector store the furthest and closest point
    cy::Vec3f min = mesh.V(0);
    cy::Vec3f max = mesh.V(0);

    for (int i = 1; i < mesh.NV(); i++)
    {
        cy::Vec3f v = mesh.V(i);

        // campare the point in every single loop to find the extreme point
        if (min.x < v.x)
            min.x = v.x;
        if (min.y < v.y)
            min.y = v.y;
        if (min.z < v.z)
            min.z = v.z;
        if (max.x > v.x)
            max.x = v.x;
        if (max.y > v.y)
            max.y = v.y;
        if (max.z > v.z)
            max.z = v.z;

    }

    // calculate the center point of the object
    objCenter = (min + max) / 2;
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
    // ESC is key27 
    if (key == 27)
    {
        // Close the window
        glutLeaveMainLoop();
        return;
    }

    // use key a to rerendering 
    if (key == 97) {
        if (prog.BuildFiles("shader.vert", "shader.frag"))
        {
            prog.Bind();
        }
    }
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
    // calculating the movement of the rotation
    int dx = x - lastX;
    int dy = y - lastY;

    int mods = glutGetModifiers();

    if (leftDown)
    {
        //check ctrl is pressed down or not
        if ((mods & GLUT_ACTIVE_CTRL) != 0)
        {
            // light x-axis left and right
            lightRotX += dx;
            // light y-axis up and down
            lightRotY += dy;
        }
        else
        {
            // model x-axis left and right
            rotX += (-dx) * 0.005f;
            // model y-axis up and down
            rotY += (-dy) * 0.005f;
        }
    }

    if (rightDown)
    {
        // model z-axis far and close
        posZ += dy;
    }

    // recoding the last cordinate which help to track future movement
    lastX = x;
    lastY = y;

    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    // initialize glut
    glutInit(&argc, argv);

    // set window size
    glutInitWindowSize(1920, 1080);
    // set window position
    glutInitWindowPosition(100, 100);

    // set display mode
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

    // set window title
    glutCreateWindow("Project7_ShadowMapping");

    // initialize GLEW to load OpenGL function pointers
    GLenum err = glewInit();

    // center the model in the screen
    center();

    // compile vs and fs shader
    prog.BuildFiles("shader.vert", "shader.frag");

    // bind the buffer
    prog.Bind();

    // VAO
    
    glGenVertexArrays(1, &meshvao);
    glBindVertexArray(meshvao);

    // get all vertex data and put in an array
    std::vector<cy::Vec3f> vertsArray;

    for (int i = 0; i < mesh.NF(); i++)
    {
        auto vFace = mesh.F(i);
        vertsArray.push_back(mesh.V(vFace.v[0]));
        vertsArray.push_back(mesh.V(vFace.v[1]));
        vertsArray.push_back(mesh.V(vFace.v[2]));
    }

    // get all normal data and put in an array
    std::vector<cy::Vec3f> normalArray;

    for (int i = 0; i < mesh.NF(); i++)
    {
        auto nFace = mesh.FN(i);
        normalArray.push_back(mesh.VN(nFace.v[0]));
        normalArray.push_back(mesh.VN(nFace.v[1]));
        normalArray.push_back(mesh.VN(nFace.v[2]));
    }

    // VBO
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        vertsArray.size() * sizeof(cy::Vec3f),
        vertsArray.data(),
        GL_STATIC_DRAW);

    GLuint pos = prog.AttribLocation("pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(
        pos, 3, GL_FLOAT,
        GL_FALSE, 0, (GLvoid*)0);

    // normal buffer
    GLuint normalBuffer;
    glGenBuffers(1, &normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glBufferData(GL_ARRAY_BUFFER,
        normalArray.size() * sizeof(cy::Vec3f),
        normalArray.data(),
        GL_STATIC_DRAW);

    GLuint normal = prog.AttribLocation("normal");
    glEnableVertexAttribArray(normal);
    glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    //////////////////////////// Build Plane ////////////////////////////
    float planeVerts[] = {
        -30, 0, -30,  0, 0,
         30, 0,  30,  1, 1,
         30, 0, -30,  1, 0,
        -30, 0, -30,  0, 0,
        -30, 0,  30,  0, 1,
         30, 0,  30,  1, 1,
    };

    glGenVertexArrays(1, &vaoPlane);
    glBindVertexArray(vaoPlane);

    glGenBuffers(1, &vboPlane);
    glBindBuffer(GL_ARRAY_BUFFER, vboPlane);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVerts), planeVerts, GL_STATIC_DRAW);

    // pos @ location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, 
        GL_FALSE, 
        5 * sizeof(float), 
        (void*)0);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, 
        GL_FALSE, 
        5 * sizeof(float), 
        (void*)(3 * sizeof(float)));

    glDisableVertexAttribArray(1);
    glVertexAttrib3f(1, 0.0f, 1.0f, 0.0f);

    glBindVertexArray(0);
    ////////////////////////////////////////////////////

    //////////////////////// shadow ////////////////////////

    cy::Vec3f planeVP[] = {
        {-30, 0, -30 },
        { 30, 0,  30 },
        { 30, 0, -30 },
        {-30, 0, -30 },
        {-30, 0,  30 },
        { 30, 0,  30 },
    };

    shadowProg.BuildFiles("shadow.vert", "shadow.frag");
    shadowProg.Bind();

    glBindVertexArray(vaoShadow);

    GLuint shadowTeapotBuffer;
    glGenBuffers(1, &shadowTeapotBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, shadowTeapotBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * vertsArray.size(), vertsArray.data(), GL_STATIC_DRAW); //sizeof(cy::Vec3f) * teapot.NV() --- 1 size * num of v

    GLuint shadowTeapotPos = shadowProg.AttribLocation("pos");
    glEnableVertexAttribArray(shadowTeapotPos);
    glVertexAttribPointer(
        shadowTeapotPos, 3, 
        GL_FLOAT, GL_FALSE, 0, 
        (GLvoid*)0);

    glBindVertexArray(vaoPlane);

    GLuint shadowPlaneBuffer;
    glGenBuffers(1, &shadowPlaneBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, shadowPlaneBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * 6, planeVP, GL_STATIC_DRAW);

    GLuint shadowPosPlane = shadowProg.AttribLocation("pos");
    glEnableVertexAttribArray(shadowPosPlane);
    glVertexAttribPointer(
        shadowPosPlane, 3, 
        GL_FLOAT, GL_FALSE, 0, 
        (GLvoid*)0);

    ////////////////////////////////////////////////////

    // handle mouse click
    glutMouseFunc(MouseButton);
    // handle mouse movement
    glutMotionFunc(MouseMotion);
    // handle display
    glutDisplayFunc(Display);
    // handle animation
    glutIdleFunc(myIdle);
    // handle keyboard overwrite
    glutKeyboardFunc(Keyboard);
    // handle redrawing
    glutReshapeFunc(Reshape);

    // Start loop
    glutMainLoop();
    return 0;
}