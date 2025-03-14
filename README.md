# Circle Physics
A 2D physics simulator of circles.

## Build and Usage
Assumes you have a compiler capable of C++17.
### Windows
#### With `Visual Studio`
There is a VS 2022 solution in `vs2022`
### Linux (Ubuntu, Debian)
#### With `gcc` and `make`
1. Install basic build tools
```
sudo apt update
sudo apt install build-essential
```
2. Install `glew`
```
sudo apt install libglew-dev
```
2. Install `glfw`
```
sudo apt install libglfw3-dev
```
3. Build the program with make
```
make
```
4. Run the program
```
bin/circle_physics
```
5. Run the program with parameters, one extreme example:
```
bin/circle_physics 1920 1200 1 5 100000 0 0 0.8 0 60 1 1
```
### Program parameters
These command-line parameters will be read by the program, in order:
```
Parameter (default value) - explanation

Initial Window Width (1024)
Initial Window Height (768)
Minimum Radius (5)
Maximum Radius (50)
Spawn Limit (5000)
Gravity (1) - Unit: half intital world height / second^2
Spawn Rate (1) - Circles / second; 0 for everything at once
Restitution (0.8) - 1 for full bounciness; 0 for no bounciness
Outline Circles (0) - 1 to display the circles as outlined circles; 0 for filled disks
Physics Frequency (60) - Physics updates / second (Hz)
Scale Physics (1) - if 1, the physics frequency will be dynamically adjusted at high loads
Correction Iterations (4) - more -> better stability for objects resting on each other
```
