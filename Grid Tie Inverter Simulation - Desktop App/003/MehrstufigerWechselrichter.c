#include "inverter.h"

void npc_inverter_output(InverterParams *params, double time, double *output) {
    double peak_voltage = params->voltage * sqrt(2); // Convert RMS to peak
    double angle = fmod(2 * M_PI * params->frequency * time + params->phase, 2 * M_PI);
    // 3-level NPC: -V, 0, +V
    double level;
    if (angle < M_PI / 3 || angle >= 5 * M_PI / 3) {
        level = -peak_voltage; // Negative peak
    } else if (angle >= 2 * M_PI / 3 && angle < 4 * M_PI / 3) {
        level = peak_voltage; // Positive peak
    } else {
        level = 0.0; // Zero level
    }
    output[0] = level;
    output[1] = 0.0;
    output[2] = 0.0;
}

void flying_capacitor_output(InverterParams *params, double time, double *output) {
    double peak_voltage = params->voltage * sqrt(2); // Convert RMS to peak
    double angle = fmod(2 * M_PI * params->frequency * time + params->phase, 2 * M_PI);
    // 5-level Flying Capacitor: -V, -V/2, 0, V/2, V
    double level;
    if (angle < M_PI / 5 || angle >= 9 * M_PI / 5) {
        level = -peak_voltage;
    } else if (angle < 2 * M_PI / 5) {
        level = -peak_voltage / 2;
    } else if (angle < 3 * M_PI / 5 || angle >= 7 * M_PI / 5) {
        level = 0.0;
    } else if (angle < 4 * M_PI / 5) {
        level = peak_voltage / 2;
    } else {
        level = peak_voltage;
    }
    output[0] = level;
    output[1] = 0.0;
    output[2] = 0.0;
}

void cascaded_h_bridge_output(InverterParams *params, double time, double *output) {
    double peak_voltage = params->voltage * sqrt(2); // Convert RMS to peak
    // 3-level per phase for simplicity (can be extended to more levels)
    double angles[3] = {params->phase, params->phase + 2 * M_PI / 3, params->phase + 4 * M_PI / 3};
    for (int i = 0; i < 3; i++) {
        double angle = fmod(2 * M_PI * params->frequency * time + angles[i], 2 * M_PI);
        if (angle < M_PI / 3 || angle >= 5 * M_PI / 3) {
            output[i] = -peak_voltage;
        } else if (angle >= 2 * M_PI / 3 && angle < 4 * M_PI / 3) {
            output[i] = peak_voltage;
        } else {
            output[i] = 0.0;
        }
    }
}