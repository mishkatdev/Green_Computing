#include "energy.h"

double calculatePower(
    double cpuUsage,
    double ramUsage,
    double idlePower,
    double cpuMaxAdditional,
    double ramMaxAdditional)
{
    double cpuContribution =
        (cpuUsage / 100.0) * cpuMaxAdditional;

    double ramContribution =
        (ramUsage / 100.0) * ramMaxAdditional;

    return idlePower + cpuContribution + ramContribution;
}

double calculateEnergy(double power, double hours)
{
    return (power / 1000.0) * hours;
}

double calculateCost(double energy, double electricityRate)
{
    return energy * electricityRate;
}

double calculateCO2(double energy, double emissionFactor)
{
    return energy * emissionFactor;
}
