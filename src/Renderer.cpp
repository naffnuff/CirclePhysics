#include "Renderer.h"

#include <iostream>
#include <random>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <functional>

#include "Shaders.h"

namespace CirclePhysics {

// Error callback
void errorCallback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// Viewport resize callback
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

    // Update global window dimensions
    renderer->m_windowWidth = (float)width;
    renderer->m_windowHeight = (float)height;

    glViewport(0, 0, width, height);
}

// Key callback
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// Utility function to compile shaders
GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
        return 0;
    }

    return shader;
}

// Utility function to create shader program
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource)
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check for linking errors
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Shader program linking error: " << infoLog << std::endl;
        return 0;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void Renderer::intialize()
{

    // Initialize GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Set error callback
    glfwSetErrorCallback(errorCallback);

    // Set window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Enable window resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow((int)m_initialWindowWidth, (int)m_initialWindowHeight, "Circle Physics", NULL, NULL);
    if (!m_window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(m_window, this);

    // Set context
    glfwMakeContextCurrent(m_window);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    // Enable VSync to match monitor refresh rate
    glfwSwapInterval(1);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create and compile shaders
    m_circleShaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (!m_circleShaderProgram)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create shaders");
    }

    // Quad vertices for a unit circle (actually a square that will be made circular in the fragment shader)
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f 
    };

    // Indices for drawing the quad as two triangles
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    m_circles.reserve(m_spawnLimit);

    // Create buffers for circles
    glGenVertexArrays(1, &m_circleVAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    glGenBuffers(1, &m_instanceVBO);

    // Bind VAO
    glBindVertexArray(m_circleVAO);

    // Setup vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Setup element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Setup instance buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, m_circles.size() * sizeof(Circle), m_circles.data(), GL_DYNAMIC_DRAW);

    // Instance attributes
    // Position offset
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Circle), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // Color
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Circle), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // Radius
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Circle), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // Outline width
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Circle), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Initialize window dimensions
    m_windowWidth = m_initialWindowWidth;
    m_windowHeight = m_initialWindowHeight;

    // Set uniforms
    m_projectionUniform = glGetUniformLocation(m_circleShaderProgram, "u_Projection");
    m_outlineCirclesUniform = glGetUniformLocation(m_circleShaderProgram, "u_OutlineCircles");
}
void Renderer::run()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> radiusDist(m_minRadius / m_initialWindowHeight, m_maxRadius / m_initialWindowHeight);
    std::uniform_real_distribution<float> velDist(-0.001f, 0.001f); // Velocity for animation

    // Initial aspect ratio for positioning
    float initialAspectRatio = (float)m_initialWindowWidth / (float)m_initialWindowHeight;
    //std::uniform_real_distribution<float> xPosDist(-initialAspectRatio * 0.9f, initialAspectRatio * 0.9f);
    std::uniform_real_distribution<float> xPosDist(0.0f, 0.0f);
    //std::uniform_real_distribution<float> yPosDist(-0.9f, 0.9f);
    std::uniform_real_distribution<float> yPosDist(0.0, 0.0f);

    // FPS calculation variables
    double lastTime = glfwGetTime();
    double lastReportTime = lastTime;
    int nbFrames = 0;
    double fps = 0.0;

    // Main loop
    while (!glfwWindowShouldClose(m_window))
    {
        // Calculate and report FPS
        double currentTime = glfwGetTime();
        nbFrames++;

        // Print FPS to stdout every second
        if (currentTime - lastReportTime >= 1.0)
        {
            fps = static_cast<double>(nbFrames) / (currentTime - lastReportTime);
            std::cout << "Circle count: " << m_circles.size() << std::endl;
            std::cout << "Average FPS: " << std::fixed << std::setprecision(1) << fps << std::endl;
            std::cout << "Window size: " << m_windowWidth << "x" << m_windowHeight << std::endl;
            nbFrames = 0;
            lastReportTime = currentTime;
        }

        float scale = m_windowHeight / m_initialWindowHeight;

        // Update projection matrix based on current window dimensions
        float aspectRatio = m_windowWidth / m_windowHeight;
        float projection[16] = {
            1.0f / scale / aspectRatio, 0.0f,           0.0f, 0.0f,
            0.0f,                       1.0f / scale,   0.0f, 0.0f,
            0.0f,                       0.0f,           1.0f, 0.0f,
            0.0f,                       0.0f,           0.0f, 1.0f
        };

        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        if (m_circles.size() < m_spawnLimit)
        {
            Circle circle;
            circle.x = xPosDist(gen);
            circle.y = yPosDist(gen);
            circle.r = colorDist(gen);
            circle.g = colorDist(gen);
            circle.b = colorDist(gen);
            circle.radius = radiusDist(gen);
            circle.dx = velDist(gen);
            circle.dy = velDist(gen);
            circle.outlineWidth = 2.f / circle.radius / m_initialWindowHeight;
            m_circles.push_back(circle);
        }

        // Update circle positions for animation
        for (Circle& circle : m_circles)
        {
            // Update position
            circle.x += circle.dx;
            circle.y += circle.dy;

            // Bounce off walls - using fixed world coordinates regardless of window size
            float worldBoundX = scale * aspectRatio; // World bounds scale with aspect ratio
            float worldBoundY = scale;

            if (circle.x < -worldBoundX + circle.radius)
            {
                circle.x = -worldBoundX + circle.radius;
                circle.dx = -circle.dx;
            }
            else if (circle.x > worldBoundX - circle.radius)
            {
                circle.x = worldBoundX - circle.radius;
                circle.dx = -circle.dx;
            }
            if (circle.y < -worldBoundY + circle.radius)
            {
                circle.y = -worldBoundY + circle.radius;
                circle.dy = -circle.dy;
            }
            else if (circle.y > worldBoundY - circle.radius)
            {
                circle.y = worldBoundY - circle.radius;
                circle.dy = -circle.dy;
            }
        }

        // Update instance buffer with new positions
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, m_circles.size() * sizeof(Circle), m_circles.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Draw circles
        glUseProgram(m_circleShaderProgram);
        glUniformMatrix4fv(m_projectionUniform, 1, GL_FALSE, projection);
        glUniform1i(m_outlineCirclesUniform, m_outlineCircles);

        glBindVertexArray(m_circleVAO);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (GLsizei)m_circles.size());
        glBindVertexArray(0);

        // Swap buffers and poll events
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

}

void Renderer::cleanUp()
{
    if (m_circleVAO)
    {
        glDeleteVertexArrays(1, &m_circleVAO);
    }
    if (m_VBO)
    {
        glDeleteBuffers(1, &m_VBO);
    }
    if (m_EBO)
    {
        glDeleteBuffers(1, &m_EBO);
    }
    if (m_instanceVBO)
    {
        glDeleteBuffers(1, &m_instanceVBO);
    }
    if (m_circleShaderProgram)
    {
        glDeleteProgram(m_circleShaderProgram);
    }
}

}