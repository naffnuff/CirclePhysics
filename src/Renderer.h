#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <vector>

namespace CirclePhysics
{

// Circle struct to hold instance data, all in normalized space
struct Circle
{
    // Position
    float x = 0;
    float y = 0;

    // Color
    float r = 0;
    float g = 0;
    float b = 0;

    // Radius
    float radius = 0.f;

    // The width of the strokes that outline the circle 
    float outlineWidth = 0.f;

    // Velocity
    float dx = 0.f;
    float dy = 0.f;
};

class Renderer
{
public:
    Renderer(float initialWindowWidth, float initialWindowHeight, float minRadius, float maxRadius, int spawnLimit, float gravity, bool outlineCircles)
        : m_initialWindowWidth(initialWindowWidth)
        , m_initialWindowHeight(initialWindowHeight)
        , m_minRadius(minRadius)
        , m_maxRadius(maxRadius)
        , m_spawnLimit(spawnLimit)
        , m_gravity(gravity)
        , m_outlineCircles(outlineCircles)
    { }

    ~Renderer()
    {
        cleanUp();
    }

    void intialize();
    void run();
    void cleanUp();

private:
    float m_initialWindowWidth = 0.f;
    float m_initialWindowHeight = 0.f;
    float m_minRadius = 0.f;
    float m_maxRadius = 0.f;
    int m_spawnLimit = 0;
    float m_gravity = 0.f;
    bool m_outlineCircles = true;

    // Global window dimensions that will be updated during resize
    float m_windowWidth = 0.f;
    float m_windowHeight = 0.f;
    GLFWwindow* m_window = nullptr;
    GLuint m_circleVAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;
    GLuint m_instanceVBO = 0;
    GLuint m_circleShaderProgram = 0;
    GLint m_projectionUniform = 0;
    GLint m_outlineCirclesUniform = 0;

    std::vector<Circle> m_circles;

    friend void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};

}