# Compiler and flags
CXX = g++
CXXFLAGS = -O3 -march=native -mavx2 -ffast-math -flto -pthread -std=c++11
LDFLAGS = -lGLEW -lglfw -lGL

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
EXECUTABLE = $(BIN_DIR)/circle_physics

# Default target
all: directories $(EXECUTABLE)

# Create necessary directories
directories:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BIN_DIR)

# Link the executable
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(CXXFLAGS) $(LDFLAGS) -o $@

# Compile objects
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $< $(CXXFLAGS) -o $@

# Debug build
debug: CXXFLAGS = -g -Wall -Wextra -DDEBUG -pthread -std=c++11
debug: all

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all directories clean debug