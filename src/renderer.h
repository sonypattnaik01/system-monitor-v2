#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <vector>
#include <deque>

#include "process_utils.h"
#include "system_stats.h"

// initialize console for colored output
void InitConsoleVT();

// draw everything
void DrawScreen(const std::vector<ProcessSample>& rows,
                double totalCpu,
                const MemoryInfo& mem,
                SortKey key,
                bool descending,
                std::deque<double>& cpuHistory,
                std::deque<double>& memHistory,
                size_t maxRows = 15);

// handle killing process prompt
bool PromptAndKill(ProcessCollector& pc);

// helpers
std::string ToUtf8(const std::wstring& w);
std::string HumanBytes(unsigned long long bytes);
