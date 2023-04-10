#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include"myShader.h"
#include"myTexture.h"
#include"myCamera.h"
#include"myModel.h"

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

    float cameraSpeed = 0.05f; // adjust accordingly
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
    /*opengl窗口初始化*/
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);

    /*Shader管线创建*/
    myShader shader = myShader("shader/standard.vsh", "shader/standard.fsh", "shader/standard.gsh");
    myShader shader_post = myShader("shader/postprocess.vsh", "shader/postprocess.fsh");
    myShader shader_cubemap = myShader("shader/cubemap.vsh", "shader/cubemap.fsh");
    myShader shader_dir_light = myShader("shader/dir_light.vsh", "shader/dir_light.fsh");
    myShader shader_point_light = myShader("shader/point_light.vsh", "shader/point_light.fsh", "shader/point_light.gsh");

    /*模型设置*/
    myModel* models = new myModel("D:/blender/nanosuit/nanosuit2.obj", false);

    /*相机设置*/
    camera = new myCamera(glm::vec3(0.0f, 4.0f, 10.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), Euler{ 0.0f,-90.0f,0.0f }, 45.0f);

    /*cubemap设置*/
    vector<std::string> faces
    {
        "D:/cg_opengl/OpenglRenderer/resources/cubemap/right.jpg",
        "D:/cg_opengl/OpenglRenderer/resources/cubemap/left.jpg",
        "D:/cg_opengl/OpenglRenderer/resources/cubemap/top.jpg",
        "D:/cg_opengl/OpenglRenderer/resources/cubemap/bottom.jpg",
        "D:/cg_opengl/OpenglRenderer/resources/cubemap/front.jpg",
        "D:/cg_opengl/OpenglRenderer/resources/cubemap/back.jpg"
    };    
    /*天空盒的纹理模型设置*/
    unsigned int cubemapTexture = loadCubemap(faces);
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    /*屏幕的纹理模型设置*/
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    /*帧缓冲设置*/
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    /*帧纹理设置，将帧缓冲的颜色信息写入*/
    unsigned int textureColorbuffer;
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    /*渲染缓冲设置，接收深度值和模板值*/
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    /*平行光源阴影缓冲设置*/
    GLuint depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    /*平行光源阴影纹理设置*/
    const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    GLuint depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /*点光源阴影缓冲设置*/
    GLuint depthMapFBO2;
    glGenFramebuffers(1, &depthMapFBO2);
    /*点光源阴影纹理设置*/
    GLuint depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (GLuint i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
            SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO2);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glm::vec3 lightPos = glm::vec3(0, 8, 8);
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        /////////////////////////这里开始平行光源视角渲染
        glEnable(GL_DEPTH_TEST);
        shader_dir_light.use();
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        GLfloat near_plane = 1.0f, far_plane = 500.0f;
        glm::mat4 lightProjection = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(glm::vec3(40.0f, 40.0f, 40.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;
        glm::mat4 model = glm::identity<glm::mat4>();
        shader_dir_light.setMatrix("lightSpaceMatrix", lightSpaceMatrix);
        shader_dir_light.setMatrix("model", model);
        models->Draw(shader_dir_light);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        /////////////////////////这里开始点光源视角渲染
        shader_point_light.use();
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO2);
        glClear(GL_DEPTH_BUFFER_BIT);
        GLfloat aspect = (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT;
        GLfloat near = 1.0f;
        GLfloat far = 50.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));
        for (GLuint i = 0; i < 6; ++i)
            shader_point_light.setMatrix("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        shader_point_light.setFloat("far_plane", far);
        model = glm::identity<glm::mat4>();
        shader_point_light.setMatrix("model", model);
        models->Draw(shader_point_light);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        /////////////////////////这里开始场景渲染
        glViewport(0, 0, screenWidth, screenHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setFloat("time", glfwGetTime());
        /*shader光源设置*/
        shader.setVec3("dl[0].dir", glm::vec3(-1, -1, -1));
        shader.setVec3("dl[0].color", glm::vec3(0.0f, 0.0f, 2.0f));
        shader.setVec3("pl[0].pos", lightPos);
        shader.setVec3("pl[0].color", glm::vec3(2.0f, 0.0f, 0.0f));
        shader.setFloat("pl[0].constant", 1.0f);
        shader.setFloat("pl[0].linear", 0.1f);
        shader.setFloat("pl[0].quadratic", 0.0f);

        /*shader相机设置*/
        shader.setVec3("cameraPos", camera->cameraPos);

        /*shader空间变换设置*/
        model = glm::identity<glm::mat4>();
        model = glm::rotate(model, glm::radians(-0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 view = camera->getView();
        glm::mat4 projection = glm::identity<glm::mat4>();
        projection = glm::perspective(camera->fov, screenWidth / screenHeight, 0.1f, 500.0f);
        shader.setMatrix("model", model);
        shader.setMatrix("view", view);
        shader.setMatrix("projection", projection); 

        /*shader材质参数设置*/
        shader.setVec3("material.ambient", glm::vec3(0.25f, 0.25f, 0.25f));
        shader.setVec4("material.diffuse", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
        shader.setVec3("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
        shader.setFloat("material.reflects", 0.0f);
        shader.setFloat("material.shininess_n", 64.0f);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        shader.setInt("cubeTexture", 4);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        shader.setInt("dir_shadowMap", 5);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        shader.setInt("point_shadowMap", 6);
        shader.setMatrix("lightSpaceMatrix", lightSpaceMatrix);
        shader.setFloat("far_plane", far);
        models->Draw(shader);

        /////////////////////////这里开始立方体环境贴图渲染
        /*立方体贴图背景渲染*/
        glDepthFunc(GL_LEQUAL);
        shader_cubemap.use();
        shader_cubemap.setInt("cubeTexture", 0);
        view = glm::mat4(glm::mat3(camera->getView()));
        shader_cubemap.setMatrix("view", view);
        shader_cubemap.setMatrix("projection", projection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        /////////////////////////这里开始将渲染结果输出到屏幕上
        glDepthFunc(GL_LESS);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shader_post.setInt("screenTexture", 0);
        shader_post.use();
        glBindVertexArray(quadVAO);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);	// use the color attachment texture as the texture of the quad plane
        glDrawArrays(GL_TRIANGLES, 0, 6);


        glfwSwapBuffers(window);               //双缓冲，交换前后buffer
        glfwPollEvents();                      //检查事件队列中是否有事件传达
    }
    glfwTerminate();                           //结束线程，释放资源
    return 0;
}