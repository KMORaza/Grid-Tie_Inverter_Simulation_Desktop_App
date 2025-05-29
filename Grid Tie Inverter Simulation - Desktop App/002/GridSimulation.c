#include "inverter.h"
#include <stdlib.h>
#include <time.h>

double grid_simulation_voltage(InverterParams *params, double t, double inverter_current, double *frequency, double *amplitude, gboolean *grid_connected) {
    static gboolean initialized = FALSE;
    if (!initialized) {
        srand((unsigned int)time(NULL));
        initialized = TRUE;
    }

    // Grid impedance (R + jX)
    double R, L, X;
    if (params->grid_condition == GRID_WEAK) {
        R = 1.0; // Ω, higher for weak grid
        L = 0.005; // 5 mH
    } else {
        R = 0.4; // Ω, normal grid
        L = 0.001; // 1 mH
    }
    X = 2 * M_PI * 50.0 * L; // Reactance at 50 Hz

    // Nominal grid voltage and frequency
    double V_nom = 220.0; // RMS
    double f_nom = 50.0; // Hz
    *amplitude = V_nom;
    *frequency = f_nom;

    // Random grid disconnection (5% chance per second)
    if (rand() % 100 < 5 && t > 1.0) {
        *grid_connected = FALSE;
    }

    if (!*grid_connected) {
        *frequency = 50.0; // Local load frequency
        *amplitude = 220.0; // Local load voltage
        return *amplitude * sqrt(2) * sin(2 * M_PI * *frequency * t);
    }

    // Grid faults
    double fault_factor = 1.0;
    double freq_shift = 0.0;
    double harmonic = 0.0;
    if (params->grid_condition == GRID_FAULT_SAG && t >= 1.0 && t <= 1.5) {
        fault_factor = 0.5; // 50% voltage sag
    } else if (params->grid_condition == GRID_FAULT_SWELL && t >= 1.0 && t <= 1.5) {
        fault_factor = 1.2; // 120% voltage swell
    } else if (params->grid_condition == GRID_FAULT_HARMONICS) {
        harmonic = 0.05 * sin(3 * 2 * M_PI * f_nom * t) + // 3rd harmonic
                   0.03 * sin(5 * 2 * M_PI * f_nom * t) + // 5th harmonic
                   0.02 * sin(7 * 2 * M_PI * f_nom * t);  // 7th harmonic
    } else if (params->grid_condition == GRID_FAULT_FREQ_SHIFT && t >= 1.0 && t <= 1.5) {
        freq_shift = 2.0; // +2 Hz shift
    }

    // Grid voltage source
    *amplitude = V_nom * fault_factor;
    *frequency = f_nom + freq_shift;
    double V_grid = *amplitude * sqrt(2) * sin(2 * M_PI * *frequency * t) + harmonic;

    // Voltage drop due to grid impedance: V_pcc = V_grid - I * (R + jX)
    double V_drop = inverter_current * R; // Resistive drop (in-phase)
    V_drop += inverter_current * X * cos(2 * M_PI * *frequency * t); // Reactive drop (90° phase)
    double V_pcc = V_grid - V_drop;

    return V_pcc;
}