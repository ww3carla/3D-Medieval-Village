#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

#include <iostream>

enum RenderMode {
    SOLID,
    WIREFRAME,
    POINTS
};

RenderMode currentMode = SOLID;

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

GLint skyModelLoc;

// spotlight uniforms 
GLint spotPosLoc;
GLint spotDirLoc;
GLint spotCutOffLoc;
GLint spotOuterCutOffLoc;
GLint spotColorLoc;

//pointlight uniforms
GLint tavPosLoc, tavColorLoc, tavConstLoc, tavLinLoc, tavQuadLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 1.0f, 5.0f),
    glm::vec3(0.0f, 1.0f, 4.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.2f;

GLboolean pressedKeys[1024];

// for mouse
bool firstMouse = true;
float lastX = 512.0f;
float lastY = 384.0f;
float yaw = -90.0f;
float pitch = 0.0f;

// models
gps::Model3D towerModel;
gps::Model3D churchModel;
gps::Model3D castleModel;
gps::Model3D statuetModel;
gps::Model3D groundModel;
gps::Model3D skyModel;
gps::Model3D treeModel;
gps::Model3D buildingModel;
gps::Model3D house1Model;
gps::Model3D house2Model;
gps::Model3D house3Model;
gps::Model3D tavernModel;
GLfloat angle;

// shaders
gps::Shader myBasicShader;
gps::Shader skyShader;

//tree animation
float treeRotationAngle = 0.0f;
double lastTimeStamp = glfwGetTime();
float rotationSpeed = 30.0f;

//animation
bool cinematic = true;
double cinematicStartTime = 0.0;
float cinematicDuration = 18.0f;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
    glViewport(0, 0, width, height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_1) {
            currentMode = SOLID;
        }
        if (key == GLFW_KEY_2) {
            currentMode = WIREFRAME;
        }
        if (key == GLFW_KEY_3) {
            currentMode = POINTS;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    //TODO
    if (cinematic) return;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    myCamera.setCameraFront(glm::normalize(direction));

    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void startCinematicTour() {
    cinematic = true;
    cinematicStartTime = 0.0;
    firstMouse = true;
}

void processMovement() {
    bool moved = false;

    glm::vec3 front;
    front.x = -view[0][2];
    front.y = 0.0f;  
    front.z = -view[2][2];
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

    if (pressedKeys[GLFW_KEY_W]) {
        myCamera = gps::Camera(
            myCamera.getPosition() + front * cameraSpeed,
            myCamera.getPosition() + front * cameraSpeed + glm::vec3(front.x, 0.0f, front.z),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        moved = true;
    }

    if (pressedKeys[GLFW_KEY_S]) {
        myCamera = gps::Camera(
            myCamera.getPosition() - front * cameraSpeed,
            myCamera.getPosition() - front * cameraSpeed + glm::vec3(front.x, 0.0f, front.z),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        moved = true;
    }

    if (pressedKeys[GLFW_KEY_A]) {
        myCamera = gps::Camera(
            myCamera.getPosition() - right * cameraSpeed,
            myCamera.getPosition() - right * cameraSpeed + front,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        moved = true;
    }

    if (pressedKeys[GLFW_KEY_D]) {
        myCamera = gps::Camera(
            myCamera.getPosition() + right * cameraSpeed,
            myCamera.getPosition() + right * cameraSpeed + front,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        moved = true;
    }

    if (pressedKeys[GLFW_KEY_Q]) {
        myCamera.rotate(0.0f, -2.0f);
        moved = true;
    }

    if (pressedKeys[GLFW_KEY_E]) {
        myCamera.rotate(0.0f, 2.0f);
        moved = true;
    }

    if (pressedKeys[GLFW_KEY_T]) {
        if (cinematic) {
            cinematic = false;
            firstMouse = true;
        }
        else {
            startCinematicTour();
        }
    }

    if (moved) {
        view = myCamera.getViewMatrix();

        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        skyShader.useShaderProgram();
        GLint skyViewLoc = glGetUniformLocation(skyShader.shaderProgram, "view");
        glUniformMatrix4fv(skyViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);

    //glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    towerModel.LoadModel("objects/pisaTower.obj", "textures/tower/");
    churchModel.LoadModel("objects/church.obj", "textures/church/");
    castleModel.LoadModel("objects/castle.obj", "textures/castle/");
    statuetModel.LoadModel("objects/Grillparzer_C.obj", "textures/statuet/");
    groundModel.LoadModel("objects/ground.obj", "textures/ground/");
    skyModel.LoadModel("objects/skydome.obj", "textures/sky/");
    treeModel.LoadModel("objects/tree.obj", "textures/tree/");
    buildingModel.LoadModel("objects/3DModel.obj", "textures/building/");
    house1Model.LoadModel("objects/house1.obj", "textures/house1/");
    house2Model.LoadModel("objects/tavern2.obj", "textures/house2/");
    house3Model.LoadModel("objects/medievalHouse3.obj", "textures/house3/");
    tavernModel.LoadModel("objects/Tavern.obj", "textures/tavern/");
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
    skyShader.loadShader("shaders/sky.vert", "shaders/sky.frag");
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 500.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));


    skyShader.useShaderProgram();

    skyModelLoc = glGetUniformLocation(skyShader.shaderProgram, "model");
    GLint skyViewLoc = glGetUniformLocation(skyShader.shaderProgram, "view");
    GLint skyProjLoc = glGetUniformLocation(skyShader.shaderProgram, "projection");

    glUniformMatrix4fv(skyViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(skyProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    spotPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "bradSpotLight.position");
    spotDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "bradSpotLight.direction");
    spotCutOffLoc = glGetUniformLocation(myBasicShader.shaderProgram, "bradSpotLight.cutOff");
    spotOuterCutOffLoc = glGetUniformLocation(myBasicShader.shaderProgram, "bradSpotLight.outerCutOff");
    spotColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "bradSpotLight.color");

    tavPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "tavernLight.position");
    tavColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "tavernLight.color");
    tavConstLoc = glGetUniformLocation(myBasicShader.shaderProgram, "tavernLight.constant");
    tavLinLoc = glGetUniformLocation(myBasicShader.shaderProgram, "tavernLight.linear");
    tavQuadLoc = glGetUniformLocation(myBasicShader.shaderProgram, "tavernLight.quadratic");
}

GLuint ReadTextureFromFile(const char* file_name) {
    int x, y, n;
    int force_channels = 4;
    unsigned char* image_data = stbi_load(file_name, &x, &y, &n, force_channels);

    if (!image_data) {
        fprintf(stderr, "ERROR: could not load %s\n", file_name);
        return 0;
    }

    // NPOT check
    if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
        fprintf(
            stderr, "WARNING: texture %s is not power-of-2 dimensions\n", file_name
        );
    }

    int width_in_bytes = x * 4;
    unsigned char* top = NULL;
    unsigned char* bottom = NULL;
    unsigned char temp = 0;
    int half_height = y / 2;

    for (int row = 0; row < half_height; row++) {

        top = image_data + row * width_in_bytes;
        bottom = image_data + (y - row - 1) * width_in_bytes;

        for (int col = 0; col < width_in_bytes; col++) {

            temp = *top;
            *top = *bottom;
            *bottom = temp;
            top++;
            bottom++;
        }
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_SRGB, //GL_SRGB,//GL_RGBA,
        x,
        y,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image_data
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;

}

void updateTreeRotation() {
    double currentTimeStamp = glfwGetTime();
    double elapsedSeconds = currentTimeStamp - lastTimeStamp;
    lastTimeStamp = currentTimeStamp;

    treeRotationAngle += rotationSpeed * (float)elapsedSeconds;

    if (treeRotationAngle > 360.0f) {
        treeRotationAngle -= 360.0f;
    }
}

void renderScene() {

    // RENDER MODE
    switch (currentMode) {
    case SOLID:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;

    case WIREFRAME:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;

    case POINTS:
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glPointSize(3.0f);
        break;
    }

    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float groundLevelY = -3.0f;
    float statuetScale = 0.4f;
    float churchCastleScale = 0.6f;

    // SKYDOME
    skyShader.useShaderProgram();

    glDisable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT);
    glDisable(GL_CULL_FACE);

    model = glm::mat4(1.0f);
    model = glm::translate(model, myCamera.getPosition());
    model = glm::scale(model, glm::vec3(300.0f));

    glUniformMatrix4fv(skyModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    skyModel.Draw(skyShader);

    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    myBasicShader.useShaderProgram();

    // TREE SPOTLIGHT
    glm::vec3 treePosition = glm::vec3(50.0f, groundLevelY, -10.0f);

    glm::vec3 spotLightPosWorld = treePosition + glm::vec3(0.0f, 5.0f, 0.0f);
    glm::vec3 spotLightPosEye =
        glm::vec3(view * glm::vec4(spotLightPosWorld, 1.0f));

    glm::vec3 spotLightDirEye =
        glm::mat3(view) * glm::vec3(0.0f, -1.0f, 0.0f);

    glUniform3fv(spotPosLoc, 1, glm::value_ptr(spotLightPosEye));
    glUniform3fv(spotDirLoc, 1, glm::value_ptr(spotLightDirEye));
    glUniform1f(spotCutOffLoc, glm::cos(glm::radians(12.5f)));
    glUniform1f(spotOuterCutOffLoc, glm::cos(glm::radians(17.5f)));
    glUniform3fv(spotColorLoc, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.8f)));

    // TAVERN POINTLIGHT
    glm::vec3 tavernPos = glm::vec3(80.0f, groundLevelY, -5.0f);
    glm::vec3 tavernLightPosWorld = tavernPos + glm::vec3(-9.5f, 6.3f, 0.2f);
    glm::vec3 tavernLightPosEye = glm::vec3(view * glm::vec4(tavernLightPosWorld, 1.0f));


    glUniform3fv(tavPosLoc, 1, glm::value_ptr(tavernLightPosEye));
    glUniform3fv(tavColorLoc, 1, glm::value_ptr(glm::vec3(3.0f, 2.5f, 2.0f)));
    glUniform1f(tavConstLoc, 1.0f);
    glUniform1f(tavLinLoc, 0.09f);
    glUniform1f(tavQuadLoc, 0.032f);

    // GROUND
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, groundLevelY, 0.0f));
    model = glm::scale(model, glm::vec3(10.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    groundModel.Draw(myBasicShader);

    // BUILDINGS
    // CASTLE
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(40.0f, groundLevelY, -100.0f));
    model = glm::scale(model, glm::vec3(churchCastleScale * 1.5f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    castleModel.Draw(myBasicShader);

    // TOWER
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(10.0f, groundLevelY, -40.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    towerModel.Draw(myBasicShader);

    // CHURCH
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(80.0f, groundLevelY, -40.0f));
    model = glm::scale(model, glm::vec3(churchCastleScale));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    churchModel.Draw(myBasicShader);

    // STATUET
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(106.0f, groundLevelY, -45.0f));
    model = glm::scale(model, glm::vec3(statuetScale));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    statuetModel.Draw(myBasicShader);

    // TREE
    float treeScale = 2.0f;
    model = glm::mat4(1.0f);
    model = glm::translate(model, treePosition);
    model = glm::rotate(model, glm::radians(treeRotationAngle),
        glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(treeScale));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    treeModel.Draw(myBasicShader);

    // BUILDING
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-20.0f, groundLevelY, -40.0f));
    model = glm::scale(model, glm::vec3(0.7f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    buildingModel.Draw(myBasicShader);

    float villageScale = 1.5f;

    // HOUSE 1
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, groundLevelY, 5.0f));
    model = glm::scale(model, glm::vec3(villageScale));
    model = glm::rotate(model, glm::radians(10.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    house1Model.Draw(myBasicShader);

    // HOUSE 2
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(100.0f, groundLevelY, 5.0f));
    model = glm::scale(model, glm::vec3(villageScale));
    model = glm::rotate(model, glm::radians(-10.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    house2Model.Draw(myBasicShader);

    // HOUSE 3
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(20.0f, -4.0f, -5.0f));
    model = glm::scale(model, glm::vec3(villageScale));
    model = glm::rotate(model, glm::radians(3.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    house3Model.Draw(myBasicShader);

    // TAVERN
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(80.0f, groundLevelY, -5.0f));
    model = glm::scale(model, glm::vec3(villageScale));
    model = glm::rotate(model, glm::radians(-30.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    tavernModel.Draw(myBasicShader);
}

//for initial animation
static float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static float easeInOut(float t) {
    t = clamp01(t);
    return t * t * (3.0f - 2.0f * t); // smoothstep
}

static glm::vec3 lerpVec3(const glm::vec3& a, const glm::vec3& b, float t) {
    return a + t * (b - a);
}

void updateCinematicCamera() {
    if (!cinematic) return;

    double now = glfwGetTime();
    if (cinematicStartTime == 0.0)
        cinematicStartTime = now;

    float elapsed = (float)(now - cinematicStartTime);
    float t = elapsed / cinematicDuration;

    if (t >= 1.0f) {
        cinematic = false;
        return;
    }

    // Keyframes
    glm::vec3 p0(0.0f, 2.0f, 12.0f);
    glm::vec3 p1(10.0f, 3.0f, 0.0f);
    glm::vec3 p2(45.0f, 6.0f, -60.0f);
    glm::vec3 p3(80.0f, 4.0f, -30.0f);

    // Keyframes
    glm::vec3 l0(20.0f, -3.0f, -20.0f);
    glm::vec3 l1(50.0f, -3.0f, -10.0f);
    glm::vec3 l2(40.0f, -3.0f, -100.0f);
    glm::vec3 l3(80.0f, -3.0f, -40.0f);

    float seg = t * 3.0f;
    int i = (int)seg;           
    float localT = seg - (float)i;
    localT = easeInOut(localT);

    glm::vec3 camPos, lookAt;

    if (i == 0) {
        camPos = lerpVec3(p0, p1, localT);
        lookAt = lerpVec3(l0, l1, localT);
    }
    else if (i == 1) {
        camPos = lerpVec3(p1, p2, localT);
        lookAt = lerpVec3(l1, l2, localT);
    }
    else {
        camPos = lerpVec3(p2, p3, localT);
        lookAt = lerpVec3(l2, l3, localT);
    }

    view = glm::lookAt(camPos, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    skyShader.useShaderProgram();
    GLint skyViewLoc = glGetUniformLocation(skyShader.shaderProgram, "view");
    glUniformMatrix4fv(skyViewLoc, 1, GL_FALSE, glm::value_ptr(view));
}


void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
	initShaders();
	initUniforms();
    setWindowCallbacks();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        if (cinematic) {
            updateCinematicCamera();
        }
        else {
            processMovement();
        }

        updateTreeRotation();

        static double lastTime = glfwGetTime();
        double currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
