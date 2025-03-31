#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// Shader sources
const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec4 position;
    layout(location = 1) in vec3 color;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    out vec3 fragmentColor;
    
    void main() {
        gl_Position = projection * view * model * position;
        fragmentColor = color;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 fragmentColor;
    out vec4 color;
    
    void main() {
        color = vec4(fragmentColor, 1.0);
    }
)";

// Window dimensions
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 800;

// Structure to store a triangle
struct Triangle {
    glm::vec4 vertices[3];
};

// Global variables for animation
static float animationTime = 0.0f;
static const float SPEED = 2.0f; // Units per second
static const float RADIUS = 1.0f; // Assuming sphere radius of 1
static const glm::vec4 A(-4.0f, 1.0f, 4.0f, 1.0f);
static const glm::vec4 B(3.0f, 1.0f, -4.0f, 1.0f);
static const glm::vec4 C(-3.0f, 1.0f, -3.0f, 1.0f);
static const float distAB = glm::length(glm::vec3(B - A));
static const float distBC = glm::length(glm::vec3(C - B));
static const float distCA = glm::length(glm::vec3(A - C));
// Function prototypes
GLuint compileShader(GLenum type, const char* source);
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource);
std::vector<Triangle> readSphereFile(const std::string& filename);
void setupQuadrilateral(GLuint& VAO, GLuint& VBO);
void setupAxes(GLuint& VAO, GLuint& VBO);
void setupSphere(GLuint& VAO, GLuint& VBO, const std::vector<Triangle>& triangles);
void render(GLFWwindow* window, GLuint shaderProgram, GLuint quadVAO, GLuint axesVAO, GLuint sphereVAO, 
            int numTriangles);
void idleCallback();
glm::vec3 getPosition(float t, float& totalRotation);
glm::vec3 getRotationAxis(glm::vec3 direction);


int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Rolling Sphere", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.529f, 0.807f, 0.92f, 1.0f);

    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    
    std::string filename;
    std::cout << "Enter sphere file name: ";
    std::cin >> filename;
    std::vector<Triangle> triangles = readSphereFile(filename);
    
    GLuint quadVAO, quadVBO, axesVAO, axesVBO, sphereVAO, sphereVBO;
    setupQuadrilateral(quadVAO, quadVBO);
    setupAxes(axesVAO, axesVBO);
    setupSphere(sphereVAO, sphereVBO, triangles);
    
    // Animation timing
    double lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window)) {
        // Update animation time
        double currentTime = glfwGetTime();
        animationTime += (float)(currentTime - lastTime) * SPEED;
        lastTime = currentTime;
        
        render(window, shaderProgram, quadVAO, axesVAO, sphereVAO, triangles.size());
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }
    
    // [Cleanup code]
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &axesVAO);
    glDeleteBuffers(1, &axesVBO);
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteProgram(shaderProgram);
    
    glfwTerminate();
    return 0;
}

// Compile a shader
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    // Check for compilation errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
        return 0;
    }
    
    return shader;
}
glm::vec3 getPosition(float t, float& totalRotation) {
    // Calculate segment distances
    float distAB = glm::length(glm::vec3(B - A));
    float distBC = glm::length(glm::vec3(C - B));
    float distCA = glm::length(glm::vec3(A - C));
    float totalDist = distAB + distBC + distCA;
    
    // Normalize time to cycle
    float cycleTime = totalDist / SPEED;
    float normalizedTime = fmod(t, cycleTime);
    
    // Determine which segment we're in
    float timeAB = distAB / SPEED;
    float timeBC = distBC / SPEED;
    
    if (normalizedTime <= timeAB) {
        // A to B
        float progress = normalizedTime / timeAB;
        totalRotation = (distAB * progress) / RADIUS;
        return glm::vec3(A) + (glm::vec3(B - A) * progress);
    }
    else if (normalizedTime <= (timeAB + timeBC)) {
        // B to C
        float progress = (normalizedTime - timeAB) / timeBC;
        totalRotation = (distAB + distBC * progress) / RADIUS;
        return glm::vec3(B) + (glm::vec3(C - B) * progress);
    }
    else {
        // C to A
        float timeCA = distCA / SPEED;
        float progress = (normalizedTime - timeAB - timeBC) / timeCA;
        totalRotation = (distAB + distBC + distCA * progress) / RADIUS;
        return glm::vec3(C) + (glm::vec3(A - C) * progress);
    }
}
glm::vec3 getRotationAxis(glm::vec3 direction) {
    // Cross product with y-axis gives rotation axis perpendicular to direction
    return glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
}

// Create a shader program from vertex and fragment shaders
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    if (!vertexShader || !fragmentShader) {
        return 0;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    // Check for linking errors
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Shader program linking error: " << infoLog << std::endl;
        return 0;
    }
    
    // Cleanup
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

// Read sphere data from file
std::vector<Triangle> readSphereFile(const std::string& filename) {
    std::vector<Triangle> triangles;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return triangles;
    }
    int numTriangles;
    file >> numTriangles;
    std::cout << "Number of triangles: " << numTriangles << std::endl;
    for (int i = 0; i < numTriangles; ++i) {
        int n;
        file >> n;
        if (n != 3) {
            std::cerr << "Invalid polygon: expected 3 vertices, got " << n << std::endl;
            continue;
        }
        Triangle triangle;
        for (int j = 0; j < 3; ++j) {
            float x, y, z;
            file >> x >> y >> z;
            triangle.vertices[j] = glm::vec4(x, y, z, 1.0f);
            std::cout << "Vertex " << j << ": (" << x << ", " << y << ", " << z << ")" << std::endl;
        }
        triangles.push_back(triangle);
    }
    file.close();
    std::cout << "Total triangles loaded: " << triangles.size() << std::endl;
    return triangles;
}
// Setup the quadrilateral (x-z plane)
void setupQuadrilateral(GLuint& VAO, GLuint& VBO) {
    // Vertices for the quadrilateral with position and color
    float quadVertices[] = {
        // positions            // colors (green)
        5.0f, 0.0f, 8.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        5.0f, 0.0f, -4.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -5.0f, 0.0f, -4.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        
        5.0f, 0.0f, 8.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -5.0f, 0.0f, -4.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -5.0f, 0.0f, 8.0f, 1.0f,  0.0f, 1.0f, 0.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Setup the x, y, z axes
void setupAxes(GLuint& VAO, GLuint& VBO) {
    // Slightly elevate the axes above the ground to avoid z-fighting
    const float yOffset = 0.02f;
    
    // Vertices for the axes with position and color
    float axesVertices[] = {
        // x-axis (red)
        -10.0f, yOffset, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
        10.0f, yOffset, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
        
        // y-axis (magenta)
        0.0f, -10.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 10.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        
        // z-axis (blue)
        0.0f, yOffset, -10.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, yOffset, 10.0f, 1.0f, 0.0f, 0.0f, 1.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axesVertices), axesVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Setup the sphere
void setupSphere(GLuint& VAO, GLuint& VBO, const std::vector<Triangle>& triangles) {
    std::vector<float> vertices;
    float color[3] = {1.0f, 0.84f, 0.0f}; // Yellow sphere
    
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; ++i) {
            glm::vec4 vertex = triangle.vertices[i]; // No initial translation
            vertices.push_back(vertex.x);
            vertices.push_back(vertex.y);
            vertices.push_back(vertex.z);
            vertices.push_back(vertex.w);
            vertices.push_back(color[0]);
            vertices.push_back(color[1]);
            vertices.push_back(color[2]);
        }
    }
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
void render(GLFWwindow* window, GLuint shaderProgram, GLuint quadVAO, GLuint axesVAO, 
	GLuint sphereVAO, int numTriangles) {
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glUseProgram(shaderProgram);

GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

glm::mat4 view = glm::lookAt(
 glm::vec3(5.0f, 10.0f, 5.0f),
 glm::vec3(0.0f, 1.0f, 0.0f),
 glm::vec3(0.0f, 1.0f, 0.0f)
);
glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

glm::mat4 model = glm::mat4(1.0f);
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
glBindVertexArray(quadVAO);
glDrawArrays(GL_TRIANGLES, 0, 6);
glBindVertexArray(axesVAO);
glDrawArrays(GL_LINES, 0, 6);

float totalRotation;
glm::vec3 currentPos = getPosition(animationTime, totalRotation);
glm::vec3 direction;
float timeAB = distAB / SPEED;
float timeBC = distBC / SPEED;
float totalDist = distAB + distBC + distCA;
float normalizedTime = fmod(animationTime, totalDist / SPEED);

if (normalizedTime <= timeAB) {
 direction = glm::normalize(glm::vec3(B - A));
} else if (normalizedTime <= (timeAB + timeBC)) {
 direction = glm::normalize(glm::vec3(C - B));
} else {
 direction = glm::normalize(glm::vec3(A - C));
}

model = glm::mat4(1.0f);
glm::vec3 axis = getRotationAxis(direction);
model = glm::translate(model, currentPos);
model = glm::rotate(model, totalRotation, axis);

glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
glBindVertexArray(sphereVAO);
glDrawArrays(GL_TRIANGLES, 0, numTriangles * 3);

glBindVertexArray(0);
}