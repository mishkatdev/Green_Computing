# Green PC Monitor

A Windows terminal-based C++ project that monitors real-time system usage and estimates energy consumption and environmental impact.

## Features

- Real-time CPU usage
- Real-time RAM usage
- Windows uptime
- Running process count
- Estimated power consumption
- Session energy consumption
- Estimated electricity cost
- Estimated CO2 emissions
- Simple green score
- Idle/high-resource recommendations
- Configurable settings
- CSV monitoring history

## Project structure

```text
GreenPCMonitor/
│
├── main.cpp
├── cpu.cpp
├── cpu.h
├── memory.cpp
├── memory.h
├── system.cpp
├── system.h
├── energy.cpp
├── energy.h
├── config.txt
└── README.md
```

## Requirements

- Windows 10 or Windows 11
- C++17-compatible compiler
- MinGW/MSYS2 or another Windows C++ compiler
- VS Code is recommended but not required

## Compile with MinGW/MSYS2

Open a terminal inside the project directory:

```bash
g++ -std=c++17 main.cpp cpu.cpp memory.cpp system.cpp energy.cpp -o GreenPCMonitor.exe
```

Run:

```bash
./GreenPCMonitor.exe
```

## Important note about power

The program uses a simple software model based on CPU and RAM usage to estimate power. It does NOT directly measure the electrical power being drawn from the wall.

For actual whole-PC wattage, an external power meter/smart plug would be required.

## Configuration

Edit `config.txt` to change:

- electricity rate
- CO2 emission factor
- estimated idle power
- estimated CPU/RAM power contribution
- idle threshold
- refresh interval

## History

The program automatically creates:

```text
history.csv
```

The file stores timestamped monitoring samples that can later be analyzed in Excel, Python, or another data-analysis tool.

## Green computing concept

The project demonstrates how real-time computer resource information can be used to estimate energy consumption and provide energy-awareness recommendations.
