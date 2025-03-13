#include "Engine.h"

#include <iostream>
#include <iomanip>

namespace CirclePhysics
{

Engine::Engine(const Config& config)
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

    // Determine number of threads (leave one core for main thread)
    int numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1);

    // Initialize per-thread collision vectors
    m_collisions.resize(numThreads);
    for (std::vector<Collision>& collisions : m_collisions)
    {
        collisions.reserve(config.spawnLimit);
    }

    // Start the worker threads
    for (int i = 0; i < numThreads; ++i)
    {
        m_threadPool.emplace_back(&Engine::workerThread, this, i);
    }

    std::cout << numThreads << " helper threads supported" << std::endl;

    // Reserve space for all circles
    m_circleData.reserve(config.spawnLimit);
}

Engine::~Engine()
{
    // Signal all threads to exit
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_terminateThreads = true;
    }

    // Wake up all threads
    m_condition.notify_all();

    // Wait for all threads to finish
    for (auto& thread : m_threadPool)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

void Engine::setWorldBounds(float worldBoundX, float worldBoundY)
{
    m_worldBoundX = worldBoundX;
    m_worldBoundY = worldBoundY;
}

int Engine::step(double simulationTime, double deltaTime)
{
    spawnCircles(simulationTime);

    // Store previous positions for interpolation
    for (int i = 0; i < m_circleData.getCircleCount(); ++i)
    {
        m_circleData.previousPositionsX[i] = m_circleData.positionsX[i];
        m_circleData.previousPositionsY[i] = m_circleData.positionsY[i];
    }

    // Apply gravity and update positions
    for (int i = 0; i < m_circleData.getCircleCount(); ++i)
    {
        // Apply gravity
        if (m_circleData.inverseMasses[i] > 0) // Don't apply to infinite mass objects
        {
            m_circleData.velocitiesY[i] -= (float)(m_config.gravity * deltaTime);
        }

        // Update position
        m_circleData.positionsX[i] += m_circleData.velocitiesX[i] * deltaTime;
        m_circleData.positionsY[i] += m_circleData.velocitiesY[i] * deltaTime;
    }

    resolveWallCollisions();
    detectCollisions();
    const int collisionChecks = m_potentialCollisionPairs.size();
    resolveCollisions();

    return (int)collisionChecks;
}

void Engine::resolveWallCollisions()
{
    // For all circles
    for (int i = 0; i < m_circleData.getCircleCount(); ++i)
    {
        const float x = m_circleData.positionsX[i];
        const float y = m_circleData.positionsY[i];
        const float radius = m_circleData.radii[i];

        // Checking each of the world boundaries
        if (x - radius < -m_worldBoundX) // Left wall
        {
            // Reflect the velocity
            m_circleData.velocitiesX[i] = -m_circleData.velocitiesX[i] * m_config.restitution;
            // Correct the position
            m_circleData.positionsX[i] = -m_worldBoundX + radius;
        }
        else if (x + radius > m_worldBoundX) // Right wall
        {
            // Reflect the velocity
            m_circleData.velocitiesX[i] = -m_circleData.velocitiesX[i] * m_config.restitution;
            // Correct the position
            m_circleData.positionsX[i] = m_worldBoundX - radius;
        }
        if (y - radius < -m_worldBoundY) // Floor
        {
            // Reflect the velocity
            m_circleData.velocitiesY[i] = -m_circleData.velocitiesY[i] * m_config.restitution;
            // Correct the position
            m_circleData.positionsY[i] = -m_worldBoundY + radius;
        }
        else if (y + radius > m_worldBoundY) // Ceiling
        {
            // Reflect the velocity
            m_circleData.velocitiesY[i] = -m_circleData.velocitiesY[i] * m_config.restitution;
            // Correct the position
            m_circleData.positionsY[i] = m_worldBoundY - radius;
        }
    }
}

void Engine::detectCollisions()
{
    // Clear collision vectors
    for (std::vector<Collision>& collisions : m_collisions)
    {
        collisions.clear();
    }

    if (m_useSpatialPartitioning)
    {
        // Clear and update spatial grid
        m_spatialGrid.updateDimensions(m_worldBoundX, m_worldBoundY);
        m_spatialGrid.clear();

        // Insert all circles into the grid
        for (int i = 0; i < m_circleData.getCircleCount(); ++i)
        {
            const Vector2 position(m_circleData.positionsX[i], m_circleData.positionsY[i]);
            m_spatialGrid.insert(i, position, m_circleData.radii[i]);
        }

        // Get potential collisions
        m_spatialGrid.getPotentialCollisions(m_potentialCollisionPairs);

        // Up to a certain point, not worth the threading overhead
        if (m_singleThreaded || m_potentialCollisionPairs.size() < 5000)
        {
            // Use sequential approach
            for (const std::pair<int, int>& pair : m_potentialCollisionPairs)
            {
                checkPotentialCollisionPair(pair.first, pair.second, m_collisions[0]);
            }
        }
        else
        {
            // Number of worker threads in our pool
            int numThreads = m_threadPool.size();

            // Calculate work division
            int totalPairs = m_potentialCollisionPairs.size();
            int pairsPerThread = (totalPairs + numThreads - 1) / numThreads;

            // Reset active threads counter
            m_activeThreads = 0;

            // Submit tasks to work queue
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);

                for (int threadId = 0; threadId < numThreads; ++threadId)
                {
                    int startIdx = threadId * pairsPerThread;
                    int endIdx = std::min((threadId + 1) * pairsPerThread, totalPairs);

                    if (startIdx >= endIdx) break;

                    // Create a task to process this batch
                    m_workQueue.push(
                        [this, threadId, startIdx, endIdx]()
                        {
                            std::vector<Collision>& localCollisions = m_collisions[threadId];

                            for (int i = startIdx; i < endIdx; ++i)
                            {
                                const std::pair<int, int>& pair = m_potentialCollisionPairs[i];

                                checkPotentialCollisionPair(pair.first, pair.second, localCollisions);
                            }
                        });
                }
            }

            // Notify worker threads
            m_condition.notify_all();

            // Wait for all tasks to complete
            while (m_activeThreads > 0 || !m_workQueue.empty())
            {
                std::this_thread::yield();
            }
        }
    }
    else
    {
        // For all potential circle pairs
        for (int i = 0; i < m_circleData.getCircleCount(); ++i)
        {
            const Vector2 firstPosition(m_circleData.positionsX[i], m_circleData.positionsY[i]);
            const float firstRadius = m_circleData.radii[i];

            for (int j = i + 1; j < m_circleData.getCircleCount(); ++j)
            {
                const Vector2 secondPosition(m_circleData.positionsX[j], m_circleData.positionsY[j]);
                const float secondRadius = m_circleData.radii[j];

                checkCollision(i, j, firstPosition, secondPosition, firstRadius, secondRadius, m_collisions[0]);
            }
        }
    }
}

void Engine::checkPotentialCollisionPair(int i, int j, std::vector<Collision>& result) const
{
    // Get positions and radii
    const Vector2 firstPosition(m_circleData.positionsX[i], m_circleData.positionsY[i]);
    const Vector2 secondPosition(m_circleData.positionsX[j], m_circleData.positionsY[j]);
    const float firstRadius = m_circleData.radii[i];
    const float secondRadius = m_circleData.radii[j];

    checkCollision(i, j, firstPosition, secondPosition, firstRadius, secondRadius, result);
}

void Engine::checkCollision(int i, int j, Vector2 firstPosition, Vector2 secondPosition, float firstRadius, float secondRadius, std::vector<Collision>& result) const
{
    // Finer collision detection with squared numbers for efficiency
    const float radii = firstRadius + secondRadius;
    const float radiiSquared = radii * radii;
    const Vector2 difference = secondPosition - firstPosition;
    const float distanceSquared = difference.lengthSquared();

    if (distanceSquared < radiiSquared)
    {
        // Save as a collision
        const float penetration = radii - difference.length();
        result.push_back({
            i, j,
            difference.normalized(),
            penetration
        });
    }
}

void Engine::resolveCollisions()
{
    for (const std::vector<Collision>& collisions : m_collisions)
    {
        for (const Collision& collision : collisions)
        {
            correctVelocities(collision);
        }
    }

    // Then apply position corrections in multiple iterations
    for (int i = 0; i < m_config.correctionIterations; ++i)
    {
        if (i > 0)
        {
            detectCollisions();
        }

        for (const std::vector<Collision>& collisions : m_collisions)
        {
            for (const Collision& collision : collisions)
            {
                correctPositions(collision);
            }
        }
    }
}

void Engine::correctVelocities(const Collision& collision)
{
    const int i = collision.firstIndex;
    const int j = collision.secondIndex;

    // Get velocities
    const Vector2 firstVelocity(m_circleData.velocitiesX[i], m_circleData.velocitiesY[i]);
    const Vector2 secondVelocity(m_circleData.velocitiesX[j], m_circleData.velocitiesY[j]);

    // Get inverse masses
    const float firstInverseMass = m_circleData.inverseMasses[i];
    const float secondInverseMass = m_circleData.inverseMasses[j];

    // Compute relative velocity
    const Vector2 relativeVelocity = secondVelocity - firstVelocity;

    // Compute the relative velocity along the collision normal
    const float velocityAlongNormal = relativeVelocity.dot(collision.normal);

    // If objects are separating, no need to resolve
    if (velocityAlongNormal > 0)
    {
        return;
    }

    const float totalInverseMass = firstInverseMass + secondInverseMass;
    const float impulseMagnitude = -(1.0f + m_config.restitution) * velocityAlongNormal / totalInverseMass;

    // Compute impulse vector
    const Vector2 impulse = collision.normal * impulseMagnitude;

    // Apply impulse to velocities
    m_circleData.velocitiesX[i] -= impulse.x * firstInverseMass;
    m_circleData.velocitiesY[i] -= impulse.y * firstInverseMass;
    m_circleData.velocitiesX[j] += impulse.x * secondInverseMass;
    m_circleData.velocitiesY[j] += impulse.y * secondInverseMass;
}

void Engine::correctPositions(const Collision& collision)
{
    const int i = collision.firstIndex;
    const int j = collision.secondIndex;

    // Get positions and inverse masses
    const float firstInverseMass = m_circleData.inverseMasses[i];
    const float secondInverseMass = m_circleData.inverseMasses[j];
    const float firstRadius = m_circleData.radii[i];
    const float secondRadius = m_circleData.radii[j];

    const float totalInverseMass = firstInverseMass + secondInverseMass;

    if (totalInverseMass <= 0)
    {
        return; // Both objects have infinite mass
    }

    const Vector2 correction = collision.normal * (collision.penetration / totalInverseMass);

    // The following is to ensure that the world boundaries are respected above all else

    // Apply X correction respecting constraints
    const float xCorrection = correction.x;
    if (xCorrection > 0.f)
    {
        const float firstPositionX = m_circleData.positionsX[i] - xCorrection * firstInverseMass;
        const float secondPositionX = m_circleData.positionsX[j] + xCorrection * secondInverseMass;

        if (firstPositionX - firstRadius < -m_worldBoundX)
        {
            // First object is constrained in negative X, put all correction on second
            m_circleData.positionsX[j] += xCorrection * totalInverseMass;
        }
        else if (secondPositionX + secondRadius > m_worldBoundX)
        {
            // Second object is constrained in positive X, put all correction on first
            m_circleData.positionsX[i] -= xCorrection * totalInverseMass;
        }
        else
        {
            // Neither or both constrained, apply normal correction
            m_circleData.positionsX[i] = firstPositionX;
            m_circleData.positionsX[j] = secondPositionX;
        }
    }
    else if (xCorrection < 0.f)
    {
        const float firstPositionX = m_circleData.positionsX[i] - xCorrection * firstInverseMass;
        const float secondPositionX = m_circleData.positionsX[j] + xCorrection * secondInverseMass;

        if (firstPositionX + firstRadius > m_worldBoundX)
        {
            // First object is constrained in positive X, put all correction on second
            m_circleData.positionsX[j] += xCorrection * totalInverseMass;
        }
        else if (secondPositionX - secondRadius < -m_worldBoundX)
        {
            // Second object is constrained in negative X, put all correction on first
            m_circleData.positionsX[i] -= xCorrection * totalInverseMass;
        }
        else
        {
            // Neither or both constrained, apply normal correction
            m_circleData.positionsX[i] = firstPositionX;
            m_circleData.positionsX[j] = secondPositionX;
        }
    }

    // Apply Y correction respecting constraints
    const float yCorrection = correction.y;
    if (yCorrection > 0.f)
    {
        const float firstPositionY = m_circleData.positionsY[i] - yCorrection * firstInverseMass;
        const float secondPositionY = m_circleData.positionsY[j] + yCorrection * secondInverseMass;

        if (firstPositionY - firstRadius < -m_worldBoundY)
        {
            // First object is constrained in negative Y, put all correction on second
            m_circleData.positionsY[j] += yCorrection * totalInverseMass;
        }
        else if (secondPositionY + secondRadius > m_worldBoundY)
        {
            // Second object is constrained in positive Y, put all correction on first
            m_circleData.positionsY[i] -= yCorrection * totalInverseMass;
        }
        else
        {
            // Neither or both constrained, apply normal correction
            m_circleData.positionsY[i] = firstPositionY;
            m_circleData.positionsY[j] = secondPositionY;
        }
    }
    else if (yCorrection < 0.f)
    {
        const float firstPositionY = m_circleData.positionsY[i] - yCorrection * firstInverseMass;
        const float secondPositionY = m_circleData.positionsY[j] + yCorrection * secondInverseMass;

        if (firstPositionY + firstRadius > m_worldBoundY)
        {
            // First object is constrained in positive Y, put all correction on second
            m_circleData.positionsY[j] += yCorrection * totalInverseMass;
        }
        else if (secondPositionY - secondRadius < -m_worldBoundY)
        {
            // Second object is constrained in negative Y, put all correction on first
            m_circleData.positionsY[i] -= yCorrection * totalInverseMass;
        }
        else
        {
            // Neither or both constrained, apply normal correction
            m_circleData.positionsY[i] = firstPositionY;
            m_circleData.positionsY[j] = secondPositionY;
        }
    }
}

void Engine::spawnCircles(double simulationTime)
{
    const int expectedSpawnCount = m_config.spawnRate > 0 ? std::min((int)(double)(m_config.spawnRate * simulationTime), m_config.spawnLimit) : m_config.spawnLimit;

    while (m_circleData.getCircleCount() < expectedSpawnCount)
    {
        const float radius = radiusDistribution(m_numberGenerator);

        const float density = 1.f;
        // PI can be excluded since there are no real-world units in this engine
        const float mass = radius * radius * density;
        const float inverseMass = mass == 0.f ? 0.f : 1.f / mass;

        const Vector2 position(spawnXDistribution(m_numberGenerator), spawnYDistribution(m_numberGenerator));
        const Vector2 velocity(velocityDistribution(m_numberGenerator), velocityDistribution(m_numberGenerator));

        // Add to combined data structure
        m_circleData.addCircle(
            position,
            velocity,
            inverseMass,
            radius,
            colorDistribution(m_numberGenerator),
            colorDistribution(m_numberGenerator),
            colorDistribution(m_numberGenerator),
            2.f / radius / m_config.initialWindowHeight
        );
    }
}

void Engine::workerThread(int threadId)
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            // Wait for work or termination signal
            m_condition.wait(lock,
                [this]()
                {
                    return !m_workQueue.empty() || m_terminateThreads;
                });

            // Check if we should exit
            if (m_terminateThreads && m_workQueue.empty())
            {
                return;
            }

            // Get next task
            task = std::move(m_workQueue.front());
            m_workQueue.pop();
        }

        // Execute the task
        m_activeThreads++;
        task();
        m_activeThreads--;
    }
}

}