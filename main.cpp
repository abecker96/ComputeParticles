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

// imgui includes
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imstb_rectpack.h"
#include "imgui/imstb_textedit.h"
#include "imgui/imstb_truetype.h"
#include "imgui/imconfig.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

// Project-specific includes
#include "common/AidanGLCamera.h"
#include "common/LoadShaders.h"
#include "common/UsefulFunctions.h"

int NUM_PARTICLES = 1500 * 1500; // total number of particles to move
int WORK_GROUP_SIZE = 100;       // # work-items per work-group

// create references to SSBO's for these data
GLuint posSSbo;
GLuint velSSbo;
GLuint colSSbo;

// Declaration of Camera object
Camera camera = Camera();

int windowWidth, windowHeight, windowSizeX, windowSizeY;
int boundingSphereEnable, floorEnable;
float cameraSpeed, mouseSensitivity, horizontalAngle, verticalAngle, initialFoV;
float particleSize, colorSpeed, colorScale;
float simulationSpeed, blackHoleGravity, blackHoleSpeed, blackHoleYcoord, blackHoleXcoord;
float blackHoleZcoord, blackHoleXZDisp, blackHoleYDisp, floorPos;
bool userCameraInput, runSim, floorCheckBoxFlag;
bool sphereCheckBoxFlag;
glm::vec3 cameraPosition, startColorA, startColorB, endColorA, endColorB;
glm::vec4 sphere;
ImVec4 clearColor;
GLuint renderShader, computeShader, vao;
glm::mat4 viewMatrix, projectionMatrix;
GLint viewMatRef, projMatRef, BH1Ref, BH2Ref, sphereRef, DTRef, particleSizeRef;
GLint BHGravityRef, sphereEnableRef, floorEnableRef, colorScaleRef, startColorRef;
GLint endColorRef, bouncingRef, floorPosRef;

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

void initGlobals()
{
    // I really need to implement a user interaction method that
    // requires fewer global variables
    cameraSpeed = 500.0f;
    mouseSensitivity = 0.05f;                  // Mouse sensitivity
    horizontalAngle = 0.0f;                    // initial camera angle
    verticalAngle = 0.0f;                      // initial camera angle
    initialFoV = 62.0f;                        // initial camera field of view
    cameraPosition = glm::vec3(0.0f, 500.0f, -1800.0f); // initial camera position
    particleSize = 1000.0f;
    renderShader = createShaders("shaders/vert.glsl", "shaders/frag.glsl");
    computeShader = createComputeShader("shaders/compute.glsl");
    colorSpeed = 0.0f;
    colorScale = 2.5f;
    simulationSpeed = 400.0f;
    blackHoleGravity = 25.0f;
    blackHoleSpeed = 0.0005f;
    blackHoleYcoord = 0.0f;
    blackHoleXcoord = 0.0f;
    blackHoleZcoord = 0.0f;
    blackHoleXZDisp = 300.0f;
    blackHoleYDisp = 100.0f;
    floorPos = -1000.0f;

    boundingSphereEnable = 1;

    userCameraInput = true;
    runSim = false;

    startColorA = glm::vec3(0.0f, 0.0f, 0.8f);
    startColorB = glm::vec3(0.0f, 0.494f, 0.7843f);
    endColorA = glm::vec3(1.0f, 0.0f, 0.0f);
    endColorB = glm::vec3(0.117f, 1.0f, 0.0f);

    clearColor = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
    sphere = glm::vec4(0.0f, 0.0f, 0.0f, 1000.0f);

    // initialize viewMatrix reference in render shader
    viewMatRef = glGetUniformLocation(renderShader, "viewMat");
    if (viewMatRef < 0)
    {
        std::cerr << "couldn't find viewMatRef in shader\n";
    }
    // initialize viewMatrix reference in render shader
    projMatRef = glGetUniformLocation(renderShader, "projMat");
    if (projMatRef < 0)
    {
        std::cerr << "couldn't find projMatRef in shader\n";
    }
    // initialize DT reference in compute shader
    particleSizeRef = glGetUniformLocation(renderShader, "particleSizeScalar");
    if (particleSizeRef < 0)
    {
        std::cerr << "couldn't find particleSizeRef in shader\n";
    }
    // initialize blackHole 1 reference in compute shader
    BH1Ref = glGetUniformLocation(computeShader, "blackHole1");
    if (BH1Ref < 0)
    {
        std::cerr << "couldn't find BH1Ref in shader\n";
    }
    // initialize blackHole 2 reference in compute shader
    BH2Ref = glGetUniformLocation(computeShader, "blackHole2");
    if (BH2Ref < 0)
    {
        std::cerr << "couldn't find BH2Ref in shader\n";
    }
    // initialize sphere reference in compute shader
    sphereRef = glGetUniformLocation(computeShader, "sphere");
    if (sphereRef < 0)
    {
        std::cerr << "couldn't find BH2Ref in shader\n";
    }
    // initialize floorPos reference in compute shader
    floorPosRef = glGetUniformLocation(computeShader, "floorPos");
    if (floorPosRef < 0)
    {
        std::cerr << "couldn't find floorPosRef in shader\n";
    }
    // initialize floorEnable reference in compute shader
    floorEnableRef = glGetUniformLocation(computeShader, "floorEnable");
    if (floorEnableRef < 0)
    {
        std::cerr << "couldn't find floorEnableRef in shader\n";
    }
    // initialize blackHoleGravity reference in compute shader
    BHGravityRef = glGetUniformLocation(computeShader, "blackHoleAccel");
    if (BHGravityRef < 0)
    {
        std::cerr << "couldn't find BHGravityRef in shader\n";
    }
    // initialize blackHoleGravity reference in compute shader
    sphereEnableRef = glGetUniformLocation(computeShader, "sphereEnable");
    if (sphereEnableRef < 0)
    {
        std::cerr << "couldn't find sphereEnableRef in shader\n";
    }
    // initialize blackHoleGravity reference in compute shader
    colorScaleRef = glGetUniformLocation(computeShader, "colorScale");
    if (colorScaleRef < 0)
    {
        std::cerr << "couldn't find colorScaleRef in shader\n";
    }
    // initialize blackHoleGravity reference in compute shader
    startColorRef = glGetUniformLocation(computeShader, "startColor");
    if (startColorRef < 0)
    {
        std::cerr << "couldn't find startColorRef in shader\n";
    }
    // initialize blackHoleGravity reference in compute shader
    endColorRef = glGetUniformLocation(computeShader, "endColor");
    if (endColorRef < 0)
    {
        std::cerr << "couldn't find endColorRef in shader\n";
    }
    // initialize DT reference in compute shader
    DTRef = glGetUniformLocation(computeShader, "DT");
    if (DTRef < 0)
    {
        std::cerr << "couldn't find DTRef in shader\n";
    }
}

glm::vec3 randomInSphere()
{
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
    glm::vec4 *points = (glm::vec4 *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(glm::vec4), bufMask);
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        points[i] = glm::vec4(randomInSphere(), 1.0f);
        // std::cout << points[i].x << " " << points[i].y << " " << points[i].z << std::endl;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glGenBuffers(1, &velSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, velSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(glm::vec4), NULL, GL_STATIC_DRAW);
    glm::vec4 *vels = (glm::vec4 *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(glm::vec4), bufMask);
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        vels[i] = 0.1f * glm::vec4(randomInSphere(), 0.0f);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glGenBuffers(1, &colSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, colSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(glm::vec4), NULL, GL_STATIC_DRAW);
    glm::vec4 *colors = (glm::vec4 *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(glm::vec4), bufMask);
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        // float randNum = randomBetween(0.5, 1);
        colors[i] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, posSSbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, velSSbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, colSSbo);
}

void toggleCameraInput()
{
    if (userCameraInput)
    {
        camera.disableUserInput();
    }
    else
    {
        camera.enableUserInput();
    }
    userCameraInput = !userCameraInput;
}

void initShaders(GLFWwindow *window)
{
    initSSBOs();

    viewMatrix = camera.getViewMatrix();
    projectionMatrix = camera.getProjectionMatrix();
    glUniformMatrix4fv(viewMatRef, 1, GL_FALSE, glm::value_ptr(viewMatrix));       // update viewmatrix in shader
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

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glUseProgram(renderShader);

    // Initialize camera object
    camera.init(
        window, cameraPosition,
        glm::perspective(
            glm::radians<float>(55),
            (float)windowSizeX / (float)windowSizeY,
            0.01f,
            10000.0f),
        horizontalAngle, verticalAngle,
        cameraSpeed, mouseSensitivity,
        true);

    toggleCameraInput();
    if (userCameraInput)
    {
        camera.update();
    }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_Q:
            toggleCameraInput();
            break;
        default:
            break;
        }
    }
}

GLFWwindow *initWindow(int &wW, int &wH, int &wX, int &wY)
{
    // Necessary due to glew bug
    glewExperimental = true;
    // Initialize glfw
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return NULL;
    }
    // Create the window
    glfwWindowHint(GLFW_SAMPLES, 8);               // 8x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // We want OpenGL 4.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
    // Open a window and create its OpenGL context
    GLFWwindow *window;
    // Check size of screen's available work area
    // This is the area not taken up by taskbars and other OS objects
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &wW, &wH, &wX, &wY);

    // make window
    window = glfwCreateWindow(wX, wY, "Aidan Becker Assignment 5", NULL, NULL);
    if (window == NULL)
    {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible.\n");
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window); // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return NULL;
    }

    // Ensure we can capture the escape key and mouse clicks being pressed below
    // This sets a flag that a key has been pressed, even if it was between frames
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);

    // TODO: probably not necessary for this project, because only rendering points at the moment.
    // Enable depth test so objects render based on closest distance from camera
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glfwSetKeyCallback(window, key_callback);

    return window;
}

void initIMGUI(GLFWwindow *window)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
}

void updateComputeShader(float deltaTime)
{
    static float simTime = 0.0f;
    double clockTime = glfwGetTime();

    simTime += deltaTime * simulationSpeed;
    glUniform1f(DTRef, deltaTime * simulationSpeed);
    glUniform1i(sphereEnableRef, boundingSphereEnable);

    glUniform3f(BH1Ref,
                blackHoleXZDisp * std::sin(simTime * blackHoleSpeed) + blackHoleXcoord,
                blackHoleYDisp + blackHoleYcoord,
                blackHoleXZDisp * std::cos(simTime * blackHoleSpeed) + blackHoleZcoord);
    glUniform3f(BH2Ref,
                blackHoleXZDisp * std::sin(3.1415 + simTime * blackHoleSpeed) + blackHoleXcoord,
                -blackHoleYDisp + blackHoleYcoord,
                blackHoleXZDisp * std::cos(3.1415 + simTime * blackHoleSpeed) + blackHoleZcoord);
    glUniform4fv(sphereRef, 1, glm::value_ptr(sphere));
    glUniform1f(BHGravityRef, blackHoleGravity);

    glUniform1f(floorPosRef, floorPos);
    glUniform1i(floorEnableRef, floorEnable);

    glm::vec3 startColor = startColorA * (float)std::abs(1.570796 + std::sin(clockTime * colorSpeed)) + startColorB * (float)std::abs(std::sin(clockTime * colorSpeed));
    glm::vec3 endColor = endColorA * (float)std::abs(1.570796 + std::sin(clockTime * colorSpeed)) + endColorB * (float)std::abs(std::sin(clockTime * colorSpeed));

    glUniform3f(startColorRef, startColor.r, startColor.g, startColor.b);
    glUniform3f(endColorRef, endColor.r, endColor.g, endColor.b);
    glUniform1f(colorScaleRef, colorScale);
}

void updateRenderShader()
{
    if (userCameraInput)
    {
        camera.update();
    }
    viewMatrix = camera.getViewMatrix();
    projectionMatrix = camera.getProjectionMatrix();
    glUniformMatrix4fv(viewMatRef, 1, GL_FALSE, glm::value_ptr(viewMatrix));       // update viewmatrix in shader
    glUniformMatrix4fv(projMatRef, 1, GL_FALSE, glm::value_ptr(projectionMatrix)); // update projection matrix in shader
    glUniform1f(particleSizeRef, particleSize);
}

void renderImGui(GLFWwindow *window){        
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {   // GUI options
        ImGui::Begin("Particle System Settings"); 
        if (ImGui::Button("Start"))
        {
            runSim = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
            runSim = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reverse Sim"))
        {
            simulationSpeed = -simulationSpeed;
        }
        ImGui::SameLine();
        if (ImGui::Button("Restart"))
        {
            initSSBOs();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Camera"))
        {   // re-initialize camera object
            camera.init(
                window, cameraPosition,
                glm::perspective(
                    glm::radians<float>(55),
                    (float)windowSizeX / (float)windowSizeY,
                    0.01f,
                    10000.0f),
                horizontalAngle, verticalAngle,
                cameraSpeed, mouseSensitivity,
                true);
        }
        ImGui::Text("Press Q to toggle manual camera control.");          
        ImGui::Text("WASD+Mouse.\nShift to move down, Space to move up.");
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Graphics"))
        {
            ImGui::ColorEdit4("Background color   ", (float *)&clearColor); 
            ImGui::ColorEdit3("Low-speed color 1  ", (float *)&startColorA);
            ImGui::ColorEdit3("High-speed color 1 ", (float *)&endColorA);  
            ImGui::ColorEdit3("Low-speed color 2  ", (float *)&startColorB);
            ImGui::ColorEdit3("High-speed color 2 ", (float *)&endColorB);  
            ImGui::Text("This slider adjusts the amount of particles displaying\nlow-speed colors versus high-speed colors.");
            ImGui::SliderFloat("Color scale", &colorScale, 0.0f, 50.0f);
            ImGui::SliderFloat("Color speed", &colorSpeed, 0.0f, 5.0f); 
            ImGui::SliderFloat("Particle size", &particleSize, 0.0f, 10000.0f);
        }
        if (ImGui::CollapsingHeader("Physics Settings"))
        {
            ImGui::Indent();
            ImGui::SliderFloat("Timestep", &simulationSpeed, -5000.0f, 5000.0f);
            if (ImGui::CollapsingHeader("Center Mass Settings"))
            {
                ImGui::Indent();
                if (ImGui::Button("Set zero acceleration"))
                {
                    blackHoleGravity = 0.0f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset position"))
                {
                    blackHoleXcoord = 0.0f;
                    blackHoleYcoord = 0.0f;
                    blackHoleZcoord = 0.0f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset displacement"))
                {
                    blackHoleXZDisp = 300.0f;
                    blackHoleYDisp = 100.0f;
                }
                ImGui::SliderFloat("Acceleration", &blackHoleGravity, -500.0f, 500.0f);
                ImGui::SliderFloat("X-coordinate ", &blackHoleXcoord, -1000.0f, 1000.0f);
                ImGui::SliderFloat("Y-coordinate ", &blackHoleYcoord, -1000.0f, 1000.0f);
                ImGui::SliderFloat("Z-coordinate ", &blackHoleZcoord, -1000.0f, 1000.0f);
                ImGui::SliderFloat("XZ-displacement", &blackHoleXZDisp, 0.0f, 1000.0f);
                ImGui::SliderFloat("Y-displacement", &blackHoleYDisp, 0.0f, 1000.0f);
                ImGui::Unindent();
            }
            if (ImGui::CollapsingHeader("Bounding Sphere Settings"))
            {
                ImGui::Indent();
                ImGui::Checkbox("Sphere On/off", &sphereCheckBoxFlag);
                ImGui::SliderFloat("X-coordinate", &sphere.x, -1000.0f, 1000.0f);
                ImGui::SliderFloat("Y-coordinate", &sphere.y, -1000.0f, 1000.0f);
                ImGui::SliderFloat("Z-coordinate", &sphere.z, -1000.0f, 1000.0f);
                ImGui::SliderFloat("Radius", &sphere.w, 0.0f, 1000.0f);
                ImGui::Unindent();
            }
            if(ImGui::CollapsingHeader("Bouncy Floor Settings")){
                ImGui::Indent();
                ImGui::Checkbox("Floor On/off", &floorCheckBoxFlag);
                ImGui::SliderFloat("Floor Y-offset", &floorPos, -1000.0f, 1000.0f);
                ImGui::Unindent();
            }
        }
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }
    if (sphereCheckBoxFlag)
    {
        boundingSphereEnable = 1;
    }
    else
    {
        boundingSphereEnable = 0;
    }
    if (floorCheckBoxFlag)
    {
        floorEnable = 1;
    }
    else
    {
        floorEnable = 0;
    }

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main()
{
    GLFWwindow *window = initWindow(windowWidth, windowHeight, windowSizeX, windowSizeY);

    initIMGUI(window);
    initGlobals();
    initShaders(window);

    // init timing variables
    double start = glfwGetTime();
    double current;
    double deltaTime;
    do
    {
        // Update input events
        glfwPollEvents();

        // Clear the screen before drawing new things
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // get time since last frame
        current = glfwGetTime();
        deltaTime = current - start;

        start = glfwGetTime();

        if (runSim)
        {
            glUseProgram(computeShader);
            updateComputeShader(deltaTime);
            glDispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        glUseProgram(renderShader);
        updateRenderShader();
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

        renderImGui(window);

        // actually draw created frame to screen
        glfwSwapBuffers(window);

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);

    return 0;
}