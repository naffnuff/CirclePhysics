#include <iostream>

#include "Renderer.h"

int main(int argc, char* argv[]) {
    // Default values
    int initialWindowWidth = 1920;
    int initialWindowHeight = 1080;
    float minRadius = 10.0f;
    float maxRadius = 30.0f;
    int spawnLimit = 100000;
    float gravity = 98.1f;
    bool outlineCircles = true;
    
    // Parse command line arguments
    if (argc > 1) initialWindowWidth = std::atoi(argv[1]);
    if (argc > 2) initialWindowHeight = std::atoi(argv[2]);
    if (argc > 3) minRadius = (float)std::atof(argv[3]);
    if (argc > 4) maxRadius = (float)std::atof(argv[4]);
    if (argc > 5) spawnLimit = std::atoi(argv[5]);
    if (argc > 6) gravity = (float)std::atof(argv[6]);
    if (argc > 7) outlineCircles = (float)std::atoi(argv[7]);
    
    // Validate input
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

    try
    {
        CirclePhysics::Renderer renderer = CirclePhysics::Renderer((float)initialWindowWidth, (float)initialWindowHeight, minRadius, maxRadius, spawnLimit, gravity, outlineCircles);
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