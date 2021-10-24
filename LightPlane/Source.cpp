#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h" // Camera class


using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Pencil_Plane"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 1400;
    const int WINDOW_HEIGHT = 800;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vaoBP;         // Handle for the vertex array object - Body of the pencil
        GLuint vboBP;         // Handle for the vertex buffer object - Body of the pencil
        GLuint nVertices;     // Number of indices of the mesh

        GLuint vaoNP;       // Handle for the vertex array object - Nib of the pencil
        GLuint vboNP;       // Handle for the vertex buffer object - Nib of the pencil

        GLuint vaoPL;       // Handle for the vertex array object - Plane of the scene
        GLuint vboPL;       //Handle for the vertex buffer object - PLane of the scene

        GLuint vaoKB;       // Handle for the vertex array object - Keyboard
        GLuint vboKB;       //Handle for the vertex buffer object - Keyboard
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    // Texture id
    GLuint gTextureIdBody;
    GLuint gTextureIdHead;
    GLuint gTextureIdPlane;
    GLuint gTextureIdKeyboard;
    GLuint gTextureIdPaper;
    GLuint gTextureIdNotebook;
    glm::vec2 gUVScale(1.0f, 1.0f);
    // Shader program
    GLuint gObjectsProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 2.0f, 17.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    //Light Color, Position, and Scale
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 gLightPosition(1.0f, 1.0f, 1.0f);
    glm::vec3 gLightScale(0.7f);

    //Object color
    glm::vec3 gObjectColor(1.0f, 0.2f, 0.0f);

}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Objects Vertex Shader Source Code*/
const GLchar* pyramidVertexShaderSource = GLSL(440,

layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Objects Fragment Shader Source Code*/
const GLchar* pyramidFragmentShaderSource = GLSL(440,

in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.9f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.9f; // Set specular light strength
    float highlightSize = 16.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);

/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

        //Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader program
    if (!UCreateShaderProgram(pyramidVertexShaderSource, pyramidFragmentShaderSource, gObjectsProgramId))
        return EXIT_FAILURE;
    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* texFilename = "resources/textures/pencilBody.jpg";
    if (!UCreateTexture(texFilename, gTextureIdBody))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "resources/textures/penNib.png";
    if (!UCreateTexture(texFilename, gTextureIdHead))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "resources/textures/Scene.png";
    if (!UCreateTexture(texFilename, gTextureIdPlane))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "resources/textures/keyboard.png";
    if (!UCreateTexture(texFilename, gTextureIdKeyboard))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "resources/textures/brownPaper.jpg";
    if (!UCreateTexture(texFilename, gTextureIdPaper))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "resources/textures/book.jpg";
    if (!UCreateTexture(texFilename, gTextureIdNotebook))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gObjectsProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gObjectsProgramId, "bodyTexture"), 0);
    glUniform1i(glGetUniformLocation(gObjectsProgramId, "headTexture"), 1);
    glUniform1i(glGetUniformLocation(gObjectsProgramId, "planeTexture"), 2);
    glUniform1i(glGetUniformLocation(gObjectsProgramId, "keyboardTexture"), 3);
    glUniform1i(glGetUniformLocation(gObjectsProgramId, "brownPaperTexture"), 4);
    glUniform1i(glGetUniformLocation(gObjectsProgramId, "notebookTexture"), 4);


    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release texture
    UDestroyTexture(gTextureIdBody);
    UDestroyTexture(gTextureIdHead);
    UDestroyTexture(gTextureIdPlane);
    UDestroyTexture(gTextureIdKeyboard);

    // Release shader program
    UDestroyShaderProgram(gObjectsProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UPWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWNWARD, gDeltaTime);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /// Pencil Part 1 - BODY
    ///----------------------
    glBindVertexArray(gMesh.vaoBP);
    glUseProgram(gObjectsProgramId);

    glm::mat4 scale = glm::scale(glm::vec3(0.5f, 3.0f, 0.5f));
    glm::mat4 rotation = glm::rotate(90.0f, glm::vec3(90.0, 10.0f, 0.0f));
    glm::mat4 translation = glm::translate(glm::vec3(5.0f, 0.0f, 1.0f));
    glm::mat4 model = translation * rotation * scale;

    glm::mat4 view = gCamera.GetViewMatrix();

    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gObjectsProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gObjectsProgramId, "view");
    GLint projLoc = glGetUniformLocation(gObjectsProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gObjectsProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gObjectsProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gObjectsProgramId, "lightPos");
    GLint lightPositionLoc2 = glGetUniformLocation(gObjectsProgramId, "lightPos2");
    GLint viewPositionLoc = glGetUniformLocation(gObjectsProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    
    GLint UVScaleLoc = glGetUniformLocation(gObjectsProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdBody);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    /// Pencil Part 2 - NIB
    ///--------------------
    glBindVertexArray(gMesh.vaoNP);
    glUseProgram(gObjectsProgramId);
        
    glm::mat4 scale2 = glm::scale(glm::vec3(0.25f, 0.5f, 0.25f));
    glm::mat4 rotation2 = glm::rotate(45.0f, glm::vec3(-95.0f, 0.0f, 30.0f));
    glm::mat4 translation2 = glm::translate(glm::vec3(4.6f, 0.9f, -0.8f));
    glm::mat4 model2 = translation2 * rotation2 * scale2;

    glm::mat4 view2 = gCamera.GetViewMatrix();

    glm::mat4 projection2 = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc2 = glGetUniformLocation(gObjectsProgramId, "model");
    GLint viewLoc2 = glGetUniformLocation(gObjectsProgramId, "view");
    GLint projLoc2 = glGetUniformLocation(gObjectsProgramId, "projection");

    glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, glm::value_ptr(model2));
    glUniformMatrix4fv(viewLoc2, 1, GL_FALSE, glm::value_ptr(view2));
    glUniformMatrix4fv(projLoc2, 1, GL_FALSE, glm::value_ptr(projection2));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdHead);

    glDrawArrays(GL_TRIANGLES, 0, 18);

    /// Plane
    ///---------
    glBindVertexArray(gMesh.vaoBP);
    glUseProgram(gObjectsProgramId);

    glm::mat4 scale3 = glm::scale(glm::vec3(13.0f, 10.0f, 0.5f));
    glm::mat4 rotation3 = glm::rotate(90.0f, glm::vec3(90.0f, 0.0f, 0.0f));
    glm::mat4 translation3 = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 model3 = translation3 * rotation3 * scale3;

    glm::mat4 view3 = gCamera.GetViewMatrix();

    glm::mat4 projection3 = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc3 = glGetUniformLocation(gObjectsProgramId, "model");
    GLint viewLoc3 = glGetUniformLocation(gObjectsProgramId, "view");
    GLint projLoc3 = glGetUniformLocation(gObjectsProgramId, "projection");

    glUniformMatrix4fv(modelLoc3, 1, GL_FALSE, glm::value_ptr(model3));
    glUniformMatrix4fv(viewLoc3, 1, GL_FALSE, glm::value_ptr(view3));
    glUniformMatrix4fv(projLoc3, 1, GL_FALSE, glm::value_ptr(projection3));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdPlane);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    /// Keyboard
    ///------------
    glBindVertexArray(gMesh.vaoKB);
    glUseProgram(gObjectsProgramId);

    glm::mat4 scale4 = glm::scale(glm::vec3(7.0f, 4.0f, 0.1f));
    glm::mat4 rotation4 = glm::rotate(90.0f, glm::vec3(90.0f, 0.0f, 0.0f));
    glm::mat4 translation4 = glm::translate(glm::vec3(-2.1f, 1.5f, -2.3f));
    glm::mat4 model4 = translation4 * rotation4 * scale4;

    glm::mat4 view4 = gCamera.GetViewMatrix();

    glm::mat4 projection4 = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc4 = glGetUniformLocation(gObjectsProgramId, "model");
    GLint viewLoc4 = glGetUniformLocation(gObjectsProgramId, "view");
    GLint projLoc4 = glGetUniformLocation(gObjectsProgramId, "projection");

    glUniformMatrix4fv(modelLoc4, 1, GL_FALSE, glm::value_ptr(model4));
    glUniformMatrix4fv(viewLoc4, 1, GL_FALSE, glm::value_ptr(view4));
    glUniformMatrix4fv(projLoc4, 1, GL_FALSE, glm::value_ptr(projection4));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdKeyboard);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    ///Brown Paper
    ///-----------
    glBindVertexArray(gMesh.vaoBP);
    glUseProgram(gObjectsProgramId);

    glm::mat4 scale6 = glm::scale(glm::vec3(2.0f, 3.5f, 0.1f));
    glm::mat4 rotation6 = glm::rotate(90.0f, glm::vec3(90.0f, -6.0f, 5.0f));
    glm::mat4 translation6 = glm::translate(glm::vec3(0.0f, -0.5f, 1.7f));
    glm::mat4 model6 = translation6 * rotation6 * scale6;

    glm::mat4 view6 = gCamera.GetViewMatrix();

    glm::mat4 projection6 = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    //// Retrieves and passes transform matrices to the Shader program
    GLint modelLoc6 = glGetUniformLocation(gObjectsProgramId, "model");
    GLint viewLoc6 = glGetUniformLocation(gObjectsProgramId, "view");
    GLint projLoc6 = glGetUniformLocation(gObjectsProgramId, "projection");

    glUniformMatrix4fv(modelLoc6, 1, GL_FALSE, glm::value_ptr(model6));
    glUniformMatrix4fv(viewLoc6, 1, GL_FALSE, glm::value_ptr(view6));
    glUniformMatrix4fv(projLoc6, 1, GL_FALSE, glm::value_ptr(projection6));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdPaper);

    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    /// Lined Paper
    ///-------------
    glBindVertexArray(gMesh.vaoBP);
    glUseProgram(gObjectsProgramId);

    glm::mat4 scale7 = glm::scale(glm::vec3(2.0f, 3.5f, 0.1f));
    glm::mat4 rotation7 = glm::rotate(90.0f, glm::vec3(90.0f, -6.0f, 5.0f));
    glm::mat4 translation7 = glm::translate(glm::vec3(0.5f, -0.3f, 1.5f));
    glm::mat4 model7 = translation7 * rotation7 * scale7;

    glm::mat4 view7 = gCamera.GetViewMatrix();

    glm::mat4 projection7 = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    //////// Retrieves and passes transform matrices to the Shader program
    GLint modelLoc7 = glGetUniformLocation(gObjectsProgramId, "model");
    GLint viewLoc7 = glGetUniformLocation(gObjectsProgramId, "view");
    GLint projLoc7 = glGetUniformLocation(gObjectsProgramId, "projection");

    glUniformMatrix4fv(modelLoc7, 1, GL_FALSE, glm::value_ptr(model7));
    glUniformMatrix4fv(viewLoc7, 1, GL_FALSE, glm::value_ptr(view7));
    glUniformMatrix4fv(projLoc7, 1, GL_FALSE, glm::value_ptr(projection7));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdNotebook);

    glDrawArrays(GL_TRIANGLES, 0, 36);


    /// Lamp
    ///---------
    glBindVertexArray(gMesh.vaoKB);
    glUseProgram(gLampProgramId);

    glm::mat4 scale5 = glm::scale(glm::vec3(1.5f, 1.5f, 1.5f));
    glm::mat4 rotation5 = glm::rotate(90.0f, glm::vec3(1.0, 1.0f, 1.0f));
    glm::mat4 translation5 = glm::translate(glm::vec3(0.0f, 7.0f, -6.0f));
    glm::mat4 model5 = translation5 * rotation5 * scale5;

    glm::mat4 view5 = gCamera.GetViewMatrix();

    glm::mat4 projection5 = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model5));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view5));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection5));

    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    // Vertex data
    GLfloat vertsBP[] = {
        //Positions          //Texture Coordinates
       -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
       -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
       -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

       -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
       -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
       -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

       -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
       -0.5f,  0.5f, -0.5f,  1.0f, 0.0f,
       -0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
       -0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
       -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
       -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  0.0f, 1.0f,

       -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
       -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
       -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

       -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
       -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
       -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    GLfloat vertsNP[] = {
        //Positions          //Texture Coordinates
        0.0f, 1.0f, 0.0f,       0.5f, 1.0f,
        -1.0f, -1.0f, 1.0f,     0.0f, 0.0f,
        1.0, -1.0f, 1.0f,       1.0f, 0.0f,

        0.0f, 1.0f, 0.0f,       0.5f, 1.0f,
        1.0f, -1.0f, 1.0f,      1.0f, 0.0f,
        1.0f, -1.0f, -1.0f,     0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,       0.5f, 1.0f,
        1.0f, -1.0f, -1.0f,     1.0f, 0.0f,
       -1.0f, -1.0f, -1.0f,     0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,       0.5f, 1.0f,
       -1.0f, -1.0f, -1.0f,     1.0f, 0.0f,
       -1.0f, -1.0f, 1.0f,      0.0f, 0.0f,

       -1.0f, -1.0f, 1.0f,      0.0f, 0.0f,
        1.0f, -1.0f, 1.0f,      1.0f, 0.0f,
       -1.0f, -1.0f, -1.0f,     0.5f, 1.0f,

       -1.0f, -1.0f, -1.0f,     1.0f, 0.0f,
        1.0f, -1.0f, -1.0f,     0.0f, 0.0f,
        1.0f, -1.0f, 1.0f,      0.5f, 1.0f,

    };
    GLfloat vertsPL[] = {
        -1.0f, -0.05f, -0.5f,         0.8f, 0.8f, 0.8f, 0.8f,
         1.0f, -0.05f, -0.5f,         0.8f, 0.8f, 0.8f, 0.8f,
         1.0f,  0.05f, -0.5f,         0.8f, 0.8f, 0.8f, 0.8f,
         1.0f,  0.05f, -0.5f,         0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f,  0.05f, -0.5f,         0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f, -0.05f, -0.5f,         0.8f, 0.8f, 0.8f, 0.8f,

        -1.0f, -0.05f,  0.5f,         0.8f, 0.8f, 0.8f, 0.8f,
         1.0f, -0.05f,  0.5f,         0.8f, 0.8f, 0.8f, 0.8f,
         1.0f,  0.05f,  0.5f,         0.8f, 0.8f, 0.8f, 0.8f,
         1.0f,  0.05f,  0.5f,         0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f,  0.05f,  0.5f,         0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f, -0.05f,  0.5f,         0.8f, 0.8f, 0.8f, 0.8f,

        -1.0f,  0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f,  0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f, -0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f, -0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f, -0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f,  0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,

         1.0f,  0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
         1.0f,  0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
         1.0f, -0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
         1.0f, -0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
         1.0f, -0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
         1.0f,  0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,

        -1.0f, -0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
         1.0f, -0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
         1.0f, -0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
         1.0f, -0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f, -0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f, -0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f,

        -1.0f,  0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
         1.0f,  0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
         1.0f,  0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
         1.0f,  0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f,  0.05f,  0.5f,        0.8f, 0.8f, 0.8f, 0.8f,
        -1.0f,  0.05f, -0.5f,        0.8f, 0.8f, 0.8f, 0.8f
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(vertsBP) / (sizeof(vertsBP[0]) * (floatsPerVertex + floatsPerUV));

    /// VAO and VBO for Pencil's Body
    glGenVertexArrays(1, &mesh.vaoBP);
    glBindVertexArray(mesh.vaoBP);

    glGenBuffers(1, &mesh.vboBP);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vboBP);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertsBP), vertsBP, GL_STATIC_DRAW);

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerUV);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);


    /// VAO and VBO for Pencil's Nib
    glGenVertexArrays(1, &mesh.vaoNP);
    glBindVertexArray(mesh.vaoNP);

    glGenBuffers(1, &mesh.vboNP);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vboNP); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertsNP), vertsNP, GL_STATIC_DRAW);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);


    /// VAO and VBO for Plane
    glGenVertexArrays(1, &mesh.vaoPL);
    glBindVertexArray(mesh.vaoPL);

    glGenBuffers(1, &mesh.vboPL);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vboPL); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertsPL), vertsPL, GL_STATIC_DRAW);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);

    // VAO and VBO for Keyboard
    glGenVertexArrays(1, &mesh.vaoKB);
    glBindVertexArray(mesh.vaoKB);

    glGenBuffers(1, &mesh.vboKB);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vboKB); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertsBP), vertsBP, GL_STATIC_DRAW);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vaoBP);
    glDeleteBuffers(1, &mesh.vboBP);
    glDeleteVertexArrays(1, &mesh.vaoNP);
    glDeleteBuffers(1, &mesh.vboNP);
    glDeleteVertexArrays(1, &mesh.vaoPL);
    glDeleteBuffers(1, &mesh.vboPL);
    glDeleteVertexArrays(1, &mesh.vaoKB);
    glDeleteBuffers(1, &mesh.vboKB);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
