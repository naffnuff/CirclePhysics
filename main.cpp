#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <sstream>
#include <iomanip>

// Global window dimensions that will be updated during resize
int g_windowWidth = 800;
int g_windowHeight = 600;

// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aVertex;
layout (location = 1) in vec2 aOffset;
layout (location = 2) in vec3 aColor;
layout (location = 3) in float aRadius;

out vec2 Fragment;
out vec3 Color;
out float Radius;

uniform mat4 projection;

void main() {
    vec2 pos = aOffset + aVertex * aRadius;
    gl_Position = projection * vec4(pos, 0.0, 1.0);
    Fragment = aVertex;
    Color = aColor;
    Radius = aRadius;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec2 Fragment;
in vec3 Color;
in float Radius;

out vec4 FragColor;

void main() {
    float distanceToFrag = length(Fragment);

    // everything outside of the circle is culled
    if (distanceToFrag > 1.0) {
        discard;
    }
    
    // anti-aliasing; a larger radius means a smaller fraction of the circle should be smoothed
    float smoothWidth = 0.003 / Radius;
    float alpha = 1.0 - smoothstep(1.0 - smoothWidth, 1.0, distanceToFrag);
    
    FragColor = vec4(Color, alpha);
}
)";

// Circle struct to hold instance data
struct Circle {
    float x, y;       // Position
    float r, g, b;    // Color
    float radius;     // Radius
    float dx, dy;     // Velocity
};

// Error callback
void errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// Viewport resize callback
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Update global window dimensions
    g_windowWidth = width;
    g_windowHeight = height;
    glViewport(0, 0, width, height);
}

// Key callback
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// Utility function to compile shaders
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    // Check for compilation errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
        return 0;
    }
    
    return shader;
}

// Utility function to create shader program
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
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
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Shader program linking error: " << infoLog << std::endl;
        return 0;
    }
    
    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

int main(int argc, char* argv[]) {
    // Default values
    int windowWidth = 1024;
    int windowHeight = 768;
    float minRadius = 10.0f;
    float maxRadius = 30.0f;
    int spawnLimit = 1000000;
    float gravity = 98.1f;
    
    // Parse command line arguments
    if (argc > 1) windowWidth = std::atoi(argv[1]);
    if (argc > 2) windowHeight = std::atoi(argv[2]);
    if (argc > 3) minRadius = std::atof(argv[3]);
    if (argc > 4) maxRadius = std::atof(argv[4]);
    if (argc > 5) spawnLimit = std::atoi(argv[5]);
    if (argc > 6) gravity = std::atof(argv[6]);
    
    // Validate input
    if (windowWidth <= 0) windowWidth = 800;
    if (windowHeight <= 0) windowHeight = 600;
    if (minRadius <= 0) minRadius = 10.0f;
    if (maxRadius < minRadius) maxRadius = minRadius * 2;
    if (spawnLimit <= 0) spawnLimit = 500;
    
    std::cout << "Starting simulation with:" << std::endl;
    std::cout << "Window size: " << windowWidth << "x" << windowHeight << std::endl;
    std::cout << "Radius range: " << minRadius << " to " << maxRadius << std::endl;
    std::cout << "Spawn limit: " << spawnLimit << std::endl;
    std::cout << "Gravity: " << gravity << std::endl;

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Set error callback
    glfwSetErrorCallback(errorCallback);
    
    // Set window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Enable window resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Resizable Animated Circle Renderer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    // Set context
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    
    // Enable VSync to match monitor refresh rate
    glfwSwapInterval(1);
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Create and compile shader program - only need the circle program now
    GLuint circleShaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (!circleShaderProgram) {
        glfwTerminate();
        return -1;
    }
    
    // Quad vertices for a unit circle (actually a square that will be made circular in the fragment shader)
    float vertices[] = {
        -1.0f, -1.0f,  // bottom left
         1.0f, -1.0f,  // bottom right
         1.0f,  1.0f,  // top right
        -1.0f,  1.0f   // top left
    };
    
    // Indices for drawing the quad as two triangles
    unsigned int indices[] = {
        0, 1, 2,  // first triangle
        2, 3, 0   // second triangle
    };
    
    // Generate random circles
    std::vector<Circle> circles;
    circles.reserve(spawnLimit);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> radiusDist(minRadius / windowHeight, maxRadius / windowHeight);
    std::uniform_real_distribution<float> velDist(-0.001f, 0.001f); // Velocity for animation
    
    // Initial aspect ratio for positioning
    float initialAspectRatio = (float)windowWidth / (float)windowHeight;
    std::uniform_real_distribution<float> xPosDist(-initialAspectRatio * 0.9f, initialAspectRatio * 0.9f);
    std::uniform_real_distribution<float> yPosDist(-0.9f, 0.9f);
    
    // Create buffers for circles
    GLuint circleVAO, VBO, EBO, instanceVBO;
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &instanceVBO);
    
    // Bind VAO
    glBindVertexArray(circleVAO);
    
    // Setup vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Setup element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Setup instance buffer
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, circles.size() * sizeof(Circle), circles.data(), GL_DYNAMIC_DRAW);
    
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
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // No need for text rendering VAOs or textures anymore
    
    // Initialize window dimensions
    g_windowWidth = windowWidth;
    g_windowHeight = windowHeight;
    
    // Get uniform location for projection matrix
    GLint projectionLoc = glGetUniformLocation(circleShaderProgram, "projection");
    
    // FPS calculation variables
    double lastTime = glfwGetTime();
    double lastReportTime = lastTime;
    int nbFrames = 0;
    double fps = 0.0;
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate and report FPS
        double currentTime = glfwGetTime();
        nbFrames++;
        
        // Print FPS to stdout every second
        if (currentTime - lastReportTime >= 1.0) {
            fps = static_cast<double>(nbFrames) / (currentTime - lastReportTime);
            std::cout << "Average FPS: " << std::fixed << std::setprecision(1) << fps << std::endl;
            std::cout << "Window size: " << g_windowWidth << "x" << g_windowHeight << std::endl;
            nbFrames = 0;
            lastReportTime = currentTime;
        }

        float scale = (float)g_windowHeight / windowHeight;
        
        // Update projection matrix based on current window dimensions
        float aspectRatio = (float)g_windowWidth / (float)g_windowHeight;
        float projection[16] = {
            1.0f / scale / aspectRatio, 0.0f,           0.0f, 0.0f,
            0.0f,                       1.0f / scale,   0.0f, 0.0f,
            0.0f,                       0.0f,           1.0f, 0.0f,
            0.0f,                       0.0f,           0.0f, 1.0f
        };
        
        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

    
        if (circles.size() < spawnLimit) {
            Circle circle;
            circle.x = xPosDist(gen);
            circle.y = yPosDist(gen);
            circle.r = colorDist(gen);
            circle.g = colorDist(gen);
            circle.b = colorDist(gen);
            circle.radius = radiusDist(gen);
            circle.dx = velDist(gen);
            circle.dy = velDist(gen);
            circles.push_back(circle);
        }
        
        // Update circle positions for animation
        for (auto& circle : circles) {
            // Update position
            circle.x += circle.dx;
            circle.y += circle.dy;
            
            // Bounce off walls - using fixed world coordinates regardless of window size
            float worldBoundX = scale * aspectRatio; // World bounds scale with aspect ratio
            float worldBoundY = scale;
            
            if (circle.x < -worldBoundX + circle.radius) {
                circle.x = -worldBoundX + circle.radius;
                circle.dx = -circle.dx;
            } else if (circle.x > worldBoundX - circle.radius) {
                circle.x = worldBoundX - circle.radius;
                circle.dx = -circle.dx;
            }
            if (circle.y < -worldBoundY + circle.radius) {
                circle.y = -worldBoundY + circle.radius;
                circle.dy = -circle.dy;
            } else if (circle.y > worldBoundY - circle.radius) {
                circle.y = worldBoundY - circle.radius;
                circle.dy = -circle.dy;
            }
        }
        
        // Update instance buffer with new positions
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, circles.size() * sizeof(Circle), circles.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // Draw circles
        glUseProgram(circleShaderProgram);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);
        
        glBindVertexArray(circleVAO);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, circles.size());
        glBindVertexArray(0);
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Clean up
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteProgram(circleShaderProgram);
    
    glfwTerminate();
    return 0;
}