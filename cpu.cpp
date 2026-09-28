#include "cpu.h"
#include <windows.h>
#include <algorithm>

namespace
{
    unsigned long long fileTimeToUInt64(const FILETIME& ft)
    {
        ULARGE_INTEGER value;
        value.LowPart = ft.dwLowDateTime;
        value.HighPart = ft.dwHighDateTime;
        return value.QuadPart;
    }
}

double getCPUUsage()
{
    static bool firstCall = true;
    static unsigned long long previousIdle = 0;
    static unsigned long long previousKernel = 0;
    static unsigned long long previousUser = 0;

    FILETIME idleTime, kernelTime, userTime;

    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
        return 0.0;

    unsigned long long idle = fileTimeToUInt64(idleTime);
    unsigned long long kernel = fileTimeToUInt64(kernelTime);
    unsigned long long user = fileTimeToUInt64(userTime);

    if (firstCall)
    {
        firstCall = false;
        previousIdle = idle;
        previousKernel = kernel;
        previousUser = user;
        return 0.0;
    }

    unsigned long long idleDelta = idle - previousIdle;
    unsigned long long kernelDelta = kernel - previousKernel;
    unsigned long long userDelta = user - previousUser;

    previousIdle = idle;
    previousKernel = kernel;
    previousUser = user;

    unsigned long long total = kernelDelta + userDelta;

    if (total == 0)
        return 0.0;

    double usage =
        (static_cast<double>(total - idleDelta) /
         static_cast<double>(total)) * 100.0;

    return std::clamp(usage, 0.0, 100.0);
}
