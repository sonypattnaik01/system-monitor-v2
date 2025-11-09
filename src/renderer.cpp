#include "renderer.h"
#include <iostream>
#include <iomanip>
#include <conio.h>
#include <sstream>
#include <algorithm>

void InitConsoleVT() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
}

std::string ToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), size, nullptr, nullptr);
    return s;
}

std::string HumanBytes(unsigned long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 4) { v /= 1024.0; ++u; }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision((u <= 1) ? 0 : 1) << v << " " << units[u];
    return oss.str();
}

// --- Graph helpers ---
std::string Bar(double percent, int width, const std::string& color) {
    int filled = static_cast<int>((percent / 100.0) * width);
    std::ostringstream oss;
    oss << color;
    for (int i = 0; i < width; ++i)
        oss << (i < filled ? "#" : " ");
    oss << "\x1b[0m";
    return oss.str();
}

std::string Sparkline(const std::deque<double>& history, int height = 6, int width = 40, const std::string& color = "\x1b[32m") {
    if (history.empty()) return "";
    std::ostringstream oss;
    double maxVal = 100.0;
    double minVal = 0.0;

    // Create 6 rows of graph (height)
    for (int row = height - 1; row >= 0; --row) {
        double threshold = (static_cast<double>(row) / height) * 100.0;
        oss << color;
        for (auto v : history) {
            if (v >= threshold)
                oss << "█";
            else
                oss << " ";
        }
        oss << "\x1b[0m\n";
    }
    return oss.str();
}

static const char* SortKeyName(SortKey k) {
    switch (k) {
        case SortKey::Cpu:    return "CPU";
        case SortKey::Memory: return "Memory";
        case SortKey::Pid:    return "PID";
        case SortKey::Name:   return "Name";
    }
    return "?";
}

void DrawScreen(const std::vector<ProcessSample>& rows,
                double totalCpu,
                const MemoryInfo& mem,
                SortKey key,
                bool descending,
                std::deque<double>& cpuHistory,
                std::deque<double>& memHistory,
                size_t maxRows) {
    std::cout << "\x1b[2J\x1b[H"; // clear

    // keep history up to 40 samples
    const int maxSamples = 40;
    if (cpuHistory.size() >= maxSamples) cpuHistory.pop_front();
    if (memHistory.size() >= maxSamples) memHistory.pop_front();
    cpuHistory.push_back(totalCpu);
    double memPercent = (double)mem.usedPhys / (double)mem.totalPhys * 100.0;
    memHistory.push_back(memPercent);

    // HEADER
    std::cout << "\x1b[1;36m=== System Monitor (Console UI) ===\x1b[0m\n";
    std::cout << "CPU: " << std::fixed << std::setprecision(1) << totalCpu << "% | Memory: "
              << HumanBytes(mem.usedPhys) << " / " << HumanBytes(mem.totalPhys)
              << " (" << std::setprecision(1) << memPercent << "%)\n\n";

    // GRAPHS
    std::cout << "\x1b[33mCPU usage history:\x1b[0m\n";
    std::cout << Sparkline(cpuHistory, 6, 40, "\x1b[32m");
    std::cout << "\x1b[35mMem usage history:\x1b[0m\n";
    std::cout << Sparkline(memHistory, 6, 40, "\x1b[35m");

    std::cout << "\n\x1b[1mPID     CPU%    Memory      Name\x1b[0m\n";
    std::cout << std::string(8+8+12+35, '-') << "\n";

    size_t shown = 0;
    for (const auto& p : rows) {
        if (shown++ >= maxRows) break;
        std::string name = ToUtf8(p.name);
        if (name.size() > 33) name = name.substr(0, 30) + "...";

        std::string color = (p.cpuPercent > 70) ? "\x1b[31m" :
                            (p.cpuPercent > 40) ? "\x1b[33m" : "\x1b[32m";

        std::cout << color
                  << std::left << std::setw(8)  << p.pid
                  << std::setw(8)  << std::fixed << std::setprecision(1) << p.cpuPercent
                  << std::setw(12) << HumanBytes(p.wsBytes)
                  << std::setw(35) << name
                  << "\x1b[0m\n";
    }

    std::cout << "\nSorted by: " << SortKeyName(key) << (descending ? " (desc)" : " (asc)") << "\n";
    std::cout << "Keys: [C]PU [M]EM [P]ID [N]AME | [R]everse | [K]ill | [Q]uit\n";
}

bool PromptAndKill(ProcessCollector& pc) {
    std::cout << "\nEnter PID to kill (blank to cancel): ";
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) return false;

    DWORD pid = static_cast<DWORD>(std::stoul(line));
    bool ok = pc.KillPid(pid);
    if (!ok) {
        std::cout << "\x1b[31mFailed to kill PID " << pid << "\x1b[0m\n";
    } else {
        std::cout << "\x1b[32mTerminated PID " << pid << "\x1b[0m\n";
    }
    std::cout << "Press any key to continue...\n";
    (void)_getch();
    return ok;
}
