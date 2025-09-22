#include "Precompiled.h"
#include "GraphicsSystem.h"
#include "Message.h"
#include "Core.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace Framework {

    float vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };

    GraphicsSystem::GraphicsSystem()
        : window(nullptr), shader(nullptr), VAO(0), VBO(0) {
    }

    GraphicsSystem::~GraphicsSystem() {
        std::cout << "GraphicsSystem: Cleaning up...\n";
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        delete shader;

        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }

    void GraphicsSystem::Initialize() {
        std::cout << "GraphicsSystem: Initializing...\n";

        if (!glfwInit()) {
            std::cerr << "GLFW init failed\n";
            return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(800, 600, "Graphics System", nullptr, nullptr);
        if (!window) {
            std::cerr << "Window creation failed\n";
            glfwTerminate();
            return;
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "GLAD init failed\n";
            return;
        }

        glViewport(0, 0, 800, 600);

        shader = new Shader("shaders/basic.vert", "shaders/basic.frag");

        // VAO/VBO setup
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    void GraphicsSystem::Update(float dt) {
        BeginFrame();

        shader->Bind();

        // Move the top vertex up and down over time
        vertices[1] = 0.5f + static_cast<float>(sin(glfwGetTime())) * 0.25f;

        // Re-upload new vertex data to GPU
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        EndFrame();
    }

    void GraphicsSystem::SendMessage(Message* message) {
            if (message->MessageId == Status::Quit)
            {
                std::cout << "GraphicsSystem: Received quit message\n";
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
    }

    void GraphicsSystem::BeginFrame() {
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void GraphicsSystem::EndFrame() {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    bool Framework::GraphicsSystem::ShouldClose() const {
        return glfwWindowShouldClose(window);
    }

}