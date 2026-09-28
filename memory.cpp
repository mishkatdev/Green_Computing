#include "memory.h"
#include <windows.h>

double getMemoryUsage()
{
    MEMORYSTATUSEX memoryStatus{};
    memoryStatus.dwLength = sizeof(memoryStatus);

    if (!GlobalMemoryStatusEx(&memoryStatus))
        return 0.0;

    return static_cast<double>(memoryStatus.dwMemoryLoad);
}
