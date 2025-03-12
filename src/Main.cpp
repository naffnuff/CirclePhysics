#include <iostream>

#include "Renderer.h"
#include "Engine.h"

int main(int argc, char* argv[])
{
    // Default values
    float initialWindowWidth = 1024.f;
    float initialWindowHeight = 768.f;
    float minRadius = 10.f;
    float maxRadius = 100.f;
    int spawnLimit = 10;
    float gravity = 0.f;
    bool outlineCircles = true;
    float spawnRate = 1.f;
    float physicsFrequency = 60.f;
    
    if (argc > 1) initialWindowWidth = (float)std::atof(argv[1]);
    if (argc > 2) initialWindowHeight = (float)std::atof(argv[2]);
    if (argc > 3) minRadius = (float)std::atof(argv[3]);
    if (argc > 4) maxRadius = (float)std::atof(argv[4]);
    if (argc > 5) spawnLimit = std::atoi(argv[5]);
    if (argc > 6) gravity = (float)std::atof(argv[6]);
    if (argc > 7) outlineCircles = (float)std::atoi(argv[7]);
    if (argc > 8) spawnRate = (float)std::atof(argv[8]);
    if (argc > 9) physicsFrequency = (float)std::atof(argv[9]);
    
    std::cout << "Starting simulation with:" << std::endl;
    std::cout << "Window size: " << (int)initialWindowWidth << "x" << (int)initialWindowHeight << std::endl;
    std::cout << "Radius range: " << minRadius << " to " << maxRadius << std::endl;
    std::cout << "Spawn limit: " << spawnLimit << std::endl;
    std::cout << "Gravity: " << gravity << std::endl;
    std::cout << "Outlined circles: " << outlineCircles << std::endl;
    std::cout << "Spawn rate: " << spawnRate << " circles / second" << std::endl;
    std::cout << "Physics-simulation frequency: " << physicsFrequency << " Hz" << std::endl;

    try
    {
        // The engine will always operate in a space that is normalized over the initial window height
        CirclePhysics::Engine engine({ minRadius / initialWindowHeight, maxRadius / initialWindowHeight, spawnLimit, gravity, initialWindowWidth / initialWindowHeight, initialWindowHeight, spawnRate });

        CirclePhysics::Renderer renderer({ initialWindowWidth, initialWindowHeight, outlineCircles, physicsFrequency }, engine);
        renderer.intialize();
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