#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Engine.h"

namespace CirclePhysics
{

class Renderer
{
public:
    struct Config
    {
        float initialWindowWidth = 0.f;
        float initialWindowHeight = 0.f;
        bool outlineCircles = false;
        double physicsFrequency = 0.f;
        bool scalePhysics = false;
    };

    Renderer(const Config& config, Engine& engine)
        : m_config(config)
        , m_engine(engine)
    {
    }

    ~Renderer()
    {
        cleanUp();
    }

    void initialize();
    void run();
    void cleanUp();

private:
    const Config m_config;

    // Global window dimensions that will be updated during resize
    float m_windowWidth = 0.f;
    float m_windowHeight = 0.f;

    GLFWwindow* m_window = nullptr;

    // Base rendering buffers
    GLuint m_vertexArray = 0;
    GLuint m_vertexBuffer = 0;
    GLuint m_indexBuffer = 0;

    // Attribute buffers
    GLuint m_positionXBuffer = 0;
    GLuint m_positionYBuffer = 0;
    GLuint m_prevPositionXBuffer = 0;
    GLuint m_prevPositionYBuffer = 0;
    GLuint m_redBuffer = 0;
    GLuint m_greenBuffer = 0;
    GLuint m_blueBuffer = 0;
    GLuint m_radiusBuffer = 0;
    GLuint m_outlineWidthBuffer = 0;

    GLuint m_circleShaderProgram = 0;

    GLint m_projectionUniform = 0;
    GLint m_outlineCirclesUniform = 0;
    GLint m_interpolationFactorUniform = 0;

    Engine& m_engine;

    double actualPhysicsFrequency = 0.f;

    // GLFW callback functions
    friend void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    friend void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);

    friend void framebufferSizeCallback(GLFWwindow* window, int width, int height);

};

}