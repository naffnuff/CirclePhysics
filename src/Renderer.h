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
        float physicsFrequency = 0.f;
        bool scalePhysics = false;
    };

    Renderer(const Config& config, Engine& engine)
        : m_config(config)
        , m_engine(engine)
    { }

    ~Renderer()
    {
        cleanUp();
    }

    void intialize();
    void run();
    void cleanUp();

private:
    Config m_config;

    // Global window dimensions that will be updated during resize
    float m_windowWidth = 0.f;
    float m_windowHeight = 0.f;

    GLFWwindow* m_window = nullptr;
    GLuint m_vertexArray = 0;
    GLuint m_vertexBuffer = 0;
    GLuint m_indexBuffer = 0;
    GLuint m_instanceBuffer = 0;
    GLuint m_circleShaderProgram = 0;
    GLint m_projectionUniform = 0;
    GLint m_outlineCirclesUniform = 0;
    GLint m_interpolationFactorUniform = 0;

    Engine& m_engine;

    friend void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};

}