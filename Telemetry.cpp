#include "Telemetry.h"
#include <cwchar>

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
        constexpr double bytesPerGigabyte = 1024.0 * 1024.0 * 1024.0;
        totalPhysMem = memInfo.ullTotalPhys / bytesPerGigabyte;
        currMemUsed = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / bytesPerGigabyte;
    }
}

bool GPUStats::init() {
    totalVram = 0.0;
    vramUsed = 0.0;
    isAvailable = false;
    dedicatedUsageCounters.clear();

    IDXGIFactory1* dxgiFactory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory))))
        return false;

    if (PdhOpenQueryW(nullptr, 0, &memoryQuery) != ERROR_SUCCESS) {
        dxgiFactory->Release();
        return false;
    }

    bool missingAdapterCounter = false;
    for (UINT i = 0;; ++i) {
        IDXGIAdapter1* tempAdapter = nullptr;
        if (FAILED(dxgiFactory->EnumAdapters1(i, &tempAdapter)) || !tempAdapter)
            break;

        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(tempAdapter->GetDesc1(&desc)) &&
            !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
            desc.DedicatedVideoMemory > 0) {
            wchar_t counterPath[128]{};
            std::swprintf(
                counterPath, sizeof(counterPath) / sizeof(counterPath[0]),
                L"\\GPU Adapter Memory(luid_0x%08lx_0x%08lx_phys_0)\\Dedicated Usage",
                static_cast<unsigned long>(desc.AdapterLuid.HighPart),
                static_cast<unsigned long>(desc.AdapterLuid.LowPart));

            PDH_HCOUNTER counter = nullptr;
            if (PdhAddEnglishCounterW(memoryQuery, counterPath, 0, &counter) == ERROR_SUCCESS) {
                dedicatedUsageCounters.push_back(counter);
                totalVram += desc.DedicatedVideoMemory / (1024.0 * 1024.0 * 1024.0);
            }
            else {
                missingAdapterCounter = true;
            }
        }
        tempAdapter->Release();
    }
    dxgiFactory->Release();

    if (missingAdapterCounter || dedicatedUsageCounters.empty() ||
        PdhCollectQueryData(memoryQuery) != ERROR_SUCCESS) {
        cleanup();
        return false;
    }

    isAvailable = true;
    return isAvailable;
}

void GPUStats::update() {
    if (!isAvailable || !memoryQuery)
        return;

    if (PdhCollectQueryData(memoryQuery) != ERROR_SUCCESS) {
        isAvailable = false;
        return;
    }

    constexpr double bytesPerGigabyte = 1024.0 * 1024.0 * 1024.0;
    double totalUsedBytes = 0.0;
    bool allCountersValid = true;
    for (PDH_HCOUNTER counter : dedicatedUsageCounters) {
        PDH_FMT_COUNTERVALUE value{};
        if (PdhGetFormattedCounterValue(counter, PDH_FMT_LARGE, nullptr, &value) != ERROR_SUCCESS) {
            allCountersValid = false;
            break;
        }
        if (value.CStatus != kPdhCStatusValidData &&
            value.CStatus != kPdhCStatusNewData) {
            allCountersValid = false;
            break;
        }
        if (value.largeValue < 0) {
            allCountersValid = false;
            break;
        }
        totalUsedBytes += static_cast<double>(value.largeValue);
    }

    if (allCountersValid)
        vramUsed = totalUsedBytes / bytesPerGigabyte;
}

void GPUStats::cleanup() {
    isAvailable = false;
    totalVram = 0.0;
    vramUsed = 0.0;
    dedicatedUsageCounters.clear();
    if (memoryQuery) {
        PdhCloseQuery(memoryQuery);
        memoryQuery = nullptr;
    }
}