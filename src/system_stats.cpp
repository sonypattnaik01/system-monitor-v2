#include "system_stats.h"

bool GetSystemCpuTimes(CpuTimes& out) {
    FILETIME idleFT, kernelFT, userFT;
    if (!GetSystemTimes(&idleFT, &kernelFT, &userFT)) return false;
    out.idle   = FtToULL(idleFT);
    out.kernel = FtToULL(kernelFT);
    out.user   = FtToULL(userFT);
    return true;
}

double ComputeTotalCpuPercent(const CpuTimes& prev, const CpuTimes& curr) {
    const auto idleDelta   = curr.idle   - prev.idle;
    const auto kernelDelta = curr.kernel - prev.kernel;
    const auto userDelta   = curr.user   - prev.user;

    const auto total = kernelDelta + userDelta; // kernel includes idle
    if (total == 0) return 0.0;

    const auto busy = total - idleDelta;
    double pct = (static_cast<double>(busy) / static_cast<double>(total)) * 100.0;
    if (pct < 0.0) pct = 0.0;
    if (pct > 100.0) pct = 100.0;
    return pct;
}

bool GetMemoryInfo(MemoryInfo& out) {
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) return false;
    out.totalPhys = ms.ullTotalPhys;
    out.usedPhys  = ms.ullTotalPhys - ms.ullAvailPhys;
    return true;
}
