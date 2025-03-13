#pragma once

#include <vector>
#include <random>
#include <functional>

#include "Vector2.h"
#include "SpatialGrid.h"

namespace CirclePhysics
{

// Cache-friendly SoA structure for the circle data
struct CircleData
{
    // Position data
    std::vector<float> positionsX;          // All X positions
    std::vector<float> positionsY;          // All Y positions
    std::vector<float> previousPositionsX;  // All previous X positions
    std::vector<float> previousPositionsY;  // All previous Y positions

    // Physics data
    std::vector<float> velocitiesX;         // All X velocities
    std::vector<float> velocitiesY;         // All Y velocities
    std::vector<float> inverseMasses;       // All inverse masses

    // Rendering data
    std::vector<float> radii;               // All radii
    std::vector<float> r;                   // All red components
    std::vector<float> g;                   // All green components
    std::vector<float> b;                   // All blue components
    std::vector<float> outlineWidths;       // All outline widths

    // Helper methods
    inline Vector2 getPosition(int index) const
    {
        return Vector2(positionsX[index], positionsY[index]);
    }

    inline void setPosition(int index, const Vector2& pos)
    {
        positionsX[index] = pos.x;
        positionsY[index] = pos.y;
    }

    inline Vector2 getVelocity(int index) const
    {
        return Vector2(velocitiesX[index], velocitiesY[index]);
    }

    inline void setVelocity(int index, const Vector2& vel)
    {
        velocitiesX[index] = vel.x;
        velocitiesY[index] = vel.y;
    }

    // Reserve space for a specified number of elements
    void reserve(size_t size)
    {
        positionsX.reserve(size);
        positionsY.reserve(size);
        previousPositionsX.reserve(size);
        previousPositionsY.reserve(size);
        velocitiesX.reserve(size);
        velocitiesY.reserve(size);
        inverseMasses.reserve(size);
        radii.reserve(size);
        r.reserve(size);
        g.reserve(size);
        b.reserve(size);
        outlineWidths.reserve(size);
    }

    // Add a new circle
    void addCircle(
        const Vector2& position,
        const Vector2& velocity,
        float inverseMass,
        float radius,
        float red,
        float green,
        float blue,
        float outlineWidth
    )
    {
        positionsX.push_back(position.x);
        positionsY.push_back(position.y);
        previousPositionsX.push_back(position.x);
        previousPositionsY.push_back(position.y);
        velocitiesX.push_back(velocity.x);
        velocitiesY.push_back(velocity.y);
        inverseMasses.push_back(inverseMass);
        radii.push_back(radius);
        r.push_back(red);
        g.push_back(green);
        b.push_back(blue);
        outlineWidths.push_back(outlineWidth);

        ++m_circleCount;
    }

    int getCircleCount() const
    {
        return m_circleCount;
    }

private:
    // For the sake of cleanness, let's keep the common size of all the circle-data arrays here
    int m_circleCount = 0;
};

struct Collision
{
    // Indices of the two colliding circles
    int firstIndex;
    int secondIndex;

    // Collision normal
    Vector2 normal;

    // How much closer are the objects than their radii allow?
    float penetration = 0.f;
};

// Driver of the 2D physics simulation.
class Engine
{
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

    Engine(const Config& config);

    // Expand the world with new bounds
    void setWorldBounds(float worldBoundX, float worldBoundY);

    const CircleData& getCircleData()
    {
        return m_circleData;
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

    // SoA circle data structure
    CircleData m_circleData;

    // 2D grid structure to help with the broad phase collision detection
    SpatialGrid<int> m_spatialGrid;

    // The result from the last use of the spatial grid
    std::vector<std::pair<int, int>> m_potentialCollisionPairs;

    // Temporary container for all collisions detected during the current step
    std::vector<Collision> m_collisions;

    // World bounds
    float m_worldBoundX = 0.f;
    float m_worldBoundY = 0.f;

public:
    bool m_useSpatialPartitioning = true;
};

}