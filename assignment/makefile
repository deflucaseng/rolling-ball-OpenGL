# Makefile for Sphere Renderer

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g

# Include paths
INCLUDES = -I/usr/include -I/usr/local/include

# Libraries
LIBS = -lGL -lGLEW -lglfw

# For macOS, uncomment these lines instead:
# INCLUDES = -I/usr/local/include -I/opt/homebrew/include
# LIBS = -framework OpenGL -L/usr/local/lib -L/opt/homebrew/lib -lGLEW -lglfw

# For Windows with MinGW, uncomment these lines instead:
# INCLUDES = -I/mingw64/include
# LIBS = -L/mingw64/lib -lopengl32 -lglew32 -lglfw3

# Target executable
TARGET = sphere_wireframe

# Source files
SRCS = sphere_wireframe.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)

# Run rule
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run