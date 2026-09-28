#include "system.h"
#include <windows.h>
#include <tlhelp32.h>

unsigned long long getUptime()
{
    return GetTickCount64();
}

unsigned long getProcessCount()
{
    HANDLE snapshot =
        CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32 entry{};
    entry.dwSize = sizeof(entry);

    unsigned long count = 0;

    if (Process32First(snapshot, &entry))
    {
        do
        {
            ++count;
        }
        while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);

    return count;
}
