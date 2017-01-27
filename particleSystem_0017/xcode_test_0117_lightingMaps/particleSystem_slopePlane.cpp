// Std. Includes
#include <string>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GL includes
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

// GLM Mathemtics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Other Libs
#include <SOIL.h>
#include <iostream>

struct Particle{
    glm::vec3 position;
    glm::vec3 velocity;
    float mass;
    glm::vec3 force;

};

// Properties
GLuint screenWidth = 800, screenHeight = 600;

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void Do_Movement();
GLuint loadTexture(GLchar* path);

// Camera
Camera camera(glm::vec3(0.0f, 1.0f, 5.0f));
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
GLfloat currentTime = 0.0f;
GLfloat runningTime = 0.0f;
GLfloat n = 0; //The parameter to decide how elastic is the ball
const int particleNum = 10; //The number of the particles
GLint epsilon = 0;

//options
GLboolean blinn = false;

//Global variables
GLuint floorTexture;
GLuint planeVAO;

// The MAIN function, from here we start our application and run our Game loop
int main()
{
    // Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "LearnOpenGL", nullptr, nullptr); // Windowed
    glfwMakeContextCurrent(window);

    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Options
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLEW to setup the OpenGL Function pointers
    glewExperimental = GL_TRUE;
    glewInit();

    // Define the viewport dimensions
    //glViewport(0, 0, screenWidth, screenHeight);

    // Setup some OpenGL options
    glEnable(GL_DEPTH_TEST);

    // Setup and compile our shaders
    Shader shader("particle.vs", "particle.frag");
    Shader lightingShader("advanced_lighting.vs", "advanced_lighting.frag");

    // Load models
    Model particleModel("earth/earth.obj");


    GLfloat planeVertices[] = {
        // Positions            // Normals           // Texture Coords
        25.0f, -0.5f,  25.0f,  0.0f,  1.0f,  0.0f,  25.0f, 0.0f,
        -25.0f, -0.5f, -25.0f,  0.0f,  1.0f,  0.0f,  0.0f,  25.0f,
        -25.0f, -0.5f,  25.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,

        25.0f, -0.5f,  25.0f,  0.0f,  1.0f,  0.0f,  25.0f, 0.0f,
        25.0f, -0.5f, -25.0f,  0.0f,  1.0f,  0.0f,  25.0f, 25.0f,
        -25.0f, -0.5f, -25.0f,  0.0f,  1.0f,  0.0f,  0.0f,  25.0f
    };

    //Setup plane VAO
    GLuint planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glBindVertexArray(0);

    //Light source
    glm::vec3 lightPos(0.0f, 10.0f, 0.0f);

    //Load textures
    floorTexture = loadTexture("wood.png");

    Particle particles[particleNum * particleNum * particleNum * 3];
    for(int i=0; i<=particleNum * 1.5; i++){
        for(int j=0; j<=particleNum; j++){
            for(int k=0; k<=particleNum; k++){
                Particle& p = particles[((i*particleNum) + j)*particleNum + k];
                p.mass = 2;
                p.position = glm::vec3(0.0f - (float(i)/8)+ (rand()%100)/100.0, 3.3f + (rand()%20)/100.0, 0.0f + (float(k)/20.0));
                p.velocity = glm::vec3(0.5f, 0.0f, 0.0f);
                p.force = glm::vec3(0.0f, 0.0f, 0.0f);
            }
        }
    }


    //Add the gravity
    glm::vec3 gravity = glm::vec3(0.0f, -4.0f, 0.0f);


    // Game loop
    while(!glfwWindowShouldClose(window))
    {
        // Set frame time
        GLfloat currentFrame = glfwGetTime();
//        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        deltaTime = 0.02;

        runningTime += deltaTime;

        GLfloat time = glfwGetTime();
        glm::vec3 a = glm::vec3(0.0f, 0.0f, 0.0f);

        // Check and call events
        glfwPollEvents();
        Do_Movement();

        // Clear the colorbuffer
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        shader.Use();   // <-- Don't forget this one!
        // Transformation matrices
        glm::mat4 projection = glm::perspective(camera.Zoom, (float)screenWidth/(float)screenHeight, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

        for(int i=0; i<=particleNum * 1.5; i++){
            for(int j=0; j<=particleNum; j++){
                for(int k=0; k<=particleNum; k++){
                    Particle& p = particles[((i*particleNum) + j)*particleNum + k];

                    if(p.position.x < 1.3f){
                        if(p.position.y <= 3.0f && glm::dot(p.velocity,glm::vec3(0.0f, 1.0f, 0.0f)) < epsilon){
                            n = 150 * abs(float(p.velocity.y)) * (rand()%10/10.0);
                            glm::vec3 elasticity = glm::vec3(0.0f, n, 0.0f);
                            p.force = gravity + elasticity;
                            a = p.force / p.mass;
                            p.velocity += a * deltaTime;
                            p.position.x += p.velocity.x * deltaTime;
                            p.position.y = 3.0f + p.velocity.y * deltaTime;
                            p.position.z += p.velocity.z * deltaTime;
                        }else{
                            p.force = gravity;
                            a = p.force / p.mass;
                            p.velocity += a * deltaTime;
                            p.position += p.velocity * deltaTime;
                        }
                    }else if(p.position.x > 1.7f && p.position.x < 2.5f){
                        if(p.position.y <= 2.4 - (p.position.x * 0.2) && glm::dot(p.velocity,glm::vec3(0.0f, 1.0f, 0.0f)) < epsilon){
                            n = 150 * abs(float(p.velocity.y)) * (rand()%10/10.0);
                            float m = abs(float(p.velocity.y)) * (rand()%10/10.0);
                            glm::vec3 elasticity = glm::vec3(m, n, 0.0f);
                            p.force = gravity + elasticity;
                            a = p.force / p.mass;
                            p.velocity += a * deltaTime;
                            p.position.x += p.velocity.x * deltaTime;
                            p.position.y = 2.4 - (p.position.x * 0.2) + p.velocity.y * deltaTime;
                            p.position.z += p.velocity.z * deltaTime;
                        }else{
                            p.force = gravity;
                            a = p.force / p.mass;
                            p.velocity += a * deltaTime;
                            p.position += p.velocity * deltaTime;
                        }

                    }else{
                        if(p.position.y <= -0.45f && glm::dot(p.velocity,glm::vec3(0.0f, 1.0f, 0.0f)) < epsilon){
                            n = 150 * abs(float(p.velocity.y)) * (rand()%10/10.0);
                            glm::vec3 elasticity = glm::vec3(0.0f, n, 0.0f);
                            p.force = gravity + elasticity;
                            a = p.force / p.mass;
                            p.velocity += a * deltaTime;
                            p.position.x += p.velocity.x * deltaTime;
                            p.position.y = -0.45f + p.velocity.y * deltaTime;
                            p.position.z += p.velocity.z * deltaTime;
                        }else{
                            p.force = gravity;
                            a = p.force / p.mass;
                            p.velocity += a * deltaTime;
                            p.position += p.velocity * deltaTime;

                        }

                    }

                    if(p.position.x > 4.0f){
                        p.position = glm::vec3(0.0f + (rand()%100)/1000.0, 3.3f+ (rand()%20)/100.0, 0.0f + (float(k)/20.0));
                        p.velocity = glm::vec3(0.5f + (rand()%100)/1000.0, 0.0f, 0.0f);
                    }

                    glm::mat4 model;
                    model = glm::mat4();
                    model = glm::translate(model, glm::vec3(p.position.x, p.position.y, p.position.z));
                    model = glm::scale(model, glm::vec3(0.0001f, 0.0001f, 0.0001f));
                    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    particleModel.Draw(shader);

                    //            cout<<"deltaTime:" << deltaTime<<endl;

                }
            }
        }

        //Draw objects
        lightingShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        // Set light uniforms
        glUniform3fv(glGetUniformLocation(shader.Program, "lightPos"), 1, &lightPos[0]);
        glUniform3fv(glGetUniformLocation(shader.Program, "viewPos"), 1, &camera.Position[0]);
        glUniform1i(glGetUniformLocation(shader.Program, "blinn"), blinn);
        // Floor
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // Swap the buffers
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// This function loads a texture from file. Note: texture loading functions like these are usually
// managed by a 'Resource Manager' that manages all resources (like textures, models, audio).
// For learning purposes we'll just define it as a utility function.
GLuint loadTexture(GLchar* path)
{
    // Generate texture ID and load texture data
    GLuint textureID;
    glGenTextures(1, &textureID);
    int width,height;
    unsigned char* image = SOIL_load_image(path, &width, &height, 0, SOIL_LOAD_RGB);
    // Assign texture to ID
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Parameters
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    SOIL_free_image_data(image);
    return textureID;

}

#pragma region "User input"

// Moves/alters the camera positions based on user input
void Do_Movement()
{
    // Camera controls
    if(keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if(keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if(keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if(keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

#pragma endregion
