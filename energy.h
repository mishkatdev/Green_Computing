#ifndef ENERGY_H
#define ENERGY_H

double calculatePower(
    double cpuUsage,
    double ramUsage,
    double idlePower,
    double cpuMaxAdditional,
    double ramMaxAdditional
);

double calculateEnergy(double power, double hours);
double calculateCost(double energy, double electricityRate);
double calculateCO2(double energy, double emissionFactor);

#endif
