#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>

struct ProcessSample {
    DWORD pid = 0;
    std::wstring name;
    unsigned long long wsBytes = 0; // working set bytes
    double cpuPercent = 0.0;
    unsigned long long ktime = 0;   // raw kernel time
    unsigned long long utime = 0;   // raw user time
};

enum class SortKey { Cpu, Memory, Pid, Name };

class ProcessCollector {
public:
    ProcessCollector();

    // Take snapshot and compute CPU% using deltas
    std::vector<ProcessSample> SnapshotAndCompute(double& totalCpuPercent);

    void SetSort(SortKey key, bool descending);
    SortKey GetSortKey() const { return sortKey_; }
    bool IsDescending() const { return descending_; }

    // Kill a process by PID
    bool KillPid(DWORD pid, unsigned timeoutMs = 3000);

private:
    std::unordered_map<DWORD, unsigned long long> prevProcTime_; // sum of k+u
    unsigned long long prevSysKernel_ = 0;
    unsigned long long prevSysUser_   = 0;
    unsigned long long prevSysIdle_   = 0;
    bool havePrev_ = false;

    SortKey sortKey_ = SortKey::Cpu;
    bool descending_ = true;

    void enumerateProcesses(std::vector<ProcessSample>& out,
                            unsigned long long& sysK,
                            unsigned long long& sysU,
                            unsigned long long& sysI);
};
