#include <iostream>

#include "Renderer.h"
#include "Engine.h"

int main(int argc, char* argv[])
{
    // Default values
    float initialWindowWidth = 1024.f;
    float initialWindowHeight = 768.f;
    float minRadius = 5.f;
    float maxRadius = 50.f;
    int spawnLimit = 100;
    float gravity = 1.f; // intital world height / second^2
    float spawnRate = 0.f; // per second
    float restitution = 0.8f;
    bool outlineCircles = false; // whether to display the circles as outlined circles or filled disks
    float physicsFrequency = 60.f; // Hz; the frequency with which the physics will be stepped
    bool scalePhysics = true; // lower the physics frequency if the load is too intense?
    int correctionIterations = 4; // more -> better stability for objects resting on each other
    
    // Program arguments
    if (argc > 1) initialWindowWidth = (float)std::atof(argv[1]);
    if (argc > 2) initialWindowHeight = (float)std::atof(argv[2]);
    if (argc > 3) minRadius = (float)std::atof(argv[3]);
    if (argc > 4) maxRadius = (float)std::atof(argv[4]);
    if (argc > 5) spawnLimit = std::atoi(argv[5]);
    if (argc > 6) gravity = (float)std::atof(argv[6]);
    if (argc > 7) spawnRate = (float)std::atof(argv[7]);
    if (argc > 8) restitution = (float)std::atof(argv[8]);
    if (argc > 9) outlineCircles = (float)std::atoi(argv[9]);
    if (argc > 10) physicsFrequency = (float)std::atof(argv[10]);
    if (argc > 11) scalePhysics = (float)std::atoi(argv[11]);
    if (argc > 12) correctionIterations = std::atoi(argv[12]);

    // Some basic validation
    initialWindowWidth = std::max(initialWindowWidth, 100.f);
    initialWindowHeight = std::max(initialWindowHeight, 100.f);
    maxRadius = std::max(minRadius, maxRadius);
    physicsFrequency = std::max(physicsFrequency, 0.f);
    
    std::cout << "Starting simulation with:" << std::endl;
    std::cout << "Window size: " << (int)initialWindowWidth << "x" << (int)initialWindowHeight << std::endl;
    std::cout << "Radius range: " << minRadius << " to " << maxRadius << std::endl;
    std::cout << "Spawn limit: " << spawnLimit << std::endl;
    std::cout << "Gravity: " << gravity << std::endl;
    std::cout << "Restitution: " << restitution << std::endl;
    std::cout << "Outlined circles: " << outlineCircles << std::endl;
    std::cout << "Spawn rate: " << spawnRate << " circles / second" << std::endl;
    std::cout << "Physics-simulation frequency: " << physicsFrequency << " Hz" << std::endl;
    std::cout << "Scale physics: " << scalePhysics << std::endl;
    std::cout << "Correction iterations: " << correctionIterations << std::endl;

    try
    {
        // The engine will always operate in a space that is normalized over the initial window height
        CirclePhysics::Engine engine({
            minRadius / initialWindowHeight,
            maxRadius / initialWindowHeight,
            spawnLimit,
            gravity,
            restitution,
            initialWindowWidth / initialWindowHeight,
            initialWindowHeight,
            spawnRate,
            correctionIterations
        });

        CirclePhysics::Renderer renderer({
            initialWindowWidth,
            initialWindowHeight,
            outlineCircles,
            physicsFrequency,
            scalePhysics
        }, engine);

        renderer.initialize();
        renderer.run();
        renderer.cleanUp();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    glfwTerminate();
    
    return 0;
}