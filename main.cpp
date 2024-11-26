#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


unsigned int VAO, VBO, EBO;
glm::vec3 robotPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 robotDirection = glm::vec3(0.0f, 0.0f, -1.0f);

float animationTime = 0.0f;
bool isWalking = false;


float robotRotation = 0.0f;
float robotTilt = 0.0f;

double lastMouseY = 400.0;
double lastMouseX = 500.0;       
bool firstMouse = true;           


bool isGreeting = false;
float greetingTime = 0.0f;


unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    }
    else {
        std::cerr << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    return textureID;
}



const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoord;

    uniform sampler2D texture1;

    void main() {
        FragColor = texture(texture1, TexCoord);
    }
)";

unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}


float cubeVertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f
};

unsigned int cubeIndices[] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
    0, 1, 5, 5, 4, 0, 
    2, 3, 7, 7, 6, 2, 
    0, 3, 7, 7, 4, 0, 
    1, 2, 6, 6, 5, 1  
};


void processInput(GLFWwindow* window) {
    isWalking = false;

    float speed = 0.0005f;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        robotPosition += robotDirection * speed;
        isWalking = true;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        robotPosition -= robotDirection * speed;
        isWalking = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm::vec3 leftDirection = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), robotDirection);
        robotPosition += leftDirection * speed;
        isWalking = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm::vec3 rightDirection = glm::cross(robotDirection, glm::vec3(0.0f, 1.0f, 0.0f));
        robotPosition += rightDirection * speed;
        isWalking = true;
    }

}


void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    static const float sensitivity = 0.2f;

    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastMouseX;
    float yOffset = lastMouseY - ypos;
    lastMouseX = xpos;
    lastMouseY = ypos;

    robotRotation -= xOffset * sensitivity;
    robotTilt += yOffset * sensitivity;

    if (robotTilt > 45.0f) robotTilt = 45.0f;
    if (robotTilt < -45.0f) robotTilt = -45.0f;
}



void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void createCube() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


}

void drawCube(glm::mat4 model, glm::vec3 position, glm::vec3 scale, unsigned int shaderProgram) {
    glUseProgram(shaderProgram);

    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}


int main() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWwindow* window = glfwCreateWindow(1000, 800, "BroBot", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouseCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, 1000, 800);
    glEnable(GL_DEPTH_TEST);

    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    createCube();

    unsigned int texture1 = loadTexture("texture.png");
    stbi_set_flip_vertically_on_load(true);
    unsigned int faceTexture = loadTexture("face.png");



    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

   
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1000.0f / 800.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));

    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");

    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glClearColor(0.0f, 0.6f, 0.0f, 1.0f);

        if (isWalking) {
            animationTime += 0.001f;
        }
        else {
            animationTime = 0.0f;
        }

        if (isGreeting) {
            greetingTime += 0.0001f;

            if (greetingTime > 2.0f) {
                isGreeting = false;
                greetingTime = 0.0f;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
            isGreeting = true;
            greetingTime += 0.001f;
        }
        else {
            isGreeting = false;
            greetingTime = 0.0f;
        }



        if (isWalking) {
            animationTime += 0.0005f;
        }
        else {
            animationTime = 0.0f;
        }


        float armAngle = sin(animationTime) * 30.0f;
        float legAngle = -sin(animationTime) * 30.0f;

        
        float angle = glm::radians(robotRotation);
        robotDirection.x = sin(angle); 
        robotDirection.z = cos(angle);



        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), robotPosition);
        model = glm::rotate(model, glm::radians(robotRotation), glm::vec3(0.0f, 1.0f, 0.0f));

        // Главата (с текстура лице)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, faceTexture);
        drawCube(model, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f), shaderProgram);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

  
        
        drawCube(model, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.5f, 0.5f), shaderProgram);

        
        glm::mat4 leftArmModel = glm::translate(model, glm::vec3(-0.6f, 0.5f, 0.0f));
        leftArmModel = glm::rotate(leftArmModel, glm::radians(armAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        leftArmModel = glm::translate(leftArmModel, glm::vec3(0.0f, -0.5f, 0.0f));
        drawCube(leftArmModel, glm::vec3(0.0f), glm::vec3(0.2f, 1.0f, 0.2f), shaderProgram);

        
        glm::mat4 rightLegModel = glm::translate(model, glm::vec3(0.3f, -0.5f, 0.0f));
        rightLegModel = glm::rotate(rightLegModel, glm::radians(legAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        rightLegModel = glm::translate(rightLegModel, glm::vec3(0.0f, -0.5f, 0.0f));
        drawCube(rightLegModel, glm::vec3(0.0f), glm::vec3(0.3f, 1.0f, 0.3f), shaderProgram);
       
        
        glm::mat4 rightArmModel = glm::translate(model, glm::vec3(0.6f, 0.5f, 0.0f));

        if (isGreeting) {
            float waveAngle = sin(greetingTime * 2.0f) * 15.0f;
            rightArmModel = glm::rotate(rightArmModel, glm::radians(-160.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            rightArmModel = glm::rotate(rightArmModel, glm::radians(waveAngle), glm::vec3(0.0f, 0.0f, 1.0f));
        }
        else {
            rightArmModel = glm::rotate(rightArmModel, glm::radians(-armAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        }

        rightArmModel = glm::translate(rightArmModel, glm::vec3(0.0f, -0.5f, 0.0f));
        drawCube(rightArmModel, glm::vec3(0.0f), glm::vec3(0.2f, 1.0f, 0.2f), shaderProgram);


        
        glm::mat4 leftLegModel = glm::translate(model, glm::vec3(-0.3f, -0.5f, 0.0f));
        leftLegModel = glm::rotate(leftLegModel, glm::radians(-legAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        leftLegModel = glm::translate(leftLegModel, glm::vec3(0.0f, -0.5f, 0.0f));
        drawCube(leftLegModel, glm::vec3(0.0f), glm::vec3(0.3f, 1.0f, 0.3f), shaderProgram);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

