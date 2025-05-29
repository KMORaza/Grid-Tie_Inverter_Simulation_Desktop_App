#include "inverter.h"
#include <stdlib.h>
#include <time.h>

void islanding_detection_update(InverterParams *params, double time) {
    static gboolean grid_connected = TRUE;
    static double prev_freq = 50.0;
    static double prev_time = 0.0;
    const double dt = 0.05; // Time step (50ms)

    // Assume inverter current (peak, A) based on control reference
    double inverter_current = params->control_ref_current;

    // Get grid parameters
    double grid_freq, grid_ampl;
    double v_grid = grid_simulation_voltage(params, time, inverter_current, &grid_freq, &grid_ampl, &grid_connected);
    double current_voltage = params->pll_enabled ? params->pll_voltage : params->voltage;
    double current_freq = params->pll_enabled ? params->pll_frequency : params->frequency;

    // Passive: Over/Under Voltage (OUV)
    if (current_voltage < 193.6 || current_voltage > 242.0) { // 0.88–1.1 pu
        params->islanding_detected = TRUE;
        return;
    }

    // Passive: Over/Under Frequency (OUF)
    if (current_freq < 49.0 || current_freq > 51.0) {
        params->islanding_detected = TRUE;
        return;
    }

    // Passive: Rate of Change of Frequency (ROCOF)
    double rocof = (current_freq - prev_freq) / (time - prev_time);
    if (fabs(rocof) > 1.0 && time > 0.1) { // 1 Hz/s threshold
        params->islanding_detected = TRUE;
        return;
    }

    // Active: Frequency Shift (AFS)
    if (grid_connected) {
        // Inject small frequency perturbation (±0.5 Hz)
        params->frequency += 0.5 * sin(2 * M_PI * 0.1 * time);
    } else {
        // In islanded mode, frequency should drift
        if (fabs(current_freq - 50.0) > 0.7) { // Detect drift > 0.7 Hz
            params->islanding_detected = TRUE;
            return;
        }
    }

    params->islanding_detected = FALSE;
    prev_freq = current_freq;
    prev_time = time;
}