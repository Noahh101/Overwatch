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
    double totalPhysMem;
    double currMemUsed;
    
    // refresh memory readings
    void update();
};

struct GPUStats {
    IDXGIAdapter3* adapter;
    double totalVram;
    double vramUsed;
    bool isAvailable;
    
    // manage the dxgi adapter
    bool init();
    void update();
    void cleanup();
};