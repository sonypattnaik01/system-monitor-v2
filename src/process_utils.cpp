#include "process_utils.h"
#include "system_stats.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <algorithm>

static std::wstring TrimExe(const std::wstring& s) {
    return s;
}

ProcessCollector::ProcessCollector() {}

void ProcessCollector::enumerateProcesses(std::vector<ProcessSample>& out,
                                          unsigned long long& sysK,
                                          unsigned long long& sysU,
                                          unsigned long long& sysI) {
    CpuTimes ct{};
    if (GetSystemCpuTimes(ct)) {
        sysK = ct.kernel; sysU = ct.user; sysI = ct.idle;
    } else {
        sysK = sysU = sysI = 0;
    }

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (!Process32FirstW(snap, &pe)) {
        CloseHandle(snap);
        return;
    }

    do {
        ProcessSample ps{};
        ps.pid  = pe.th32ProcessID;
        ps.name = TrimExe(pe.szExeFile);

        HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ps.pid);
        if (h) {
            PROCESS_MEMORY_COUNTERS pmc{};
            if (GetProcessMemoryInfo(h, &pmc, sizeof(pmc))) {
                ps.wsBytes = static_cast<unsigned long long>(pmc.WorkingSetSize);
            }
            FILETIME c, e, k, u;
            if (GetProcessTimes(h, &c, &e, &k, &u)) {
                ps.ktime = FtToULL(k);
                ps.utime = FtToULL(u);
            }
            CloseHandle(h);
        }

        out.push_back(std::move(ps));
    } while (Process32NextW(snap, &pe));

    CloseHandle(snap);
}

std::vector<ProcessSample> ProcessCollector::SnapshotAndCompute(double& totalCpuPercent) {
    std::vector<ProcessSample> list;
    unsigned long long sysK=0, sysU=0, sysI=0;
    enumerateProcesses(list, sysK, sysU, sysI);

    if (havePrev_) {
        const auto totalDelta = (sysK - prevSysKernel_) + (sysU - prevSysUser_);
        const auto idleDelta  = (sysI - prevSysIdle_);
        const auto denom = totalDelta ? totalDelta : 1ULL;

        double busy = static_cast<double>(totalDelta - idleDelta);
        totalCpuPercent = (busy / static_cast<double>(denom)) * 100.0;
        if (totalCpuPercent < 0.0) totalCpuPercent = 0.0;
        if (totalCpuPercent > 100.0) totalCpuPercent = 100.0;

        for (auto& p : list) {
            const auto cur = p.ktime + p.utime;
            const auto it  = prevProcTime_.find(p.pid);
            const auto prev = (it == prevProcTime_.end()) ? cur : it->second;
            const auto delta = (cur >= prev) ? (cur - prev) : 0ULL;

            p.cpuPercent = (static_cast<double>(delta) / static_cast<double>(denom)) * 100.0;
            if (p.cpuPercent < 0.0) p.cpuPercent = 0.0;
        }
    } else {
        totalCpuPercent = 0.0;
    }

    prevProcTime_.clear();
    for (const auto& p : list) prevProcTime_[p.pid] = p.ktime + p.utime;
    prevSysKernel_ = sysK; prevSysUser_ = sysU; prevSysIdle_ = sysI;
    havePrev_ = true;

    auto cmp = [&](const ProcessSample& a, const ProcessSample& b) {
        switch (sortKey_) {
            case SortKey::Cpu:    return descending_ ? (a.cpuPercent > b.cpuPercent) : (a.cpuPercent < b.cpuPercent);
            case SortKey::Memory: return descending_ ? (a.wsBytes    > b.wsBytes)    : (a.wsBytes    < b.wsBytes);
            case SortKey::Pid:    return descending_ ? (a.pid        > b.pid)        : (a.pid        < b.pid);
            case SortKey::Name:   return descending_ ? (a.name       > b.name)       : (a.name       < b.name);
        }
        return false;
    };
    std::sort(list.begin(), list.end(), cmp);
    return list;
}

void ProcessCollector::SetSort(SortKey key, bool descending) {
    sortKey_ = key;
    descending_ = descending;
}

bool ProcessCollector::KillPid(DWORD pid, unsigned timeoutMs) {
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h) return false;
    BOOL ok = TerminateProcess(h, 1);
    if (ok) WaitForSingleObject(h, timeoutMs);
    CloseHandle(h);
    return ok == TRUE;
}
