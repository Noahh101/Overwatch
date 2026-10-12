#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    // initialize the glfw window manager
    if (!glfwInit()) return -1;
    
    // set initial window dimensions and title
    GLFWwindow* window = glfwCreateWindow(1440, 900, "Overwatch Telemetry", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    int vsync = 1;
    glfwSwapInterval(vsync); // enable vsync to cap the frame rate

    // setup the imgui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); // apply default dark theme

    // initialize imgui backends for glfw and opengl
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // main application loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // start a new imgui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // the actual ui sandbox
        ImGui::Begin("Hardware Monitor");
        ImGui::Text("CPU Usage: --%%");
        ImGui::Text("RAM Usage: -- GB / -- GB");
        
        // vsync toggle button
        if (ImGui::Button(vsync ? "VSync: On" : "VSync: Off")) {
            vsync = !vsync;
            glfwSwapInterval(vsync ? 1 : 0);
        }
        ImGui::End();

        // render opengl data and swap buffers
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // cleanup and free memory on exit
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}