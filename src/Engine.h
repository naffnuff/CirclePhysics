#pragma once

#include <vector>
#include <random>
#include <functional>

#include "Vector2.h"

namespace CirclePhysics {


// Struct for all the data that gets sent to the GPU
struct CircleRenderData
{
    // Position
    Vector2 position;

    // Color
    float r = 0;
    float g = 0;
    float b = 0;

    // Radius
    float radius = 0.f;

    // The width of the strokes that outline the circle 
    float outlineWidth = 0.f;
};

// class that drives the physics simulation
class Engine
{
private:
    // Struct for data that doesn't get sent to the GPU
    struct CirclePhysicsData
    {
        // Velocity
        Vector2 velocity;

        // 1 / object mass
        // 0.f is a valid value, it means the object has infinite mass and cannot be moved by other objects
        float inverseMass = 0.f;

        // The render data representing the same circle
        CircleRenderData& renderData;
    };

    struct Collision
    {
        // Colliding pair
        CirclePhysicsData& first;
        CirclePhysicsData& second;

        // Collision normal
        Vector2 normal;

        // How much closer are the objects than their radii allow?
        float penetration = 0.f;
    };

public:
    struct Config
    {
        float minRadius = 0.f;
        float maxRadius = 0.f;
        int spawnLimit = 0;
        float gravity = 0.f;
        float restitution = 0.f;
        float initialAspectRatio = 0.f;
        float initialWindowHeight = 0.f;
        float spawnRate = 0.f;
        int correctionIterations = 0;
    };

    std::mt19937 m_numberGenerator;

    std::uniform_real_distribution<float> colorDistribution;
    std::uniform_real_distribution<float> radiusDistribution;
    std::uniform_real_distribution<float> velocityDistribution;

    std::uniform_real_distribution<float> spawnXDistribution;
    std::uniform_real_distribution<float> spawnYDistribution;

    Engine(const Config& config)
        : m_config(config)
    {
        m_numberGenerator = std::mt19937(std::random_device()());

        colorDistribution = std::uniform_real_distribution<float>(0.0f, 1.0f);
        radiusDistribution = std::uniform_real_distribution<float>(config.minRadius, config.maxRadius);
        velocityDistribution = std::uniform_real_distribution<float>(-1.0f, 1.0f);

        spawnXDistribution = std::uniform_real_distribution<float>(-m_config.initialAspectRatio * 0.9f, m_config.initialAspectRatio * 0.9f);
        //spawnYDistribution = std::uniform_real_distribution<float>(-0.9f, 0.9f);
        spawnYDistribution = std::uniform_real_distribution<float>(1.0f, 1.0f);

        m_circleRenderData.reserve(m_config.spawnLimit * 2);
        m_circlePhysicsData.reserve(m_config.spawnLimit * 2);
    }

    void setWorldBounds(float worldBoundX, float worldBoundY)
    {
        m_worldBoundX = worldBoundX;
        m_worldBoundY = worldBoundY;
    }

    int getCircleCount() const
    {
        return m_circleCount;
    }
    const CircleRenderData* getRenderData() const
    {
        return m_circleRenderData.data();
    }

    void step(double simulationTime, double deltaTime);
    void resolveWallCollisions();
    void detectCollisions();
    void resolveCollisions();
    void correctVelocities(const Collision& collision);
    void correctPositions(const Collision& collision);
    void spawnCircles(double simulationTime);

private:
    const Config m_config;

    // We keep the circle data in two arrays to keep GPU data minimal
    std::vector<CircleRenderData> m_circleRenderData; // Here goes everyhting that gets sent to the GPU...
    std::vector<CirclePhysicsData> m_circlePhysicsData; //...and here goes the rest

    std::vector<Collision> m_collisions;

    // For the sake of cleanness, let's keep the common size of the arrays here
    int m_circleCount = 0;

    float m_worldBoundX = 0.f;
    float m_worldBoundY = 0.f;

    float m_targetSpawnCount = 0.f;
};

}