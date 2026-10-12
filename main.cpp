#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "Telemetry.h" // Your new abstraction layer

int main() {
    // initialize the glfw window manager
    if (!glfwInit()) return -1;
    
    GLFWwindow* window = glfwCreateWindow(1440, 900, "Overwatch Telemetry", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    bool vsync = true;
    glfwSwapInterval(vsync ? 1 : 0);

    // setup the imgui context
    IMGUI_CHECKVERSION();
    ImGuiContext* imguiContext = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    bool showCpuMonitor = true;
    bool showRamMonitor = true;
    bool showGpuMonitor = true;
    bool darkTheme = true;
    bool showAbout = false;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImVec4 clear_color(0.1f, 0.1f, 0.1f, 1.0f);

    // Initialize Hardware Telemetry
    RAMStats ram;
    GPUStats gpu;
    gpu.init();
    
    // variables for loading screen
    bool isLoading = true;
    double startTime = glfwGetTime();

    // main application loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Grab live data from the hardware drivers
        ram.update();
        gpu.update();

        // start a new imgui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // loading screen state
        if (isLoading) {
            double currentTime = glfwGetTime();
            float loadDuration = 1.0f; // seconds
            float progress = (float)(currentTime - startTime) / loadDuration;
            
            if (currentTime - startTime > loadDuration) {
                isLoading = false;
            }

            ImGui::SetNextWindowPos(ImVec2(display_w * 0.5f, display_h * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGuiWindowFlags loadingFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground;
            ImGui::Begin("Startup", nullptr, loadingFlags);
            
            ImGui::Text("Initializing Hardware Hooks...");
            ImGui::ProgressBar(progress, ImVec2(250.0f, 6.0f), ""); 
            ImGui::End();
        } 
        // main application state
        else {
            bool openAboutRequested = false;
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Exit")) glfwSetWindowShouldClose(window, GLFW_TRUE);
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

            if (openAboutRequested) ImGui::OpenPopup("About");

            if (showCpuMonitor) {
                ImGui::Begin("CPU Monitor", &showCpuMonitor);
                ImGui::Text("CPU Usage: --%%");
                ImGui::End();
            }

            if (showRamMonitor) {
                ImGui::Begin("Ram Monitor", &showRamMonitor);
                // Accessing the struct variables cleanly
                ImGui::Text("RAM Usage: %.1f GB / %.1f GB", ram.currMemUsed, ram.totalPhysMem);
                ImGui::End();
            }

            if (showGpuMonitor) {
                ImGui::Begin("GPU Monitor", &showGpuMonitor);
                if (gpu.isAvailable) {
                    // Accessing the struct variables cleanly
                    ImGui::Text("Dedicated VRAM used by this app: %.2f GB", gpu.vramUsed);
                    ImGui::Text("Dedicated VRAM total: %.2f GB", gpu.totalVram);
                } else {
                    ImGui::Text("Dedicated VRAM statistics unavailable");
                }
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
        }

        // render opengl data and swap buffers
        ImGui::Render();
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
    
    // Close the COM connection safely
    gpu.cleanup();
    
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}