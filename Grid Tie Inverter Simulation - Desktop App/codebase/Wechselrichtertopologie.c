#include "inverter.h"

void single_phase_output(InverterParams *params, double time, double *output) {
    double peak_voltage = params->voltage * sqrt(2); // Convert RMS to peak
    output[0] = peak_voltage * sin(2 * M_PI * params->frequency * time + params->phase);
    output[1] = 0.0;
    output[2] = 0.0;
}

void three_phase_output(InverterParams *params, double time, double *output) {
    double peak_voltage = params->voltage * sqrt(2); // Convert RMS to peak
    output[0] = peak_voltage * sin(2 * M_PI * params->frequency * time + params->phase);
    output[1] = peak_voltage * sin(2 * M_PI * params->frequency * time + params->phase + 2 * M_PI / 3);
    output[2] = peak_voltage * sin(2 * M_PI * params->frequency * time + params->phase + 4 * M_PI / 3);
}