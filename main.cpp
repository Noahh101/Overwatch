#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "windows.h"
#include "psapi.h"
#include "TCHAR.h"
#include "pdh.h"

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
    bool vsync = true;
    glfwSwapInterval(vsync ? 1 : 0); // enable vsync to cap the frame rate

    // setup the imgui context
    IMGUI_CHECKVERSION();
    ImGuiContext* imguiContext = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); // apply default dark theme

    bool showCpuMonitor = true;
    bool showRamMonitor = true;
    bool showGpuMonitor = true;
    bool darkTheme = true;
    bool showAbout = false;

    // initialize imgui backends for glfw and opengl
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImVec4 clear_color(0.1f, 0.1f, 0.1f, 1.0f);

    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(memInfo);
    double totalPhysMem = 0;
    double currMemUsed = 0;


   

    // !!!!!!!!!!!!!main application loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (GlobalMemoryStatusEx(&memInfo)) {
        totalPhysMem = (memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0)+0.1); // gb conv
        currMemUsed = ((memInfo.ullTotalPhys-0.1) - memInfo.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0); // gb conv
    }
        // start a new imgui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        bool openAboutRequested = false;
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("CPU Monitor", nullptr, &showCpuMonitor);
                ImGui::MenuItem("RAM Monitor", nullptr, &showRamMonitor);
                ImGui::MenuItem("GPU Monitor", nullptr, &showGpuMonitor);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Settings")) {
                if (ImGui::MenuItem("VSync", nullptr, &vsync)) {
                    glfwSwapInterval(vsync ? 1 : 0);
                }
                if (ImGui::BeginMenu("Theme")) {
                    if (ImGui::MenuItem("Dark", nullptr, darkTheme)) {
                        darkTheme = true;
                        ImGui::StyleColorsDark();
                        clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
                    }
                    if (ImGui::MenuItem("Light", nullptr, !darkTheme)) {
                        darkTheme = false;
                        ImGui::StyleColorsLight();
                        clear_color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
                    showAbout = true;
                    openAboutRequested = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (openAboutRequested) {
            ImGui::OpenPopup("About");
        }

        if (showCpuMonitor) {
            ImGui::Begin("CPU Monitor", &showCpuMonitor);
            ImGui::Text("CPU Usage: --%%");
            ImGui::End();
        }

        if (showRamMonitor) {
            ImGui::Begin("Ram Monitor", &showRamMonitor);
            ImGui::Text("RAM Usage: %.1f GB / %.1f GB", currMemUsed, totalPhysMem);
            ImGui::End();
        }

        if (showGpuMonitor) {
            ImGui::Begin("GPU Monitor", &showGpuMonitor);
            ImGui::Text("GPU Usage: --%%");
            ImGui::End();
        }

        if (showAbout && ImGui::BeginPopupModal("About", &showAbout, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("A desktop hardware monitoring dashboard.");
            ImGui::Text("Built with C++ and Dear ImGui.");
            if (ImGui::Button("Close")) {
                showAbout = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // render opengl data and swap buffers
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // cleanup and free memory on exit
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(imguiContext);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}