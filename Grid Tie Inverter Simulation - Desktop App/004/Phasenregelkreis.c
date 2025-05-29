#include "inverter.h"

// Simulated grid voltage with variable frequency and amplitude
static double grid_voltage(double time, double *frequency, double *amplitude) {
    // Simulate grid variations (e.g., frequency 49–51 Hz, amplitude 210–230V RMS)
    *frequency = 50.0 + sin(time * 0.1) * 1.0; // Vary ±1 Hz
    *amplitude = 220.0 + sin(time * 0.2) * 10.0; // Vary ±10V RMS
    return *amplitude * sqrt(2) * sin(2 * M_PI * *frequency * time);
}

void pll_update(InverterParams *params, double time) {
    // PLL parameters
    const double dt = 0.05; // Time step (50ms, 20 FPS)
    static double integral = 0.0; // Persistent integral term
    static double prev_grid_v = 0.0; // Previous grid voltage for zero-crossing
    static double last_zero_cross = 0.0; // Time of last zero-crossing
    static int zero_cross_count = 0; // Count zero-crossings for frequency estimation

    // Get grid voltage and parameters
    double grid_freq, grid_ampl;
    double v_grid = grid_voltage(time, &grid_freq, &grid_ampl);
    double v_inv = params->voltage * sqrt(2) * sin(2 * M_PI * params->frequency * time + params->pll_phase);

    // Frequency estimation via zero-crossing detection
    if (prev_grid_v <= 0 && v_grid > 0) { // Positive zero-crossing
        zero_cross_count++;
        if (zero_cross_count >= 2) { // Estimate frequency after two crossings
            double period = (time - last_zero_cross) / (zero_cross_count - 1);
            params->pll_frequency = 1.0 / period;
            zero_cross_count = 1; // Reset for next estimation
            last_zero_cross = time;
        }
    }
    prev_grid_v = v_grid;

    // Voltage tracking: slowly adjust to grid amplitude
    params->pll_voltage += 0.1 * (grid_ampl - params->pll_voltage) * dt; // Low-pass filter effect

    // Phase detector: multiply signals
    double error = v_grid * v_inv; // Proportional to phase difference

    // PI controller
    integral += error * dt;
    double phase_correction = params->pll_kp * error + params->pll_ki * integral;

    // Update PLL phase
    params->pll_phase += phase_correction * dt;
    params->pll_phase = fmod(params->pll_phase, 2 * M_PI);
    if (params->pll_phase < 0) params->pll_phase += 2 * M_PI;

    // Update lock status (locked if phase error is small)
    params->pll_locked = fabs(error) < 0.1 * grid_ampl * params->voltage * sqrt(2);
}