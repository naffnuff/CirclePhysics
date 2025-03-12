#include "Engine.h"

#include <iostream>
#include <iomanip>

namespace CirclePhysics {

void Engine::step(double simulationTime, double deltaTime)
{
    spawnCircles(simulationTime);

    // Update circle positions for animation
    for (int i = 0; i < m_circleCount; ++i)
    {
        CircleRenderData& renderData = m_circleRenderData[i];
        CirclePhysicsData& physicsData = m_circlePhysicsData[i];

        // Apply gravity
        if (physicsData.inverseMass > 0) // Don't apply to infinite mass objects
        {
            physicsData.velocity.y -= m_config.gravity * deltaTime;
        }

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
            // Save as a collision
            CirclePhysicsData& physics = m_circlePhysicsData[i];

            // Reflect the velocity
            physics.velocity.y = -physics.velocity.y * m_config.restitution;

            // Correct the position
            circle.position.y = -m_worldBoundY + circle.radius;
        }
        else if (circle.position.y + circle.radius > m_worldBoundY) // Ceiling
        {
            // Save as a collision
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
        resolveCollision(collision);
    }
}

void Engine::resolveCollision(const Collision& collision)
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

    // Compute impulse vector
    Vector2 impulse = collision.normal * impulseMagnitude;

    // Apply impulse to velocities
    first.velocity -= impulse * first.inverseMass;
    second.velocity += impulse * second.inverseMass;

    if (totalInverseMass > 0.f)
    {
        Vector2 correction = collision.normal * (collision.penetration / totalInverseMass);
        first.renderData.position -= correction * first.inverseMass;
        second.renderData.position += correction * second.inverseMass;
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
        //const float mass = radius * radius * density;
        const float mass = radius * radius * radius * density;

        m_circleRenderData.push_back({
            Vector2(spawnXDistribution(m_numberGenerator), spawnYDistribution(m_numberGenerator)),
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