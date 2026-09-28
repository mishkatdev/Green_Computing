#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <windows.h>

#include "cpu.h"
#include "memory.h"
#include "system.h"
#include "energy.h"

using namespace std;

struct Config {
    double electricityRate = 10.40;       // Tk per kWh
    double emissionFactor = 0.5;        // kg CO2 per kWh
    double idlePower = 45.0;            // estimated whole-PC idle power
    double cpuMaxAdditional = 70.0;      // estimated extra watts at 100% CPU
    double ramMaxAdditional = 15.0;      // estimated extra watts at 100% RAM
    double idleCpuThreshold = 10.0;      // percent
    int idleMinutes = 5;
    int refreshMs = 1000;
};

bool loadConfig(const string& filename, Config& cfg)
{
    ifstream file(filename);
    if (!file)
        return false;

    string line;
    while (getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        size_t pos = line.find('=');
        if (pos == string::npos)
            continue;

        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);

        try
        {
            if (key == "electricity_rate")
                cfg.electricityRate = stod(value);
            else if (key == "emission_factor")
                cfg.emissionFactor = stod(value);
            else if (key == "idle_power")
                cfg.idlePower = stod(value);
            else if (key == "cpu_max_additional")
                cfg.cpuMaxAdditional = stod(value);
            else if (key == "ram_max_additional")
                cfg.ramMaxAdditional = stod(value);
            else if (key == "idle_cpu_threshold")
                cfg.idleCpuThreshold = stod(value);
            else if (key == "idle_minutes")
                cfg.idleMinutes = stoi(value);
            else if (key == "refresh_ms")
                cfg.refreshMs = stoi(value);
        }
        catch (...)
        {
            // Ignore malformed values and keep defaults.
        }
    }

    if (cfg.refreshMs < 250)
        cfg.refreshMs = 250;

    return true;
}

void clearScreen()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi{};

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        system("cls");
        return;
    }

    DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    COORD home{0, 0};
    DWORD written = 0;

    FillConsoleOutputCharacterA(hConsole, ' ', cellCount, home, &written);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, home, &written);
    SetConsoleCursorPosition(hConsole, home);
}

string formatUptime(unsigned long long milliseconds)
{
    unsigned long long totalSeconds = milliseconds / 1000ULL;

    unsigned long long days = totalSeconds / 86400ULL;
    totalSeconds %= 86400ULL;

    unsigned long long hours = totalSeconds / 3600ULL;
    totalSeconds %= 3600ULL;

    unsigned long long minutes = totalSeconds / 60ULL;
    unsigned long long seconds = totalSeconds % 60ULL;

    ostringstream out;

    if (days > 0)
        out << days << "d ";

    out << setfill('0') << setw(2) << hours << ":"
        << setw(2) << minutes << ":"
        << setw(2) << seconds;

    return out.str();
}

void ensureHistoryFile()
{
    ifstream check("history.csv");

    if (!check.good())
    {
        ofstream file("history.csv");
        file << "timestamp,cpu_percent,ram_percent,estimated_power_w,"
             << "session_energy_kwh,session_cost_tk,session_co2_kg\n";
    }
}

void appendHistory(double cpu,
                   double ram,
                   double power,
                   double energy,
                   double cost,
                   double co2)
{
    ofstream file("history.csv", ios::app);

    if (!file)
        return;

    SYSTEMTIME t;
    GetLocalTime(&t);

    file << setfill('0')
         << t.wYear << "-"
         << setw(2) << t.wMonth << "-"
         << setw(2) << t.wDay << " "
         << setw(2) << t.wHour << ":"
         << setw(2) << t.wMinute << ":"
         << setw(2) << t.wSecond << ","
         << fixed << setprecision(2)
         << cpu << ","
         << ram << ","
         << power << ","
         << energy << ","
         << cost << ","
         << co2 << "\n";
}

int calculateGreenScore(double cpu, double ram, double power)
{
    // Simple transparent score for this project:
    // lower average resource usage and lower estimated power = higher score.
    double resource = (cpu + ram) / 2.0;
    double powerPenalty = min(100.0, max(0.0, (power - 45.0) / 1.0));

    double score = 100.0 - (resource * 0.45) - (powerPenalty * 0.25);

    if (score < 0.0) score = 0.0;
    if (score > 100.0) score = 100.0;

    return static_cast<int>(score + 0.5);
}

void printBar(const string& label, double value)
{
    const int width = 25;
    int filled = static_cast<int>((value / 100.0) * width);

    if (filled < 0) filled = 0;
    if (filled > width) filled = width;

    cout << left << setw(8) << label << "[";

    for (int i = 0; i < width; ++i)
        cout << (i < filled ? '#' : '-');

    cout << "] " << fixed << setprecision(1) << value << "%\n";
}

int main()
{
    SetConsoleTitleA("Green PC Monitor");

    Config cfg;
    if (!loadConfig("config.txt", cfg))
    {
        cout << "config.txt not found. Using built-in default settings.\n";
        Sleep(1500);
    }

    ensureHistoryFile();

    double sessionEnergy = 0.0;
    auto lastUpdate = chrono::steady_clock::now();

    int idleCounter = 0;
    int idleLimitSamples =
        max(1, (cfg.idleMinutes * 60 * 1000) / cfg.refreshMs);

    while (true)
    {
        double cpu = getCPUUsage();
        double ram = getMemoryUsage();

        unsigned long long uptimeMs = getUptime();
        unsigned long processCount = getProcessCount();

        double power = calculatePower(
            cpu,
            ram,
            cfg.idlePower,
            cfg.cpuMaxAdditional,
            cfg.ramMaxAdditional
        );

        auto now = chrono::steady_clock::now();
        double elapsedSeconds =
            chrono::duration<double>(now - lastUpdate).count();

        lastUpdate = now;

        // kWh = watts × hours / 1000
        sessionEnergy += (power * elapsedSeconds) / 3600000.0;

        double cost = calculateCost(sessionEnergy, cfg.electricityRate);
        double co2 = calculateCO2(sessionEnergy, cfg.emissionFactor);

        int greenScore = calculateGreenScore(cpu, ram, power);

        bool idle = cpu < cfg.idleCpuThreshold;

        if (idle)
            ++idleCounter;
        else
            idleCounter = 0;

        clearScreen();

        cout << "============================================================\n";
        cout << "                  GREEN PC MONITOR v1.0\n";
        cout << "============================================================\n\n";

        printBar("CPU", cpu);
        printBar("RAM", ram);

        cout << "\n------------------------------------------------------------\n";
        cout << " SYSTEM\n";
        cout << "------------------------------------------------------------\n";

        cout << left << setw(24) << "Uptime"
             << ": " << formatUptime(uptimeMs) << "\n";

        cout << left << setw(24) << "Running Processes"
             << ": " << processCount << "\n";

        cout << "\n------------------------------------------------------------\n";
        cout << " ENERGY ESTIMATION\n";
        cout << "------------------------------------------------------------\n";

        cout << fixed << setprecision(2);

        cout << left << setw(24) << "Estimated Power"
             << ": " << power << " W\n";

        cout << left << setw(24) << "Session Energy"
             << ": " << sessionEnergy << " kWh\n";

        cout << left << setw(24) << "Estimated Cost"
             << ": Tk " << cost << "\n";

        cout << left << setw(24) << "Estimated CO2"
             << ": " << co2 << " kg\n";

        cout << "\n------------------------------------------------------------\n";
        cout << " GREEN STATUS\n";
        cout << "------------------------------------------------------------\n";

        cout << left << setw(24) << "Green Score"
             << ": " << greenScore << "/100\n";

        if (idleCounter >= idleLimitSamples)
        {
            cout << "Status              : IDLE\n";
            cout << "Recommendation      : PC has been idle for about "
                 << cfg.idleMinutes << " minutes.\n";
            cout << "                      Consider sleep mode when appropriate.\n";
        }
        else if (cpu > 80.0)
        {
            cout << "Status              : HIGH CPU ACTIVITY\n";
            cout << "Recommendation      : Check high-CPU applications if this\n";
            cout << "                      workload is not intentional.\n";
        }
        else if (ram > 85.0)
        {
            cout << "Status              : HIGH MEMORY USAGE\n";
            cout << "Recommendation      : Close unnecessary applications/tabs.\n";
        }
        else
        {
            cout << "Status              : NORMAL\n";
            cout << "Recommendation      : Continue using power-saving settings.\n";
        }

        cout << "\n------------------------------------------------------------\n";
        cout << " Settings: " << cfg.electricityRate << " Tk/kWh | "
             << cfg.emissionFactor << " kg CO2/kWh | "
             << cfg.refreshMs << " ms refresh\n";

        cout << " History: history.csv\n";
        cout << " Press Ctrl+C to exit.\n";
        cout << "============================================================\n";

        // Save one sample each refresh.
        appendHistory(cpu, ram, power, sessionEnergy, cost, co2);

        this_thread::sleep_for(chrono::milliseconds(cfg.refreshMs));
    }

    return 0;
}
