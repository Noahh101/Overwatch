#pragma once
#include <windows.h>
#include <dxgi1_4.h>

struct RAMStats {
    double totalPhysMem;
    double currMemUsed;
    
    // Function to grab live memory data
    void update();
};

struct GPUStats {
    IDXGIAdapter3* adapter;
    double totalVram;
    double vramUsed;
    bool isAvailable;
    
    // Functions to manage the DirectX COM connection
    bool init();
    void update();
    void cleanup();
};