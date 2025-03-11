#include <iostream>

#include "Renderer.h"
#include "Engine.h"

int main(int argc, char* argv[])
{
    // Default values
    float initialWindowWidth = 1920.f;
    float initialWindowHeight = 1080.f;
    float minRadius = 10.f;
    float maxRadius = 30.f;
    int spawnLimit = 5000;
    float gravity = 98.1f;
    bool outlineCircles = true;
    float spawnRate = 1.f;
    
    if (argc > 1) initialWindowWidth = (float)std::atof(argv[1]);
    if (argc > 2) initialWindowHeight = (float)std::atof(argv[2]);
    if (argc > 3) minRadius = (float)std::atof(argv[3]);
    if (argc > 4) maxRadius = (float)std::atof(argv[4]);
    if (argc > 5) spawnLimit = std::atoi(argv[5]);
    if (argc > 6) gravity = (float)std::atof(argv[6]);
    if (argc > 7) outlineCircles = (float)std::atoi(argv[7]);
    if (argc > 8) spawnRate = (float)std::atof(argv[8]);
    
    if (initialWindowWidth <= 0) initialWindowWidth = 800;
    if (initialWindowHeight <= 0) initialWindowHeight = 600;
    if (minRadius <= 0) minRadius = 10.0f;
    if (maxRadius < minRadius) maxRadius = minRadius * 2;
    if (spawnLimit <= 0) spawnLimit = 500;
    
    std::cout << "Starting simulation with:" << std::endl;
    std::cout << "Window size: " << initialWindowWidth << "x" << initialWindowHeight << std::endl;
    std::cout << "Radius range: " << minRadius << " to " << maxRadius << std::endl;
    std::cout << "Spawn limit: " << spawnLimit << std::endl;
    std::cout << "Gravity: " << gravity << std::endl;
    std::cout << "Outlined circles: " << outlineCircles << std::endl;
    std::cout << "Spawn rate: " << spawnRate << " circles / second" << std::endl;

    try
    {
        // The engine will always operate in a space that is normalized over the initial window height
        CirclePhysics::Engine engine({ minRadius / initialWindowHeight, maxRadius / initialWindowHeight, spawnLimit, gravity, initialWindowWidth / initialWindowHeight, initialWindowHeight, spawnRate });

        CirclePhysics::Renderer renderer({ initialWindowWidth, initialWindowHeight, outlineCircles }, engine);
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