/// Name: YiChong Chen
/// Date: 18 Jan 2025
/// This is a program can generate a window called Hello World 
/// Also has color switch animation

#include <GL/freeglut.h>

// initialize the helper method
void colorchange(float& color, bool& b_color);

// color variable 
// float are the rgb value 
static float colorR = 0.0f;
static float colorG = 1.0f;
static float colorB = 0.5f;
// bool are the value checker and reverse
static bool b_colorR = true;
static bool b_colorG = true;
static bool b_colorB = false;

// handle buffer swither and other display stuff
void Display()
{
    glClearColor(colorR, colorG, colorB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

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
    colorchange(colorR, b_colorR);
    colorchange(colorG, b_colorG);
    colorchange(colorB, b_colorB);
    //Tell GLUT to redraw
    glutPostRedisplay(); 
}

/// <summary>
/// making the color animation
/// changing color in different combination by adding or minusing the rgb value with 0.01
/// if the value reach to 0, make the flag to true and using the adding calculation
/// if the value reach to 1, make the flag to fale and using the minusing calculation
/// </summary>
/// <param name="color"> the pointer to the rgb value </param>
/// <param name="b_color"> the pointer to the rgb value checker </param>
void colorchange(float& color, bool& b_color)
{
    if (b_color) {
        color += 0.01f;
        // it is a rgb value protecter
        // if the rgb value reach to 1, reverse the calculation which minus 0.01 to 0
        if (color >= 1.0f)
            b_color = false;
    }
    else
    {
        color -= 0.01f;
        // it is a rgb value protecter
        // if the rgb value reach to 0, reverse the calculation which add back to 1
        if (color <= 0.0f)
            b_color = true;
    }
}

/// <summary>
/// Handle Keybaord
/// ESC - Exit the program or close the window
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
}

int main(int argc, char** argv)
{
    // initialize glut
    glutInit(&argc, argv);

    // set window size
    glutInitWindowSize(1920,1080);
    // set window position
    glutInitWindowPosition(100, 100);

    //set display mode
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

    //set window title
    glutCreateWindow("Hello World");

    // handle display
    glutDisplayFunc(Display);
    // handle animation
    glutIdleFunc(myIdle);
    // handle keyboard overwrite
    glutKeyboardFunc(Keyboard);
    // handle redrawing
    glutReshapeFunc(Reshape);
    

    // Explicitly set viewport once at startup too (in case reshape isn't called immediately)
    glViewport(0, 0, 1920, 1080);

    // Start loop
    glutMainLoop();
    return 0;
}