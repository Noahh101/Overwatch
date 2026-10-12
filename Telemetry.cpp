#include "Telemetry.h"

struct ProcessorPowerInformation {
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
};

namespace {
constexpr DWORD kPdhCStatusValidData = 0;
constexpr DWORD kPdhCStatusNewData = 1;
}

bool CPUStats::init() {
    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);
    const DWORD coreCount = sysInfo.dwNumberOfProcessors;
    if (coreCount == 0) {
        return false;
    }

    std::vector<ProcessorPowerInformation> coreInfo(coreCount);
    const NTSTATUS status = CallNtPowerInformation(
        ProcessorInformation, nullptr, 0, coreInfo.data(),
        static_cast<ULONG>(sizeof(ProcessorPowerInformation) * coreCount));
    if (status != 0) {
        return false;
    }

    unsigned long totalMax = 0;
    for (DWORD i = 0; i < coreCount; ++i) {
        totalMax += coreInfo[i].MaxMhz;
    }
    nominalMhz = totalMax / coreCount;
    if (nominalMhz == 0) {
        return false;
    }

    if (PdhOpenQueryW(nullptr, 0, &query) != ERROR_SUCCESS ||
        PdhAddEnglishCounterW(query,
            L"\\Processor Information(_Total)\\% Processor Performance",
            0, &performanceCounter) != ERROR_SUCCESS ||
        PdhCollectQueryData(query) != ERROR_SUCCESS) {
        cleanup();
        return false;
    }

    lastSampleTime = GetTickCount64();
    isAvailable = true;
    return true;
}

void CPUStats::update() {
    if (!isAvailable || !query || !performanceCounter) {
        return;
    }

    const ULONGLONG now = GetTickCount64();
    if (now - lastSampleTime < 1000) {
        return;
    }
    lastSampleTime = now;

    if (PdhCollectQueryData(query) != ERROR_SUCCESS) {
        return;
    }

    PDH_FMT_COUNTERVALUE value{};
    if (PdhGetFormattedCounterValue(
            performanceCounter, PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS &&
        (value.CStatus == kPdhCStatusValidData ||
         value.CStatus == kPdhCStatusNewData)) {
        const double estimatedMhz = nominalMhz * value.doubleValue / 100.0;
        if (estimatedMhz >= 0.0) {
            currentMhz = static_cast<unsigned long>(estimatedMhz + 0.5);
            if (currentMhz > peakMhz) {
                peakMhz = currentMhz;
            }
        }
    }
}

void CPUStats::cleanup() {
    isAvailable = false;
    performanceCounter = nullptr;
    if (query) {
        PdhCloseQuery(query);
        query = nullptr;
    }
}


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