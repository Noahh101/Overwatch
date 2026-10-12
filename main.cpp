#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "Telemetry.h"
#include "imgui/implot.h"
#include <algorithm>
#include <array>
#include <string>

namespace
{
constexpr double graphSampleInterval = 0.25;
constexpr int graphHistorySize = 240;
constexpr double graphHistoryDuration = graphSampleInterval * (graphHistorySize - 1);

const ImVec4 darkAccent(0.32f, 0.72f, 0.96f, 1.0f);
const ImVec4 lightAccent(0.10f, 0.42f, 0.68f, 1.0f);

bool LoadModernFont()
{
    char windowsDirectory[MAX_PATH]{};
    const UINT length = GetWindowsDirectoryA(windowsDirectory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return false;

    const std::string fontPath = std::string(windowsDirectory) + "\\Fonts\\segoeui.ttf";
    ImFont *font = ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.c_str(), 17.0f);
    if (!font)
        return false;

    ImGui::GetIO().FontDefault = font;
    return true;
}

void ApplyAccentStyle(bool darkTheme)
{
    ImGuiStyle &style = ImGui::GetStyle();
    const ImVec4 accent = darkTheme ? darkAccent : lightAccent;
    style.WindowRounding = 10.0f;
    style.ChildRounding = 9.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 5.0f;
    style.WindowPadding = ImVec2(18.0f, 16.0f);
    style.FramePadding = ImVec2(12.0f, 8.0f);
    style.ItemSpacing = ImVec2(12.0f, 10.0f);

    style.Colors[ImGuiCol_Text] = darkTheme ? ImVec4(0.89f, 0.92f, 0.97f, 1.0f) : ImVec4(0.12f, 0.16f, 0.22f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = darkTheme ? ImVec4(0.54f, 0.60f, 0.69f, 1.0f) : ImVec4(0.42f, 0.47f, 0.54f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = darkTheme ? ImVec4(0.055f, 0.070f, 0.095f, 1.0f) : ImVec4(0.94f, 0.96f, 0.98f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = darkTheme ? ImVec4(0.080f, 0.100f, 0.135f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = darkTheme ? ImVec4(0.075f, 0.090f, 0.120f, 0.98f) : ImVec4(0.98f, 0.99f, 1.0f, 0.98f);
    style.Colors[ImGuiCol_Border] = darkTheme ? ImVec4(0.17f, 0.21f, 0.28f, 1.0f) : ImVec4(0.79f, 0.83f, 0.88f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = darkTheme ? ImVec4(0.105f, 0.125f, 0.165f, 1.0f) : ImVec4(0.88f, 0.91f, 0.95f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = darkTheme ? ImVec4(0.14f, 0.20f, 0.27f, 1.0f) : ImVec4(0.82f, 0.88f, 0.94f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = darkTheme ? ImVec4(0.15f, 0.23f, 0.31f, 1.0f) : ImVec4(0.77f, 0.85f, 0.93f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = style.Colors[ImGuiCol_WindowBg];
    style.Colors[ImGuiCol_TitleBgActive] = darkTheme ? ImVec4(0.08f, 0.11f, 0.16f, 1.0f) : ImVec4(0.87f, 0.91f, 0.95f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = darkTheme ? ImVec4(0.075f, 0.090f, 0.120f, 1.0f) : ImVec4(0.90f, 0.93f, 0.97f, 1.0f);
    style.Colors[ImGuiCol_Button] = darkTheme ? ImVec4(0.12f, 0.29f, 0.41f, 1.0f) : ImVec4(0.82f, 0.90f, 0.96f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = darkTheme ? ImVec4(0.15f, 0.39f, 0.55f, 1.0f) : ImVec4(0.72f, 0.84f, 0.94f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = darkTheme ? ImVec4(0.10f, 0.25f, 0.37f, 1.0f) : ImVec4(0.63f, 0.79f, 0.92f, 1.0f);
    style.Colors[ImGuiCol_Header] = darkTheme ? ImVec4(0.12f, 0.25f, 0.35f, 1.0f) : ImVec4(0.82f, 0.90f, 0.96f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = style.Colors[ImGuiCol_ButtonHovered];
    style.Colors[ImGuiCol_HeaderActive] = style.Colors[ImGuiCol_ButtonActive];
    style.Colors[ImGuiCol_CheckMark] = accent;
    style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];

    ImPlot::StyleColorsAuto();
    ImPlotStyle &plotStyle = ImPlot::GetStyle();
    plotStyle.PlotDefaultSize = ImVec2(400.0f, 260.0f);
    plotStyle.PlotMinSize = ImVec2(220.0f, 160.0f);
    plotStyle.Colors[ImPlotCol_FrameBg] = style.Colors[ImGuiCol_ChildBg];
    plotStyle.Colors[ImPlotCol_PlotBg] = darkTheme ? ImVec4(0.055f, 0.070f, 0.095f, 1.0f) : ImVec4(0.96f, 0.97f, 0.99f, 1.0f);
    plotStyle.Colors[ImPlotCol_PlotBorder] = style.Colors[ImGuiCol_Border];
    plotStyle.Colors[ImPlotCol_AxisGrid] = darkTheme ? ImVec4(0.24f, 0.29f, 0.36f, 0.55f) : ImVec4(0.42f, 0.49f, 0.58f, 0.30f);
}

void DrawGraphToggle(bool &graphVisible)
{
    if (ImGui::Button(graphVisible ? "Hide graph" : "Show graph", ImVec2(142.0f, 34.0f)))
        graphVisible = !graphVisible;
}

struct MonitorHistory
{
    std::array<float, graphHistorySize> values{};
    int count = 0;

    void add(float value)
    {
        if (count < graphHistorySize)
        {
            values[count++] = value;
            return;
        }

        for (int i = 1; i < graphHistorySize; ++i)
            values[i - 1] = values[i];
        values[graphHistorySize - 1] = value;
    }

    void plot(const char *title, const char *series, const char *yAxis, float scaleRange, float plotHeight) const
    {
        if (ImPlot::BeginPlot(title, ImVec2(-1, plotHeight)))
        {
            ImPlot::SetupAxes("Seconds before now", yAxis);
            ImPlot::SetupAxisLimits(ImAxis_X1, -graphHistoryDuration, 0, ImPlotCond_Always);
            if (count > 0)
            {
                const auto bounds = std::minmax_element(values.begin(), values.begin() + count);
                const float observedMin = *bounds.first;
                const float observedMax = *bounds.second;
                const float minimumRange = std::max(scaleRange * 0.05f, 0.01f);
                const float visibleRange = std::max(observedMax - observedMin, minimumRange);
                const float center = (observedMin + observedMax) * 0.5f;
                const float halfRangeWithPadding = visibleRange * 0.75f;
                ImPlot::SetupAxisLimits(
                    ImAxis_Y1, center - halfRangeWithPadding,
                    center + halfRangeWithPadding, ImPlotCond_Always);
                ImPlot::PlotLine(series, values.data(), count, graphSampleInterval, -(count - 1) * graphSampleInterval);
            }
            else
            {
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, std::max(scaleRange, 1.0f), ImPlotCond_Always);
            }
            ImPlot::EndPlot();
        }
    }
};

void DrawCpuCard(CPUStats &cpu, MonitorHistory &history, bool &showGraph, float graphHeight)
{
    ImGui::TextDisabled("PROCESSOR");
    if (cpu.isAvailable)
    {
        ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_CheckMark], "%.2f GHz", cpu.currentMhz / 1000.0);
        ImGui::TextDisabled("Peak %.0f MHz  |  Base %lu MHz", static_cast<double>(cpu.peakMhz), cpu.nominalMhz);
    }
    else
    {
        ImGui::TextDisabled("Live CPU frequency unavailable");
    }

    DrawGraphToggle(showGraph);
    if (showGraph)
    {
        if (cpu.isAvailable)
            history.plot("CPU Clock History", "Current clock", "MHz",
                 static_cast<float>(std::max(cpu.nominalMhz, cpu.peakMhz) * 1.25f), graphHeight);
        else
            ImGui::TextDisabled("History appears when live frequency data is available.");
    }
}

void DrawRamCard(RAMStats &ram, MonitorHistory &history, bool &showGraph, float graphHeight)
{
    const double usedPercent = ram.totalPhysMem > 0.0 ? ram.currMemUsed * 100.0 / ram.totalPhysMem : 0.0;
    ImGui::TextDisabled("WHOLE-PC SYSTEM MEMORY");
    ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_CheckMark], "%.1f GB", ram.currMemUsed);
    ImGui::SameLine();
    ImGui::TextDisabled("used of %.1f GB", ram.totalPhysMem);
    ImGui::ProgressBar(static_cast<float>(usedPercent / 100.0), ImVec2(-1.0f, 8.0f), "");
    ImGui::TextDisabled("%.1f GB available  |  %.0f%% used", std::max(ram.totalPhysMem - ram.currMemUsed, 0.0), usedPercent);

    DrawGraphToggle(showGraph);
    if (showGraph)
        history.plot("Whole-PC RAM Usage History", "System RAM used", "GB", static_cast<float>(ram.totalPhysMem), graphHeight);
}

void DrawGpuCard(GPUStats &gpu, MonitorHistory &history, bool &showGraph, float graphHeight)
{
    ImGui::TextDisabled("ALL DETECTED DEDICATED GPU MEMORY");
    if (gpu.isAvailable)
    {
        ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_CheckMark], "%.2f GB", gpu.vramUsed);
        ImGui::SameLine();
        ImGui::TextDisabled("used of %.2f GB", gpu.totalVram);
        const double usedPercent = gpu.totalVram > 0.0 ? gpu.vramUsed * 100.0 / gpu.totalVram : 0.0;
        ImGui::ProgressBar(static_cast<float>(usedPercent / 100.0), ImVec2(-1.0f, 8.0f), "");
        ImGui::TextDisabled("System-wide use across all running applications");
    }
    else
    {
        ImGui::TextDisabled("System-wide GPU memory counters unavailable.");
    }

    DrawGraphToggle(showGraph);
    if (showGraph)
    {
        if (gpu.isAvailable)
            history.plot("System-wide Dedicated VRAM History", "Dedicated VRAM used", "GB", static_cast<float>(gpu.totalVram), graphHeight);
        else
            ImGui::TextDisabled("Enable Windows GPU Adapter Memory counters to view history.");
    }
}
}

int main()
{
    // start glfw
    if (!glfwInit())
        return -1;

    GLFWwindow *window = glfwCreateWindow(1440, 900, "Overwatch | System Monitor", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    bool vsync = true;
    glfwSwapInterval(vsync ? 1 : 0);

    // set up imgui
    IMGUI_CHECKVERSION();
    ImGuiContext *imguiContext = ImGui::CreateContext();
    ImPlot::CreateContext();
    const bool modernFontLoaded = LoadModernFont();
    ImGui::StyleColorsDark();
    bool darkTheme = true;
    ApplyAccentStyle(darkTheme);

    bool showCpuMonitor = true;
    bool showRamMonitor = true;
    bool showGpuMonitor = true;
    bool showCpuGraph = true;
    bool showRamGraph = true;
    bool showGpuGraph = true;
    bool showAbout = false;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImVec4 clear_color(0.1f, 0.1f, 0.1f, 1.0f);

    // initialize hardware stats
    CPUStats cpu;
    cpu.init();
    RAMStats ram;
    GPUStats gpu;
    gpu.init();
    MonitorHistory cpuHistory;
    MonitorHistory ramHistory;
    MonitorHistory gpuHistory;
    double nextGraphSample = glfwGetTime();

    // run the app
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        const double currentTime = glfwGetTime();
        if (currentTime >= nextGraphSample)
        {
            ram.update();
            gpu.update();
            cpu.update();

            if (cpu.isAvailable && cpu.currentMhz > 0)
                cpuHistory.add(static_cast<float>(cpu.currentMhz));
            if (ram.totalPhysMem > 0.0)
                ramHistory.add(static_cast<float>(ram.currMemUsed));
            if (gpu.isAvailable)
                gpuHistory.add(static_cast<float>(gpu.vramUsed));
            nextGraphSample = currentTime + graphSampleInterval;
        }

        // start an imgui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        {
            bool openAboutRequested = false;
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Exit"))
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("View"))
                {
                    ImGui::MenuItem("CPU Monitor", nullptr, &showCpuMonitor);
                    ImGui::MenuItem("RAM Monitor", nullptr, &showRamMonitor);
                    ImGui::MenuItem("GPU Monitor", nullptr, &showGpuMonitor);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Settings"))
                {
                    if (ImGui::MenuItem("VSync", nullptr, &vsync))
                    {
                        glfwSwapInterval(vsync ? 1 : 0);
                    }
                    if (ImGui::BeginMenu("Theme"))
                    {
                        if (ImGui::MenuItem("Dark", nullptr, darkTheme))
                        {
                            darkTheme = true;
                            ImGui::StyleColorsDark();
                            ApplyAccentStyle(darkTheme);
                            clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
                        }
                        if (ImGui::MenuItem("Light", nullptr, !darkTheme))
                        {
                            darkTheme = false;
                            ImGui::StyleColorsLight();
                            ApplyAccentStyle(darkTheme);
                            clear_color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Help"))
                {
                    if (ImGui::MenuItem("About"))
                    {
                        showAbout = true;
                        openAboutRequested = true;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            if (openAboutRequested)
                ImGui::OpenPopup("About");

            const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            const float menuBarHeight = ImGui::GetFrameHeight();
            ImGui::SetNextWindowPos(ImVec2(0.0f, menuBarHeight), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(
                ImVec2(displaySize.x, std::max(displaySize.y - menuBarHeight, 1.0f)),
                ImGuiCond_FirstUseEver);
            ImGui::Begin("Hardware Dashboard");

            ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_CheckMark], "OVERWATCH");
            ImGui::SameLine();
            ImGui::TextDisabled("SYSTEM TELEMETRY");
            ImGui::TextDisabled("Live hardware activity with a rolling 60-second history");
            if (!modernFontLoaded)
                ImGui::TextDisabled("Segoe UI was not found; using the built-in font.");
            ImGui::Separator();
            ImGui::Spacing();

            const int visibleMonitorCount =
                static_cast<int>(showCpuMonitor) +
                static_cast<int>(showRamMonitor) +
                static_cast<int>(showGpuMonitor);
            if (visibleMonitorCount == 0)
            {
                ImGui::TextDisabled("No monitor cards are visible. Enable one from the View menu.");
            }
            else
            {
                const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
                const float availableWidth = ImGui::GetContentRegionAvail().x;
                const int columnsThatFit = std::max(
                    1, static_cast<int>((availableWidth + itemSpacing) / (430.0f + itemSpacing)));
                const int columnCount = std::min(visibleMonitorCount, columnsThatFit);
                const int rowCount = (visibleMonitorCount + columnCount - 1) / columnCount;
                const ImVec2 available = ImGui::GetContentRegionAvail();
                const float rowHeight = (available.y - itemSpacing * (rowCount - 1)) / rowCount;
                const float cardHeight = std::max(350.0f, std::min(500.0f, rowHeight));
                const float graphHeight = std::max(170.0f, std::min(260.0f, cardHeight - 145.0f));
                float cardWidth =
                    (availableWidth - itemSpacing * (columnCount - 1)) / columnCount;
                if (visibleMonitorCount == 1)
                {
                    cardWidth = std::min(cardWidth, 800.0f);
                    ImGui::SetCursorPosX(
                        ImGui::GetCursorPosX() + (availableWidth - cardWidth) * 0.5f);
                }
                int cardIndex = 0;

                const auto beginCard = [&](const char *name) {
                    if (cardIndex > 0 && cardIndex % columnCount != 0)
                        ImGui::SameLine();
                    ImGui::BeginChild(name, ImVec2(cardWidth, cardHeight), ImGuiChildFlags_Borders);
                    ++cardIndex;
                };

                if (showCpuMonitor)
                {
                    beginCard("CPU Monitor Card");
                    DrawCpuCard(cpu, cpuHistory, showCpuGraph, graphHeight);
                    ImGui::EndChild();
                }
                if (showRamMonitor)
                {
                    beginCard("RAM Monitor Card");
                    DrawRamCard(ram, ramHistory, showRamGraph, graphHeight);
                    ImGui::EndChild();
                }
                if (showGpuMonitor)
                {
                    beginCard("GPU Monitor Card");
                    DrawGpuCard(gpu, gpuHistory, showGpuGraph, graphHeight);
                    ImGui::EndChild();
                }
            }
            ImGui::End();

            if (showAbout && ImGui::BeginPopupModal("About", &showAbout, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("A real-time desktop hardware monitoring dashboard.");
                ImGui::Text("System-wide CPU, RAM, and dedicated GPU memory history.");
                if (ImGui::Button("Close"))
                {
                    showAbout = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        // draw the frame
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // release app resources
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext(imguiContext);

    // release hardware stats
    cpu.cleanup();
    gpu.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}