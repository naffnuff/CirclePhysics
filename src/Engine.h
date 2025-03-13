#pragma once

#include <vector>
#include <random>
#include <functional>

#include "Vector2.h"
#include "SpatialGrid.h"

namespace CirclePhysics {

// Struct for all the data that gets sent to the GPU
struct CircleRenderData
{
    // Position
    Vector2 position;

    // Previous position for interpolation
    Vector2 previousPosition;

    // Color
    float r = 0;
    float g = 0;
    float b = 0;

    // Radius
    float radius = 0.f;

    // The width of the strokes that outline the circle 
    float outlineWidth = 0.f;
};

// Driver of the 2D physics simulation.
// Operates in a universe that is centered on Origo,
// and initially stretches for (+-aspect ratio, +-1),
// in all four directions, but can be expanded beyond that.
// Aspect ratio = window width / window height.
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

    Engine(const Config& config)
        : m_config(config)
        // Same bounds as the world, in the same unit space.
        // Max circle diameter as the cell size so that only surrounding cells need to be searched.
        , m_spatialGrid(config.initialAspectRatio, 1.0f, config.maxRadius * 2.0f)
    {
        m_numberGenerator = std::mt19937(std::random_device()());

        spawnXDistribution = std::uniform_real_distribution<float>(-config.initialAspectRatio * 0.9f, config.initialAspectRatio * 0.9f);
        if (config.gravity > 0.f)
        {
            // Drop from ceiling so something happens
            spawnYDistribution = std::uniform_real_distribution<float>(1.0f, 1.0f);
        }
        else
        {
            spawnYDistribution = std::uniform_real_distribution<float>(-0.9f, 0.9f);
        }

        colorDistribution = std::uniform_real_distribution<float>(0.4f, 1.f);
        radiusDistribution = std::uniform_real_distribution<float>(config.minRadius, config.maxRadius);
        velocityDistribution = std::uniform_real_distribution<float>(-1.0f, 1.0f);

        // We might be really sad and confused at some point
        // if these need to reallocate the storage of these two vectors.
        // It is therefore important that the actual circle spawn count
        // never exceeds the spawn limit.
        m_circleRenderData.reserve(config.spawnLimit);
        m_circlePhysicsData.reserve(config.spawnLimit);
    }

    // Expand the world with new bounds.
    // The world is always centered in Origo,
    // so the bounds will be applied equally
    // in all directions.
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

    // Take the next step in the physics simulation
    void step(double simulationTime, double deltaTime);

private:
    void resolveWallCollisions();
    void detectCollisions();
    void resolveCollisions();
    void correctVelocities(const Collision& collision);
    void correctPositions(const Collision& collision);
    void spawnCircles(double simulationTime);

private:
    const Config m_config;

    // Helpers for randomizing circles
    std::mt19937 m_numberGenerator;
    std::uniform_real_distribution<float> spawnXDistribution;
    std::uniform_real_distribution<float> spawnYDistribution;
    std::uniform_real_distribution<float> colorDistribution;
    std::uniform_real_distribution<float> radiusDistribution;
    std::uniform_real_distribution<float> velocityDistribution;

    // 2D grid structure to help with the broad phase collision detection
    SpatialGrid<int> m_spatialGrid;
    // The result from the last use of the spatial grid
    std::vector<std::pair<int, int>> m_potentialCollisionPairs;

    // We keep the circle data in two arrays to keep GPU data minimal
    std::vector<CircleRenderData> m_circleRenderData; // Here goes everyhting that gets sent to the GPU...
    std::vector<CirclePhysicsData> m_circlePhysicsData; //...and here goes the rest

    // Temporary container for all collisions detected during the current step
    std::vector<Collision> m_collisions;

    // For the sake of cleanness, let's keep the common size of the arrays here
    int m_circleCount = 0;

    // Initially in the range -aspect ratio..aspect ratio
    float m_worldBoundX = 0.f;
    // Initially in the range -1..1
    float m_worldBoundY = 0.f;

public:
    // Should always be on in release builds
    bool m_useSpatialPartitioning = true;
};

}