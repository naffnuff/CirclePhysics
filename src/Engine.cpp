#include "Engine.h"

#include <iostream>
#include <iomanip>

namespace CirclePhysics {

void Engine::step(double simulationTime, double deltaTime)
{
    spawnCircles(simulationTime);

    for (int i = 0; i < m_circleCount; ++i)
    {
        m_circleRenderData[i].previousPosition = m_circleRenderData[i].position;
    }

    // Update circle positions for animation
    for (int i = 0; i < m_circleCount; ++i)
    {
        CircleRenderData& renderData = m_circleRenderData[i];
        CirclePhysicsData& physicsData = m_circlePhysicsData[i];

        // Apply gravity
        if (physicsData.inverseMass > 0) // Don't apply to infinite mass objects
        {
            physicsData.velocity.y -= (float)(m_config.gravity * deltaTime);
        }

        // For interpolation in shader
        renderData.previousPosition = renderData.position;

        // Update position
        renderData.position += physicsData.velocity * deltaTime;
    }

    resolveWallCollisions();
    detectCollisions();
    resolveCollisions();
}

void Engine::resolveWallCollisions()
{
    // For all potential circle pairs
    for (int i = 0; i < m_circleCount; ++i)
    {
        CircleRenderData& circle = m_circleRenderData[i];

        // Checking each of the world boundaries
        if (circle.position.x - circle.radius < -m_worldBoundX) // Left wall
        {
            CirclePhysicsData& physics = m_circlePhysicsData[i];
            // Reflect the velocity
            physics.velocity.x = -physics.velocity.x * m_config.restitution;
            // Correct the position
            circle.position.x = -m_worldBoundX + circle.radius;
        }
        else if (circle.position.x + circle.radius > m_worldBoundX) // Right wall
        {
            CirclePhysicsData& physics = m_circlePhysicsData[i];
            // Reflect the velocity
            physics.velocity.x = -physics.velocity.x * m_config.restitution;
            // Correct the position
            circle.position.x = m_worldBoundX - circle.radius;
        }
        if (circle.position.y - circle.radius < -m_worldBoundY) // Floor
        {
            CirclePhysicsData& physics = m_circlePhysicsData[i];
            // Reflect the velocity
            physics.velocity.y = -physics.velocity.y * m_config.restitution;
            // Correct the position
            circle.position.y = -m_worldBoundY + circle.radius;
        }
        else if (circle.position.y + circle.radius > m_worldBoundY) // Ceiling
        {
            CirclePhysicsData& physics = m_circlePhysicsData[i];
            // Reflect the velocity
            physics.velocity.y = -physics.velocity.y * m_config.restitution;
            // Correct the position
            circle.position.y = m_worldBoundY - circle.radius;
        }
    }
}

void Engine::detectCollisions()
{
    m_collisions.clear();

    // For all potential circle pairs
    for (int i = 0; i < m_circleCount; ++i)
    {
        CircleRenderData& first = m_circleRenderData[i];

        for (int j = i + 1; j < m_circleCount; ++j)
        {
            CircleRenderData& second = m_circleRenderData[j];

            // Finer collision detection with squared numbers for efficiency
            const float radii = first.radius + second.radius;
            const float radiiSquared = radii * radii;
            const Vector2 difference = second.position - first.position;
            const float distanceSquared = difference.lengthSquared();

            if (distanceSquared < radiiSquared)
            {
                // Save as a collision
                const float penetration = radii - difference.length();
                m_collisions.push_back({
                    m_circlePhysicsData[i],
                    m_circlePhysicsData[j],
                    difference.normalized(),
                    penetration
                });
            }
        }
    }
}

void Engine::resolveCollisions()
{
    for (const Collision& collision : m_collisions)
    {
        correctVelocities(collision);
    }

    // Then apply position corrections in multiple iterations
    for (int i = 0; i < m_config.correctionIterations; ++i)
    {
        if (i > 0)
        {
            detectCollisions();
        }

        for (const Collision& collision : m_collisions)
        {
            correctPositions(collision);
        }
    }
}

void Engine::correctVelocities(const Collision& collision)
{
    CirclePhysicsData& first = collision.first;
    CirclePhysicsData& second = collision.second;

    // Compute relative velocity
    Vector2 relativeVelocity = second.velocity - first.velocity;

    // Compute the relative velocity along the collision normal
    float velocityAlongNormal = relativeVelocity.dot(collision.normal);

    // If objects are separating, no need to resolve
    if (velocityAlongNormal > 0)
    {
        return;
    }

    const float totalInverseMass = first.inverseMass + second.inverseMass;
    const float impulseMagnitude = -(1.0f + m_config.restitution) * velocityAlongNormal / totalInverseMass;

    // Compute impulse vector (scaled by resolution factor)
    Vector2 impulse = collision.normal * impulseMagnitude;

    // Apply impulse to velocities
    first.velocity -= impulse * first.inverseMass;
    second.velocity += impulse * second.inverseMass;
}

void Engine::correctPositions(const Collision& collision)
{
    CirclePhysicsData& first = collision.first;
    CirclePhysicsData& second = collision.second;

    const float totalInverseMass = first.inverseMass + second.inverseMass;

    Vector2 correction = collision.normal * (collision.penetration / totalInverseMass);

    // The following is to ensure that the world boundaries are respected above all else

    // Apply X correction respecting constraints
    float xCorrection = correction.x;
    if (xCorrection > 0.f)
    {
        const float firstPositionX = first.renderData.position.x - xCorrection * first.inverseMass;
        const float secondPositionX = second.renderData.position.x + xCorrection * second.inverseMass;

        if (firstPositionX - first.renderData.radius < -m_worldBoundX)
        {
            // First object is constrained in negative X, put all correction on second
            second.renderData.position.x += xCorrection * totalInverseMass;
        }
        else if (secondPositionX + second.renderData.radius > m_worldBoundX)
        {
            // Second object is constrained in positive X, put all correction on first
            first.renderData.position.x -= xCorrection * totalInverseMass;
        }
        else
        {
            // Neither or both constrained, apply normal correction
            first.renderData.position.x = firstPositionX;
            second.renderData.position.x = secondPositionX;
        }
    }
    else if (xCorrection < 0.f)
    {
        const float firstPositionX = first.renderData.position.x - xCorrection * first.inverseMass;
        const float secondPositionX = second.renderData.position.x + xCorrection * second.inverseMass;

        if (firstPositionX + first.renderData.radius > m_worldBoundX)
        {
            // First object is constrained in positive X, put all correction on second
            second.renderData.position.x += xCorrection * totalInverseMass;
        }
        else if (secondPositionX - second.renderData.radius < -m_worldBoundX)
        {
            // Second object is constrained in negative X, put all correction on first
            first.renderData.position.x -= xCorrection * totalInverseMass;
        }
        else
        {
            // Neither or both constrained, apply normal correction
            first.renderData.position.x = firstPositionX;
            second.renderData.position.x = secondPositionX;
        }
    }

    // Apply Y correction respecting constraints
    float yCorrection = correction.y;
    if (yCorrection > 0.f)
    {
        const float firstPositionY = first.renderData.position.y - yCorrection * first.inverseMass;
        const float secondPositionY = second.renderData.position.y + yCorrection * second.inverseMass;

        if (firstPositionY - first.renderData.radius < -m_worldBoundY)
        {
            // First object is constrained in negative Y, put all correction on second
            second.renderData.position.y += yCorrection * totalInverseMass;
        }
        else if (secondPositionY + second.renderData.radius > m_worldBoundY)
        {
            // Second object is constrained in positive Y, put all correction on first
            first.renderData.position.y -= yCorrection * totalInverseMass;
        }
        else
        {
            // Neither or both constrained, apply normal correction
            first.renderData.position.y = firstPositionY;
            second.renderData.position.y = secondPositionY;
        }
    }
    else if (yCorrection < 0.f)
    {
        const float firstPositionY = first.renderData.position.y - yCorrection * first.inverseMass;
        const float secondPositionY = second.renderData.position.y + yCorrection * second.inverseMass;

        if (firstPositionY + first.renderData.radius > m_worldBoundY)
        {
            // First object is constrained in positive Y, put all correction on second
            second.renderData.position.y += yCorrection * totalInverseMass;
        }
        else if (secondPositionY - second.renderData.radius < -m_worldBoundY)
        {
            // Second object is constrained in negative Y, put all correction on first
            first.renderData.position.y -= yCorrection * totalInverseMass;
        }
        else
        {
            // Neither or both constrained, apply normal correction
            first.renderData.position.y = firstPositionY;
            second.renderData.position.y = secondPositionY;
        }
    }
}

void Engine::spawnCircles(double simulationTime)
{
    const int expectedSpawnCount = m_config.spawnRate > 0 ? std::min((int)(double)(m_config.spawnRate * simulationTime), m_config.spawnLimit) : m_config.spawnLimit;

    while (m_circleRenderData.size() < expectedSpawnCount)
    {
        const float radius = radiusDistribution(m_numberGenerator);

        const float density = 1.f;
        // PI can be excluded since there are no real-world units in this engine
        const float mass = radius * radius * density;

        const Vector2 position(spawnXDistribution(m_numberGenerator), spawnYDistribution(m_numberGenerator));

        m_circleRenderData.push_back({
            position,
            position,
            colorDistribution(m_numberGenerator),
            colorDistribution(m_numberGenerator),
            colorDistribution(m_numberGenerator),
            radius,
            2.f / radius / m_config.initialWindowHeight
        });

        m_circlePhysicsData.push_back({
            Vector2(velocityDistribution(m_numberGenerator), velocityDistribution(m_numberGenerator)),
            mass == 0.f ? 0.f : 1.f / mass, // let mass = 0 mean infinite mass
            m_circleRenderData.back()
        });

        ++m_circleCount;
    }
}

}