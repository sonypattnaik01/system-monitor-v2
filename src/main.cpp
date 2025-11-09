#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <conio.h>
#include <algorithm> // for std::max

#include "process_utils.h"
#include "system_stats.h"
#include "renderer.h"

std::deque<double> cpuHistory;
std::deque<double> memHistory;

int main(int argc, char **argv)
{
    int intervalSec = 2;
    if (argc >= 2)
    {
        try
        {
            intervalSec = std::max(1, std::stoi(argv[1]));
        }
        catch (...)
        {
        }
    }

    InitConsoleVT();

    ProcessCollector pc;
    MemoryInfo mem{};
    double totalCpu = 0.0;

    bool running = true;
    while (running)
    {
        auto rows = pc.SnapshotAndCompute(totalCpu);
        GetMemoryInfo(mem);

        DrawScreen(rows, totalCpu, mem, pc.GetSortKey(), pc.IsDescending(), 25);

        auto start = std::chrono::steady_clock::now();
        while (true)
        {
            if (_kbhit())
            {
                int ch = _getch();
                switch (ch)
                {
                case 'q':
                case 'Q':
                    running = false;
                    break;
                case 'c':
                case 'C':
                    pc.SetSort(SortKey::Cpu, true);
                    break;
                case 'm':
                case 'M':
                    pc.SetSort(SortKey::Memory, true);
                    break;
                case 'p':
                case 'P':
                    pc.SetSort(SortKey::Pid, true);
                    break;
                case 'n':
                case 'N':
                    pc.SetSort(SortKey::Name, true);
                    break;
                case 'r':
                case 'R':
                    pc.SetSort(pc.GetSortKey(), !pc.IsDescending());
                    break;
                case 'k':
                case 'K':
                    std::cout << "\n";
                    PromptAndKill(pc);
                    break;
                }
                break; // re-render after key
            }
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= intervalSec)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    std::cout << "Bye.\n";
    return 0;
}
