/*
 * Author: Moksh Shah
 *
 * Description:
 * This program visualizes vector operations such as dot products and cross products
 * using OpenGL. It uses GLFW for window and input management, GLEW for OpenGL
 * extension handling, GLM for matrix and vector math, and ImGui for the user interface.
 *
 * Dependencies:
 * - OpenGL (3.3+)
 * - GLFW
 * - GLEW
 * - GLM
 * - ImGui
 *
 * License: MIT License
 */

#include<GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

// ImGui Setup
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Shader Setup
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 color;
    uniform float alpha;  // Added alpha for opacity
    void main()
    {
        FragColor = vec4(color, alpha);  // Set color with opacity
    }
)";

// Camera Setup
glm::mat4 handleZoom(float zoomLevel)
{
    return glm::perspective(glm::radians(zoomLevel), 800.0f / 600.0f, 0.1f, 100.0f);
}

// Function prototypes
GLFWwindow *initializeWindow();
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
unsigned int compileShader(unsigned int type, const char *source);
unsigned int createShaderProgram(const char *vertexShader, const char *fragmentShader);
void drawVector(unsigned int shaderProgram, const glm::vec3 &vector, const glm::vec3 &color);
void drawDotProduct(unsigned int shaderProgram, const glm::vec3 &v1, const glm::vec3 &v2);
void drawCrossProduct(unsigned int shaderProgram, const glm::vec3 &v1, const glm::vec3 &v2);
unsigned int setupGrid();
void drawGrid(unsigned int shaderProgram, unsigned int gridVAO, int lineCount);
float panX = 0.0f;
float panY = 0.0f;
float panAngleX = 0.0f;
float panAngleY = 0.0f;

// Function to draw grid lines
unsigned int setupGrid()
{
    const float gridSize = 10.0f;
    const float gridStep = 1.0f;
    std::vector<float> gridVertices;

    for (float i = -gridSize; i <= gridSize; i += gridStep)
    {
        gridVertices.push_back(-gridSize);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(i);

        gridVertices.push_back(gridSize);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(i);
    }

    for (float i = -gridSize; i <= gridSize; i += gridStep)
    {
        gridVertices.push_back(i);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(-gridSize);

        gridVertices.push_back(i);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(gridSize);
    }

    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return gridVAO;
}

//Draw Grid for the background
void drawGrid(unsigned int shaderProgram, unsigned int gridVAO, int lineCount)
{
    glBindVertexArray(gridVAO);
    glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.5f, 0.5f, 0.5f); 
    glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), 0.3f);            

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glDrawArrays(GL_LINES, 0, lineCount);
    glBindVertexArray(0);
}

// Draw Vector function

void drawVector(unsigned int shaderProgram, const glm::vec3 &vector, const glm::vec3 &color)
{

    float thickness = 4.0f; 
    float vertices[] = {
        0.0f, 0.0f, 0.01f,             
        vector.x, vector.y, vector.z + 0.01f 
    };


    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Set color uniform
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(color));

    // Set model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.01f)); // Apply a small translation on z-axis
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glLineWidth(thickness);                 
    glEnable(GL_LINE_SMOOTH);             
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glDrawArrays(GL_LINES, 0, 2);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}


// Initialize Window
GLFWwindow *initializeWindow()
{
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "Vector Visualization with Grid", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    return window;
}

// Framebuffer size callback
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// Process Input Window
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Compile Shader
unsigned int compileShader(unsigned int type, const char *source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
        return 0;
    }
    return shader;
}

unsigned int createShaderProgram(const char *vertexShader, const char *fragmentShader)
{
    unsigned int vertexShaderID = compileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fragmentShaderID = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShaderID);
    glAttachShader(shaderProgram, fragmentShaderID);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    return shaderProgram;
}

// Draw Dot Product
void drawDotProduct(unsigned int shaderProgram, const glm::vec3 &v1, const glm::vec3 &v2)
{
    float dotProduct = glm::dot(v1, v2);

    drawVector(shaderProgram, v1, glm::vec3(1.0f, 0.0f, 0.0f)); // Red for v1
    drawVector(shaderProgram, v2, glm::vec3(0.0f, 1.0f, 0.0f)); // Green for v2

    glm::vec3 scaledV2 = glm::normalize(v2) * dotProduct;
    drawVector(shaderProgram, scaledV2, glm::vec3(0.0f, 0.0f, 1.0f)); // Blue for the dot product representation

    std::cout << "Dot product: " << dotProduct << std::endl;
}

// Draw Cross Product
void drawCrossProduct(unsigned int shaderProgram, const glm::vec3 &v1, const glm::vec3 &v2)
{
    // Calculate the cross product
    glm::vec3 crossProduct = glm::cross(v1, v2);

    glLineWidth(4.0f);

    drawVector(shaderProgram, v1, glm::vec3(1.0f, 0.0f, 0.0f)); // Red for v1
    drawVector(shaderProgram, v2, glm::vec3(0.0f, 1.0f, 0.0f)); // Green for v2

    drawVector(shaderProgram, crossProduct, glm::vec3(0.0f, 0.0f, 1.0f)); // Blue for cross product

    // Vertices of the parallelogram formed by v1 and v2
    float parallelogramVertices[] = {
        0.0f, 0.0f, 0.0f,                     // Origin
        v1.x, v1.y, v1.z,                     // v1 endpoint
        v2.x, v2.y, v2.z,                     // v2 endpoint
        v1.x + v2.x, v1.y + v2.y, v1.z + v2.z // Endpoint of v1 + v2 (other corner of parallelogram)
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(parallelogramVertices), parallelogramVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    unsigned int indices[] = {
        0, 1, 2, // First triangle (Origin, v1, v2)
        1, 2, 3  // Second triangle (v1, v2, v1+v2)
    };
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.5f, 0.5f, 0.5f); 
    glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), 0.3f);            

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), 1.0f);

    std::cout << "Area of the parallelogram (cross product magnitude): " << glm::length(crossProduct) << std::endl;
}


// Main Function
int main()
{
    GLFWwindow *window = initializeWindow();
    if (!window)
        return -1;

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProgram);

    // Setup grid
    unsigned int gridVAO = setupGrid();
    int gridLineCount = 40;

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glm::vec3 v1(1.0f, 0.0f, 0.0f);
    glm::vec3 v2(0.0f, 1.0f, 0.0f);
    int operation = 0; // 0 for dot product, 1 for cross product
    float zoomLevel = 45.0f;

    // Main render loop
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(
            glm::vec3(5.0f * cos(glm::radians(panAngleX)) * cos(glm::radians(panAngleY)),
                      5.0f * sin(glm::radians(panAngleY)),
                      5.0f * sin(glm::radians(panAngleX)) * cos(glm::radians(panAngleY))), 
            glm::vec3(panX, panY, 0.0f),                                                   
            glm::vec3(0.0f, 1.0f, 0.0f)                                                   
        );                                                                                
        glm::mat4 projection = handleZoom(zoomLevel);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        drawGrid(shaderProgram, gridVAO, gridLineCount * 2);

        // ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui window for user controls
        ImGui::Begin("Vector Operations");
        ImGui::InputFloat3("v1", glm::value_ptr(v1));
        ImGui::InputFloat3("v2", glm::value_ptr(v2));
        ImGui::Combo("Operation", &operation, "Dot Product\0Cross Product\0");
        ImGui::SliderFloat("Zoom", &zoomLevel, 5.0f, 120.0f);           // Zoom slider
        ImGui::SliderFloat("Pan X", &panX, -10.0f, 10.0f);              // Pan slider for X
        ImGui::SliderFloat("Pan Y", &panY, -10.0f, 10.0f);              // Pan slider for Y
        ImGui::SliderFloat("Pan Angle X", &panAngleX, -180.0f, 180.0f); // Horizontal pan
        ImGui::SliderFloat("Pan Angle Y", &panAngleY, -90.0f, 90.0f);   // Vertical pan

     
        if (operation == 0)
        {
            drawDotProduct(shaderProgram, v1, v2); // Draw dot product operation
        }
        else if (operation == 1)
        {
            drawCrossProduct(shaderProgram, v1, v2); // Draw cross product operation
        }

        ImGui::End();

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteVertexArrays(1, &gridVAO);
    glfwTerminate();
    return 0;
}