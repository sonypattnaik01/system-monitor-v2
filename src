#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

struct CpuTimes {
    unsigned long long idle = 0;
    unsigned long long kernel = 0; // includes idle
    unsigned long long user = 0;
};

struct MemoryInfo {
    unsigned long long totalPhys = 0;
    unsigned long long usedPhys = 0;
};

bool GetSystemCpuTimes(CpuTimes& out);
double ComputeTotalCpuPercent(const CpuTimes& prev, const CpuTimes& curr);
bool GetMemoryInfo(MemoryInfo& out);

// Helper: FILETIME -> uint64
inline unsigned long long FtToULL(const FILETIME& ft) {
    ULARGE_INTEGER ui;
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    return ui.QuadPart;
}
