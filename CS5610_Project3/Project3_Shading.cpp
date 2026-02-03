/// Name: YiChong Chen
/// Date: 1 Feb 2026
/// This is a program can generate a Utah teapot in vertex version 
/// Allow to use left mouse bottom to zoom and right mouse bottom to zoom
/// Allow use key a to rerendering the scene

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include "cyMatrix.h"
#include "cyTriMesh.h"
#include "cyGL.h"

cy::GLSLProgram prog;

// the object 
cy::TriMesh mesh;
bool success = mesh.LoadFromFileObj("teapot.obj");
// total number of the vertex
int num_vertices = mesh.NV();

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

// light new postion after rotating
int lightRotX = 0;
int lightRotY = 0;

// handle buffer swither and other display stuff
void Display()
{
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // draw the teapot
    glDrawArrays(GL_TRIANGLES, 0, mesh.NF() * 3);
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

    // calculate the distance which object move
    cy::Vec3f transVector = cy::Vec3f(0, 0, modelPosZ);

    // rotation matrix
    cy::Matrix3f rotMatrix = cy::Matrix3f::RotationXYZ(modelRotY, modelRotX, 0);
    // perspective matrix
    cy::Matrix4f projMatrix = cy::Matrix4f::Perspective(FOV_Rad, float(1920) / float(1080), 0.1, 1000.0f);
    // translation matrix
    cy::Matrix4f transMatrix = cy::Matrix4f::Translation(transVector);
    // center object matrix
    cy::Matrix4f centerMatrix = cy::Matrix4f::Translation(-objCenter);

    // calculate model-view-projection matrix and pass the mvp matrix to shader
    prog["mvp"] = projMatrix * transMatrix * rotMatrix * centerMatrix;

    // calculate model's world coordinate and 
    prog["modelWorldPos"] = transMatrix * rotMatrix * centerMatrix;
    
    // calculate the camera's position
    cy::Vec3f cameraPos(0, 0, -modelPosZ);
    prog["cameraPos"] = cameraPos;

    // calculating the light's position after rotating on X
    float lrx = lightRotX * (3.1415926 / 180.0f);
    // calculating the light's position after rotating on y
    float lry = lightRotY * (3.1415926 / 180.0f);
    
    // calculate the light on model and pass the matrix to shader
    cy::Matrix3f lightRot = cy::Matrix3f::RotationXYZ(lry, lrx, 0);
    cy::Vec3f defaultLightDir(1, 1, 1);
    prog["lightDir"] = lightRot * defaultLightDir;

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
            modelRotX += dx * 0.02f;
            // model y-axis up and down
            modelRotY += dy * 0.02f;
        }
    }

    if (rightDown)
    {
        // model z-axis far and close
        modelPosZ += dy;
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
    glutCreateWindow("Project3_Shading");

    // initialize GLEW to load OpenGL function pointers
    GLenum err = glewInit();

    // center the model in the screen
    center();

    // compile vs and fs shader
    prog.BuildFiles("shader.vert", "shader.frag");

    // bind the buffer
    prog.Bind();

    // VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

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