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

        // Update position
        renderData.position += physicsData.velocity * deltaTime;
    }

    detectCollisions();
    resolveCollisions();
}

void Engine::detectCollisions()
{
    m_collisions.clear();

    // For all potential circle pairs
    for (int i = 0; i < m_circleCount; ++i)
    {
        CircleRenderData& first = m_circleRenderData[i];
        // Start by checking world boundaries
        if (first.position.x - first.radius < -m_worldBoundX) // Left wall
        {
            // Save as a collision
            const float penetration = -m_worldBoundX - (first.position.x - first.radius);
            m_collisions.push_back({
                m_circlePhysicsData[i],
                nullptr, // nullptr represents immovable objects
                Vector2(1.f, 0.f), // collision normal
                penetration // penetration
            });
        }
        else if (first.position.x + first.radius > m_worldBoundX) // Right wall
        {
            // Save as a collision
            const float penetration = (first.position.x + first.radius) - m_worldBoundX;
            m_collisions.push_back({
                m_circlePhysicsData[i],
                nullptr, // nullptr represents immovable objects
                Vector2(-1.f, 0.f), // collision normal
                penetration // penetration
            });
        }
        if (first.position.y - first.radius < -m_worldBoundY) // Floor
        {
            // Save as a collision
            const float penetration = -m_worldBoundY - (first.position.y - first.radius);
            m_collisions.push_back({
                m_circlePhysicsData[i],
                nullptr, // nullptr represents immovable objects
                Vector2(0.f, 1.f), // collision normal
                penetration // penetration
            });
        }
        else if (first.position.y + first.radius > m_worldBoundY) // Ceiling
        {
            // Save as a collision
            const float penetration = (first.position.y + first.radius) - m_worldBoundY;
            m_collisions.push_back({
                m_circlePhysicsData[i],
                nullptr, // nullptr represents immovable objects
                Vector2(0.f, -1.f), // collision normal
                penetration // penetration
            });
        }


        for (int j = i + 1; j < m_circleCount; ++j)
        {
            // TODO Broad phase (AABB check)
            //if (AABBsOverlap(circles[i].aabb, circles[j].aabb))
            {
                // Narrow phase if broad phase passes
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
                        &m_circlePhysicsData[j],
                        difference.normalized(),
                        difference.length()
                    });
                }
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
    if (collision.second)
    {
        CirclePhysicsData& first = collision.first;
        CirclePhysicsData& second = *collision.second;

            // Compute relative velocity
        Vector2 relativeVelocity = second.velocity - first.velocity;

        // Compute the relative velocity along the collision normal
        float velocityAlongNormal = relativeVelocity.dot(collision.normal);

        // If objects are separating, no need to resolve
        if (velocityAlongNormal > 0)
        {
            return;
        }

        // Compute restitution (bounciness factor, 1 for perfectly elastic)
        float restitution = 1.0f; // Change this based on your physics needs

        float mass = 1.0f;
        // Compute impulse scalar
        float inverseMass1 = 1.0f / mass;
        float inverseMass2 = 1.0f / mass;
        float impulseMagnitude = -(1.0f + restitution) * velocityAlongNormal / (inverseMass1 + inverseMass2);

        // Compute impulse vector
        Vector2 impulse = collision.normal * impulseMagnitude;

        // Apply impulse to velocities
        first.velocity -= impulse * inverseMass1;
        second.velocity += impulse * inverseMass2;

        Vector2 correction = collision.normal * collision.penetration;
        //first.renderData.position -= correction;
        //second.renderData.position += correction;

        {
            // Debug output
            //std::cout << std::fixed << std::setprecision(5) << "Normal: " << collision.normal.x << "," << collision.normal.y << " Penetration: " << collision.penetration << " Correction: " << correction.x << "," << correction.y << std::endl;
        }
    }
    else // second object is immovable, for example a wall
    {
        CirclePhysicsData& circle = collision.first;

        //std::cout << std::fixed << std::setprecision(5) << "Velocity before: " << circle.velocity.x << "," << circle.velocity.x << std::endl;

        circle.velocity = circle.velocity.reflect(collision.normal);

        //std::cout << std::fixed << std::setprecision(5) << "Velocity after: " << circle.velocity.x << "," << circle.velocity.x << std::endl;

        Vector2 correction = collision.normal * collision.penetration;
        circle.renderData.position += correction;
    }
}

void Engine::spawnCircles(double simulationTime)
{
    const int expectedSpawnCount = m_config.spawnRate > 0 ? std::min((int)(double)(m_config.spawnRate * simulationTime), m_config.spawnLimit) : m_config.spawnLimit;

    while (m_circleRenderData.size() < expectedSpawnCount)
    {
        const float radius = radiusDist(m_numberGenerator);

        m_circleRenderData.push_back({
            Vector2(xPosDist(m_numberGenerator), yPosDist(m_numberGenerator)),
            colorDist(m_numberGenerator),
            colorDist(m_numberGenerator),
            colorDist(m_numberGenerator),
            radius,
            2.f / radius / m_config.initialWindowHeight
        });

        m_circlePhysicsData.push_back({
            Vector2(velDist(m_numberGenerator), velDist(m_numberGenerator)),
            m_circleRenderData.back()
        });

        ++m_circleCount;
    }
}

}