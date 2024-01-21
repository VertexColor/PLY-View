/*
    James William Fletcher (github.com/mrbid)
        January 2024

    https://github.com/VertexColor

    cc main.c rply.c glad_gl.c -I inc -Ofast -lglfw -lm -o plv
*/

#include <stdio.h>
#include <stdlib.h>
#define uint GLuint
#define sint GLint
#include "gl.h"
#define GLFW_INCLUDE_NONE
#include "glfw3.h"
#define fTime() (float)glfwGetTime()
#define MAX_MODELS 1 // hard limit, be aware and increase if needed
#define MODEL_DATA_STRIDED // load vertex data as strided
#include "esAux6.h"
#include "rply.h"

const char appTitle[]="PLY Viewer";
uint winw=1024, winh=768;
GLFWwindow* wnd;
mat projection, model;
void updateModel(){glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (float*)&model.m[0][0]);}

// load model from file to gpu memory with a permanent 12 MB staging buffer
#define MAX_SIZE 2097152
GLfloat vertex_buffer[MAX_SIZE];
GLushort index_buffer[MAX_SIZE];
uint vbl = 0, ibl = 0; // buffer lens
uint ntris = 0, nverts = 0;
static int vertex_cb(p_ply_argument argument)
{
    if(vbl > MAX_SIZE-1){return 0;}
    static uint vc = 0;
    long eol;
    ply_get_argument_user_data(argument, NULL, &eol);
    vertex_buffer[vbl] = ply_get_argument_value(argument);
    if(vc > 5){vertex_buffer[vbl] *= 0.003921569f;}
    vbl++;
    vc++;
    if(eol){vc = 0;}
    return 1;
}
static int face_cb(p_ply_argument argument)
{
    if(ibl > MAX_SIZE-1){return 0;}
    long length, value_index;
    ply_get_argument_property(argument, NULL, &length, &value_index);
    switch(value_index)
    {
        case 0:
        case 1: 
            index_buffer[ibl] = ply_get_argument_value(argument);
            ibl++;
            break;
        case 2:
            index_buffer[ibl] = ply_get_argument_value(argument);
            ibl++;
            break;
        default: 
            break;
    }
    return 1;
}
void loadModel(const char* fp)
{
    // reset buffers
    vbl = 0, ibl = 0;

    // open file
    p_ply ply = ply_open(fp, NULL, 0, NULL);
    if(!ply){esModelArray_index++; return;}
    if(!ply_read_header(ply))
    {
        ply_close(ply);
        esModelArray_index++;
        return;
    }

    // read file setup
    nverts = ply_set_read_cb(ply, "vertex", "x", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "y", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "z", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "nx", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "ny", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "nz", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "red", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "green", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "blue", vertex_cb, NULL, 1);
    ntris = ply_set_read_cb(ply, "face", "vertex_indices", face_cb, NULL, 0);

    // read file
    if(!ply_read(ply))
    {
        ply_close(ply);
        esModelArray_index++;
        return;
    }

    // close file
    ply_close(ply);

    // bind to gpu
    esBind(GL_ARRAY_BUFFER, &esModelArray[esModelArray_index].vid, vertex_buffer, vbl*sizeof(GLfloat), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &esModelArray[esModelArray_index].iid, index_buffer, ibl*sizeof(GLushort), GL_STATIC_DRAW);
    esModelArray[esModelArray_index].itp = GL_UNSIGNED_SHORT;
    esModelArray[esModelArray_index].ni = ibl;
    printf("Loaded PLY: %u %u %u\n", esModelArray_index+1, vbl, ibl);
    esModelArray_index++;
}

// process entry point
int main(int argc, char** argv)
{
    // create window with custom MSAA level
    int msaa = 16;
    if(argc >= 2){msaa = atoi(argv[1]);}
    if(!glfwInit()){printf("glfwInit() failed.\n"); exit(EXIT_FAILURE);}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, msaa);
    glfwWindowHint(GLFW_RESIZABLE, 0);
    wnd = glfwCreateWindow(winw, winh, appTitle, NULL, NULL);
    if(!wnd)
    {
        printf("glfwCreateWindow() failed.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    const GLFWvidmode* desktop = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(wnd, (desktop->width/2)-(winw/2), (desktop->height/2)-(winh/2)); // center window on desktop
    glfwMakeContextCurrent(wnd);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync

    // load our test model
    loadModel("test.ply");

    // configure render options
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    makeLambert();
    shadeLambert(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &ambient_id, &saturate_id, &opacity_id);
    glViewport(0, 0, winw, winh);
    mIdent(&projection);
    mPerspective(&projection, 55.0f, (float)winw / (float)winh, 0.01f, 64.f);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (float*)&projection.m[0][0]);
    glUniform1f(ambient_id, 0.32f);
    glUniform1f(saturate_id, 1.f);

    // render loop
    while(!glfwWindowShouldClose(wnd))
    {
        // poll events so that we know when the window is closed
        glfwPollEvents(); 

        // clear buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render model
        mIdent(&model);
        mRotX(&model, 90.f * DEG2RAD);
        mRotY(&model, 90.f * DEG2RAD);
        mRotZ(&model, fTime() * 1.2f);
        mSetPos3(&model, 0.f, 0.f, -2.f);
        updateModel();
        esBindRender(0);

        // display render
        glfwSwapBuffers(wnd);
    }

    // end
    glfwDestroyWindow(wnd);
    glfwTerminate();
    exit(EXIT_SUCCESS);
    return 0;
}
