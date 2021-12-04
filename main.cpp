// General includes
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

// Opengl includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// Project-specific includes
#include "common/AidanGLCamera.h"
#include "common/LoadShaders.h"
#include "common/UsefulFunctions.h"

#define NUM_PARTICLES 1500*1500 // total number of particles to move
#define WORK_GROUP_SIZE 50       // # work-items per work-group


// create references to SSBO's for these data
GLuint posSSbo;
GLuint velSSbo;
GLuint colSSbo;

// Declaration of Camera object
Camera camera = Camera();

// I really need to implement a user interaction method that
// requires fewer global variables
const float cameraSpeed = 100.0f; 
const float mouseSensitivity = 0.05f;                       //Mouse sensitivity
float horizontalAngle = 0.0f;                               //initial camera angle
float verticalAngle = 0.0f;                                 //initial camera angle
float initialFoV = 62.0f;                                   //initial camera field of view
glm::vec3 cameraPosition(0, 200, -900);                         //initial camera position

//General color scheme to follow
glm::vec3 BACKGROUND_COLOR(0.82f, 0.705f, 0.549f);
glm::vec3 PARTICLE_COLOR(0.8f, 0.8f, 0.8f);

glm::vec3 randomInSphere(){
    float u = randomBetween(0, 1);
    glm::vec3 point = glm::vec3(randomBetween(-1, 1), randomBetween(-1, 1), randomBetween(-1, 1));

    point = glm::normalize(point);

    // cube root
    float c = std::cbrt(u);

    return point * c;
}

void initSSBOs()
{
    glGenBuffers(1, &posSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(glm::vec4), NULL, GL_STATIC_DRAW);
    GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT; // the invalidate makes a big difference when re-writing

    // positions and velocities generated randomly in a sphere using normally distributed random numbers
    // https://karthikkaranth.me/blog/generating-random-points-in-a-sphere/
    glm::vec4 *points = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(glm::vec4), bufMask);
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        points[i] = glm::vec4(randomInSphere(), 1.0f);
        // std::cout << points[i].x << " " << points[i].y << " " << points[i].z << std::endl;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glGenBuffers(1, &velSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, velSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(glm::vec4), NULL, GL_STATIC_DRAW);
    glm::vec4 *vels = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(glm::vec4), bufMask);
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        vels[i] = 0.1f * glm::vec4(randomInSphere(), 0.0f);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glGenBuffers(1, &colSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, colSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(glm::vec4), NULL, GL_STATIC_DRAW);
    glm::vec4 *colors = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(glm::vec4), bufMask);
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        // float randNum = randomBetween(0.5, 1);
        colors[i] = glm::vec4(PARTICLE_COLOR, 1.0f);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void bindSSBOs()
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, posSSbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, velSSbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, colSSbo);
}

GLuint createComputeShader(const char *compute_file_path)
{
    GLuint ComputeShaderID = glCreateShader(GL_COMPUTE_SHADER);

    std::string ComputeShaderCode;
    std::ifstream ComputeShaderStream(compute_file_path, std::ios::in);
    if (ComputeShaderStream.is_open())
    {
        std::stringstream sstr;
        sstr << ComputeShaderStream.rdbuf();
        ComputeShaderCode = sstr.str();
        ComputeShaderStream.close();
    }
    else
    {
        printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", compute_file_path);
        getchar();
        return 0;
    }

    // Init result variables to check return values
    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Compute Shader
    // Read shader as c_string
    char const *ComputeSourcePointer = ComputeShaderCode.c_str();
    // Read shader source into ComputeShaderID
    glShaderSource(ComputeShaderID, 1, &ComputeSourcePointer, NULL);
    // Compile shader
    glCompileShader(ComputeShaderID);

    // Check Compute Shader
    // These functions get the requested shader information
    glGetShaderiv(ComputeShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(ComputeShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> ComputeShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(ComputeShaderID, InfoLogLength, NULL, &ComputeShaderErrorMessage[0]);
        printf("Compiling shader : %s\n", compute_file_path);
        printf("%s\n", &ComputeShaderErrorMessage[0]);
    }

    // Link the program
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, ComputeShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("Linking program\n");
        printf("%s\n", &ProgramErrorMessage[0]);
    }

    // Cleanup
    glDetachShader(ProgramID, ComputeShaderID);
    glDeleteShader(ComputeShaderID);

    // printf("Compute Shader source:\n%s\n", ComputeShaderCode.c_str());

    return ProgramID;
}

GLuint createShaders(const char *vertex_file_path, const char *fragment_file_path)
{
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    // This part is just reading from a file, nothing to do with computer graphics
    // This is always so fiddly, I really don't mind copy/pasting from the tutorial
    // even if I would do it differently.
    // http://www.opengl-tutorial.org/beginners-tutorials/tutorial-2-the-first-triangle/
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if (VertexShaderStream.is_open())
    {
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        VertexShaderCode = sstr.str();
        VertexShaderStream.close();
    }
    else
    {
        printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
        getchar();
        return 0;
    }
    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if (FragmentShaderStream.is_open())
    {
        std::stringstream sstr;
        sstr << FragmentShaderStream.rdbuf();
        FragmentShaderCode = sstr.str();
        FragmentShaderStream.close();
    }
    else
    {
        printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", fragment_file_path);
        getchar();
        return 0;
    }

    // Init result variables to check return values
    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    // Read shader as c_string
    char const *VertexSourcePointer = VertexShaderCode.c_str();
    // Read shader source into VertexShaderID
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    // Compile shader
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    // These functions get the requested shader information
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("Compiling shader : %s\n", vertex_file_path);
        printf("%s\n", &VertexShaderErrorMessage[0]);
    }

    // Compile Fragment Shader
    // Same steps as vertex shader
    // TODO: condense into a single function or loop maybe?
    char const *FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        printf("Compiling shader : %s\n", fragment_file_path);
        printf("%s\n", &FragmentShaderErrorMessage[0]);
    }

    // Link the program
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("Linking program\n");
        printf("%s\n", &ProgramErrorMessage[0]);
    }

    // Cleanup
    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    // printf("Vertex Shader source:\n%s\n", VertexShaderCode.c_str());
    // printf("Geometry Shader source:\n%s\n", GeometryShaderCode.c_str());
    // printf("Fragment Shader source:\n%s\n", FragmentShaderCode.c_str());

    return ProgramID;
}

int main()
{

    /*========  Begin: window creation ========*/
    int windowWidth, windowHeight, windowSizeX, windowSizeY;

    // Necessary due to glew bug
    glewExperimental = true;

    // Initialize glfw
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // Create the window
    glfwWindowHint(GLFW_SAMPLES, 8);              // 8x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // We want OpenGL 4.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

    // Open a window and create its OpenGL context
    GLFWwindow *window;

    // Check size of screen's available work area
    // This is the area not taken up by taskbars and other OS objects
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &windowWidth, &windowHeight, &windowSizeX, &windowSizeY);

    // make window
    window = glfwCreateWindow(windowSizeX, windowSizeY, "Aidan Becker Assignment 5", NULL, NULL);
    if (window == NULL)
    {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    // Ensure we can capture the escape key and mouse clicks being pressed below
    // This sets a flag that a key has been pressed, even if it was between frames
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);

    // TODO: probably not necessary for this project, because only rendering points at the moment.
    // Enable depth test so objects render based on closest distance from camera
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    /*========  End: window creation ========*/
    // TODO: I should probably functionalize that and put it in a lib

    // Initialize camera object
    camera.init(
        window, cameraPosition, 
        glm::perspective(
            glm::radians<float>(55),
            (float)windowSizeX/(float)windowSizeY,
            0.01f, 
            10000.0f
        ),
        horizontalAngle, verticalAngle,
        cameraSpeed, mouseSensitivity,
        true
    );
    glm::mat4 viewMatrix, projectionMatrix;

    initSSBOs();
    bindSSBOs();

    GLuint renderShader = createShaders("shaders/vert.glsl", "shaders/frag.glsl");
    GLuint computeShader = createComputeShader("shaders/compute.glsl");
    GLuint vao;
    GLint viewMatRef, projMatRef, BH1Ref, BH2Ref, DTRef;

    // initialize viewMatrix reference in render shader
    viewMatRef = glGetUniformLocation(renderShader, "viewMat");
    if(viewMatRef < 0)
    {   std::cerr << "couldn't find viewMatRef in shader\n";  }

    // initialize viewMatrix reference in render shader
    projMatRef = glGetUniformLocation(renderShader, "projMat");
    if(projMatRef < 0)
    {   std::cerr << "couldn't find projMatRef in shader\n";  }

    // initialize blackHole 1 reference in compute shader
    BH1Ref = glGetUniformLocation(computeShader, "blackHole1");
    if(BH1Ref < 0)
    {   std::cerr << "couldn't find BH1Ref in shader\n";  }

    // initialize blackHole 2 reference in compute shader
    BH2Ref = glGetUniformLocation(computeShader, "blackHole2");
    if(BH2Ref < 0)
    {   std::cerr << "couldn't find BH2Ref in shader\n";  }

    // initialize DT reference in compute shader
    DTRef = glGetUniformLocation(computeShader, "DT");
    if(DTRef < 0)
    {   std::cerr << "couldn't find DTRef in shader\n";  }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);


    glEnable(GL_PROGRAM_POINT_SIZE);


    // glPointSize(2.5f);


    glUseProgram(renderShader);

    camera.update();
    viewMatrix = camera.getViewMatrix();
    projectionMatrix = camera.getProjectionMatrix();
    glUniformMatrix4fv(viewMatRef, 1, GL_FALSE, glm::value_ptr(viewMatrix)); // update viewmatrix in shader
    glUniformMatrix4fv(projMatRef, 1, GL_FALSE, glm::value_ptr(projectionMatrix)); // update projection matrix in shader

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, posSSbo);
    glVertexPointer(4, GL_FLOAT, 0, (void *)0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, velSSbo);
    glVertexPointer(4, GL_FLOAT, 0, (void *)0);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, colSSbo);
    glVertexPointer(4, GL_FLOAT, 0, (void *)0);

    //init timing variables
    // double start = glfwGetTime();
    // double current;
    // double deltaTime;
    // double sumFrameTimes = 0;
    int numFrames = 0;
    // Set default background color to something closer to a night sky
    glClearColor(0.05, 0.05, 0.05, 1.0);
    do
    {
        // Update input events
        // Most tutorials do this at the end of the main loop
        // But getting input after frames are calculated might lead to significant
        // input delay in the event that frames take a while to render
        glfwPollEvents();
        // Clear the screen before drawing new things
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // get time since last frame
        // current = glfwGetTime();
        // deltaTime = current-start;
        numFrames++;

        // Optionally output frametimes
        // std::cout << "New Frame in: " << deltaTime << std::endl;
        // Reset timer
        // start = glfwGetTime();

        glUseProgram(computeShader);
        glUniform3f(BH1Ref,
            300.0f*std::sin(glfwGetTime()*0.4),
            100, 
            300.0f*std::cos(glfwGetTime()*0.4)
        );
        glUniform3f(BH2Ref,
            300.0f*std::sin(3.1415 + glfwGetTime()*0.4),
            -100, 
            300.0f*std::cos(3.1415 + glfwGetTime()*0.4)
        );
        glUniform1f(DTRef, glfwGetTime()*0.2);
        glDispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glUseProgram(renderShader);

        //camera.update();
    
        viewMatrix = glm::lookAt(glm::vec3(0, 500, 1800), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));//camera.getViewMatrix();
        projectionMatrix = camera.getProjectionMatrix();
        glUniformMatrix4fv(viewMatRef, 1, GL_FALSE, glm::value_ptr(viewMatrix)); // update viewmatrix in shader
        glUniformMatrix4fv(projMatRef, 1, GL_FALSE, glm::value_ptr(projectionMatrix)); // update projection matrix in shader
        // glUniform1fv(DTRef, 1, glm::value_ptr((float)(deltaTime)));

        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // actually draw created frame to screen
        glfwSwapBuffers(window);

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);

    double avgFrameTime = glfwGetTime() / (double)numFrames;
    std::cout << "Average frametimes for this run: " << avgFrameTime << std::endl;

    return 0;
}