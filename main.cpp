#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include"myShader.h"
#include"myTexture.h"
#include"myCamera.h"
#include"myModel.h"

unsigned int VAO[2]; 
unsigned int VBO[2]; 
unsigned int EBO[2];
unsigned int TEX[2];
float screenWidth = 800, screenHeight = 600;
double lastX = 0, lastY = 0;
bool firstMouse = true;
myCamera* camera;

/*重构窗口大小*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    screenWidth = width;
    screenHeight = height;
    glViewport(0, 0, width, height);
}
/*键盘输入响应函数*/
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 0.003f; // adjust accordingly
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->cameraPos += cameraSpeed * camera->cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->cameraPos -= cameraSpeed * camera->cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->cameraPos -= glm::normalize(glm::cross(camera->cameraFront, camera->cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->cameraPos += glm::normalize(glm::cross(camera->cameraFront, camera->cameraUp)) * cameraSpeed;
}
/*鼠标事件回调函数*/
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        return;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    float sensitivity = 0.1;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    camera->cameraEuler.yaw += xoffset;
    camera->cameraEuler.pitch += yoffset;
    if (camera->cameraEuler.pitch > 89.0f)
        camera->cameraEuler.pitch = 89.0f;
    if (camera->cameraEuler.pitch < -89.0f)
        camera->cameraEuler.pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(camera->cameraEuler.yaw)) * cos(glm::radians(camera->cameraEuler.pitch));
    front.y = sin(glm::radians(camera->cameraEuler.pitch));
    front.z = sin(glm::radians(camera->cameraEuler.yaw)) * cos(glm::radians(camera->cameraEuler.pitch));
    camera->cameraFront = glm::normalize(front);
}
/*滚轮事件回调函数*/
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    float sensitivity = 0.1;
    if (camera->fov >= 1.0f && camera->fov <= 45.0f)
        camera->fov -= yoffset * sensitivity;
    if (camera->fov <= 1.0f)
        camera->fov = 1.0f;
    if (camera->fov >= 45.0f)
        camera->fov = 45.0f;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);                                //主版本：3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);                                //次版本：3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);                //设置为核心模式
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "LearnOpenGL", NULL, NULL);   //开启窗口
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);                                        //将窗口上下文绑定为当前线程的上下文
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);     //绑定窗口大小改变时调用的函数

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))               //初始化glad
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetScrollCallback(window, scroll_callback);
    glEnable(GL_DEPTH_TEST);

    myShader shader = myShader("shader/light.vsh", "shader/light.fsh");
    unsigned int shaderProgram = shader.ID;

    myMesh* mesh1 = new myMesh(glm::vec3(-4, 7, 0), glm::vec3(3, 3, 3), myTexture{loadTexture("Albedo1.jpg"),"diffuse_texture"});
    mesh1->Setup();
    myMesh* mesh2 = new myMesh(glm::vec3(4, 7, 0), glm::vec3(3, 3, 3), myTexture{ loadTexture("Albedo2.jpg"),"diffuse_texture" });
    mesh2->Setup();
    myModel* models = new myModel("D:/blender/941c2a7f-ea44-41aa-924a-e2ba967ab2b1.fbx", true);

    camera = new myCamera(glm::vec3(0.0f, 4.0f, 10.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), Euler{ 0.0f,-90.0f,0.0f }, 45.0f);
    while (!glfwWindowShouldClose(window))     //开始渲染循环
    {
        processInput(window);                  //自定义的检测键盘输入函数

        //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        shader.setVec3("dl[0].dir", glm::vec3(-1, -1, -1));
        shader.setVec3("dl[0].color", glm::vec3(1.0f, 1.0f, 1.0f));
        shader.setVec3("pl[0].pos", glm::vec3(0.3f, 5.0f, 3.0f));
        shader.setVec3("pl[0].color", glm::vec3(5.0f, 0.0f, 0.0f));
        shader.setFloat("pl[0].constant", 1.0f);
        shader.setFloat("pl[0].linear", 0.5f);
        shader.setFloat("pl[0].quadratic", 0.3f);

        shader.setVec3("cameraPos", camera->cameraPos);

        glm::mat4 model = glm::identity<glm::mat4>();
        model = glm::rotate(model, glm::radians(-0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 view = camera->getView();
        glm::mat4 projection = glm::identity<glm::mat4>();
        projection = glm::perspective(camera->fov, screenWidth / screenHeight, 0.1f, 100.0f);
        shader.setMatrix("model", model);
        shader.setMatrix("view", view);
        shader.setMatrix("projection", projection); 

        shader.setVec3("material.ambient", glm::vec3(0.3f, 0.3f, 0.3f));
        shader.setVec3("material.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
        shader.setVec3("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
        shader.setFloat("material.shininess_n", 64.0f);

        mesh1->Draw(shader);
        mesh2->Draw(shader);
        models->Draw(shader);

        glfwSwapBuffers(window);               //双缓冲，交换前后buffer
        glfwPollEvents();                      //检查事件队列中是否有事件传达
    }
    glDeleteVertexArrays(2, VAO);
    glDeleteBuffers(2, VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();                           //结束线程，释放资源
    return 0;
}