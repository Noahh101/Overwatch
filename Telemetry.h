#pragma once
#include <windows.h>
#include <dxgi1_4.h>
#include <pdh.h>
#include <powrprof.h>
#include <vector>

struct CPUStats {
    unsigned long currentMhz = 0;
    unsigned long nominalMhz = 0;
    unsigned long peakMhz = 0;
    bool isAvailable = false;
    PDH_HQUERY query = nullptr;
    PDH_HCOUNTER performanceCounter = nullptr;
    ULONGLONG lastSampleTime = 0;

    bool init();
    void update();
    void cleanup();
};


struct RAMStats {
    double totalPhysMem = 0.0;
    double currMemUsed = 0.0;
    
    // refresh memory readings
    void update();
};

struct GPUStats {
    double totalVram = 0.0;
    double vramUsed = 0.0;
    bool isAvailable = false;
    PDH_HQUERY memoryQuery = nullptr;
    std::vector<PDH_HCOUNTER> dedicatedUsageCounters;
    
    // manage system-wide dedicated GPU memory counters
    bool init();
    void update();
    void cleanup();
};