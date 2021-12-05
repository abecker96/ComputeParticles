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

#define NUM_PARTICLES 1500 * 1500 // total number of particles to move
#define WORK_GROUP_SIZE 100       // # work-items per work-group

// create references to SSBO's for these data
GLuint posSSbo;
GLuint velSSbo;
GLuint colSSbo;

// Declaration of Camera object
Camera camera = Camera();

int windowWidth, windowHeight, windowSizeX, windowSizeY;
float cameraSpeed, mouseSensitivity, horizontalAngle, verticalAngle, initialFoV, particleSize, colorSpeed, colorScale;
float simulationSpeed, blackHoleGravity, blackHoleSpeed;
bool userCameraInput;
glm::vec3 cameraPosition, startColorA, startColorB, endColorA, endColorB;
GLuint renderShader, computeShader, vao;
glm::mat4 viewMatrix, projectionMatrix;
GLint viewMatRef, projMatRef, BH1Ref, BH2Ref, DTRef, particleSizeRef, BHGravityRef, sphereEnableRef, colorScaleRef, startColorRef, endColorRef;


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

void initGlobals(){
    // I really need to implement a user interaction method that
    // requires fewer global variables
    cameraSpeed = 500.0f;
    mouseSensitivity = 0.05f;    // Mouse sensitivity
    horizontalAngle = 0.0f;            // initial camera angle
    verticalAngle = 0.0f;              // initial camera angle
    initialFoV = 62.0f;                // initial camera field of view
    userCameraInput = true;
    cameraPosition = glm::vec3(0, 500, -1800); // initial camera position
    particleSize = 1000.0f;
    renderShader = createShaders("shaders/vert.glsl", "shaders/frag.glsl");
    computeShader = createComputeShader("shaders/compute.glsl");
    colorSpeed = 0.25;
    colorScale = 2.5;
    simulationSpeed = 800;
    blackHoleGravity = 25;
    blackHoleSpeed = 0.4;

    startColorA = glm::vec3(0.0, 0.0, 0.8);
    startColorB = glm::vec3(0.8, 0.66, 0.0);
    endColorA = glm::vec3(1.0, 0.0, 0.0);
    endColorB = glm::vec3(0.0, 1.0, 0.28);

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

void initShaders(GLFWwindow *window){
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

GLFWwindow* initWindow(int &wW, int &wH, int &wX, int &wY){
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

void initIMGUI(GLFWwindow *window){
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

void updateComputeShader(float deltaTime){
    glUniform1f(DTRef, deltaTime * simulationSpeed);
    glUniform1i(sphereEnableRef, 0);

    glUniform3f(BH1Ref, 300.0f * std::sin(glfwGetTime()*blackHoleSpeed), 100, 300.0f * std::cos(glfwGetTime()*blackHoleSpeed));
    glUniform3f(BH2Ref, 300.0f * std::sin(3.1415 + glfwGetTime()*blackHoleSpeed), -100, 300.0f * std::cos(3.1415 + glfwGetTime()*blackHoleSpeed));
    glUniform1f(BHGravityRef, blackHoleGravity);

    glm::vec3 startColor = startColorA*(float)std::abs(1.570796 + std::sin(glfwGetTime()*colorSpeed)) + startColorB*(float)std::abs(std::sin(glfwGetTime()*colorSpeed));
    glm::vec3 endColor = endColorA*(float)std::abs(1.570796 + std::sin(glfwGetTime()*colorSpeed)) + endColorB*(float)std::abs(std::sin(glfwGetTime()*colorSpeed));
    
    glUniform3f(startColorRef, startColor.r, startColor.g, startColor.b);
    glUniform3f(endColorRef, endColor.r, endColor.g, endColor.b);
    glUniform1f(colorScaleRef, colorScale);
}

void updateRenderShader(){
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

int main()
{
    GLFWwindow *window = initWindow(windowWidth, windowHeight, windowSizeX, windowSizeY);
    
    initIMGUI(window);
    initGlobals();

    // variables for ImGui windows
    bool show_demo_window = true;
    bool show_another_window = false;

    initShaders(window);

    // init timing variables
    double start = glfwGetTime();
    double current;
    double deltaTime;
    // double sumFrameTimes = 0;
    int numFrames = 0;
    // Set default background color to something closer to a night sky
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.05, 0.05, 0.05, 1.0);
    do
    {
        // Update input events
        glfwPollEvents();

        // Clear the screen before drawing new things
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // get time since last frame
        current = glfwGetTime();
        deltaTime = current - start;
        numFrames++;

        // Optionally output frametimes
        // std::cout << "New Frame in: " << deltaTime << std::endl;
        // Reset timer
        start = glfwGetTime();

        glUseProgram(computeShader);
        updateComputeShader(deltaTime);
        glDispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glUseProgram(renderShader);
        updateRenderShader();
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

        // if (show_demo_window)
        {
            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
            ImGui::ShowDemoWindow(&show_demo_window);
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Simulation Settings"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float *)&startColorA); // Edit 3 floats representing a color

            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // actually draw created frame to screen
        glfwSwapBuffers(window);

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);

    double avgFrameTime = glfwGetTime() / (double)numFrames;
    std::cout << "Average frametimes for this run: " << avgFrameTime << std::endl;

    return 0;
}