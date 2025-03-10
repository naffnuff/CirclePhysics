#include "Engine.h"

namespace CirclePhysics {

void Engine::initialize()
{
    m_circleRenderData.reserve(m_config.spawnLimit);
    m_circlePhysicsData.reserve(m_config.spawnLimit);
}

void Engine::step(double simulationTime, double deltaTime)
{
    spawnCircles(simulationTime);

    // Update circle positions for animation
    for (int i = 0; i < circleCount; ++i)
    {
        CircleRenderData& renderData = m_circleRenderData[i];
        CirclePhysicsData& physicsData = m_circlePhysicsData[i];

        // Update position
        renderData.x += physicsData.dx;
        renderData.y += physicsData.dy;

        if (renderData.x < -m_worldBoundX + renderData.radius)
        {
            renderData.x = -m_worldBoundX + renderData.radius;
            physicsData.dx = -physicsData.dx;
        }
        else if (renderData.x > m_worldBoundX - renderData.radius)
        {
            renderData.x = m_worldBoundX - renderData.radius;
            physicsData.dx = -physicsData.dx;
        }
        if (renderData.y < -m_worldBoundY + renderData.radius)
        {
            renderData.y = -m_worldBoundY + renderData.radius;
            physicsData.dy = -physicsData.dy;
        }
        else if (renderData.y > m_worldBoundY - renderData.radius)
        {
            renderData.y = m_worldBoundY - renderData.radius;
            physicsData.dy = -physicsData.dy;
        }
    }
}

void Engine::spawnCircles(double simulationTime)
{
    const int expectedSpawnCount = std::min((int)(double)(m_config.spawnRate * simulationTime), m_config.spawnLimit);

    while (m_circleRenderData.size() < expectedSpawnCount)
    {
        const float radius = radiusDist(m_numberGenerator);

        m_circleRenderData.push_back({
            xPosDist(m_numberGenerator),
            yPosDist(m_numberGenerator),
            colorDist(m_numberGenerator),
            colorDist(m_numberGenerator),
            colorDist(m_numberGenerator),
            radius,
            2.f / radius / m_config.initialWindowHeight
        });

        m_circlePhysicsData.push_back({
            velDist(m_numberGenerator),
            velDist(m_numberGenerator)
        });

        ++circleCount;
    }
}

}