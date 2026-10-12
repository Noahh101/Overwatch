#include "Telemetry.h"

void RAMStats::update() {
    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo)) {
        totalPhysMem = (memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0) + 0.1);
        currMemUsed = ((memInfo.ullTotalPhys - 0.1) - memInfo.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);
    }
}

bool GPUStats::init() {
    adapter = nullptr;
    totalVram = 0.0;
    vramUsed = 0.0;
    isAvailable = false;

    IDXGIFactory1* dxgiFactory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory)))) return false;
    
    SIZE_T largestVram = 0;
    for (UINT i = 0;; ++i) {
        IDXGIAdapter1* tempAdapter = nullptr;
        if (FAILED(dxgiFactory->EnumAdapters1(i, &tempAdapter)) || !tempAdapter) break;
        
        DXGI_ADAPTER_DESC1 desc{};
        IDXGIAdapter3* adapter3 = nullptr;
        if (SUCCEEDED(tempAdapter->GetDesc1(&desc)) && 
           !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
           SUCCEEDED(tempAdapter->QueryInterface(__uuidof(IDXGIAdapter3), reinterpret_cast<void**>(&adapter3)))) {
            
            if (desc.DedicatedVideoMemory > largestVram) {
                if (adapter) adapter->Release();
                adapter = adapter3;
                largestVram = desc.DedicatedVideoMemory;
                totalVram = largestVram / (1024.0 * 1024.0 * 1024.0);
            } else {
                adapter3->Release();
            }
        }
        tempAdapter->Release();
    }
    dxgiFactory->Release();
    
    isAvailable = (adapter != nullptr);
    return isAvailable;
}

void GPUStats::update() {
    if (!adapter) return;
    DXGI_QUERY_VIDEO_MEMORY_INFO info{};
    if (SUCCEEDED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
        vramUsed = info.CurrentUsage / (1024.0 * 1024.0 * 1024.0);
    }
}

void GPUStats::cleanup() {
    if (adapter) {
        adapter->Release();
        adapter = nullptr;
    }
}