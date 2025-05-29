#include "inverter.h"

// Simplified plant model: RL load + grid
static double plant_model(double duty, double time, double *current, double grid_voltage, double params_voltage, double params_frequency, double params_phase) {
    const double R = 10.0; // Load resistance (Ohms)
    const double L = 0.01; // Load inductance (H)
    const double dt = 0.05; // Time step (50ms)
    static double i_prev = 0.0; // Previous current

    // Inverter output voltage (based on duty cycle)
    double v_inv = duty * params_voltage * sqrt(2) * sin(2 * M_PI * params_frequency * time + params_phase);
    // Grid voltage
    double v_grid = grid_voltage * sqrt(2) * sin(2 * M_PI * params_frequency * time);
    // Differential equation: L*di/dt + R*i = v_inv - v_grid
    double di_dt = (v_inv - v_grid - R * (*current)) / L;
    *current = i_prev + di_dt * dt; // Euler integration
    i_prev = *current;
    return *current;
}

void control_update(InverterParams *params, double time) {
    const double dt = 0.05; // Time step (50ms)
    static double integral = 0.0; // PI/PR integral term
    static double prev_error = 0.0; // For PR resonant term
    double current = 0.0; // Simulated current
    double grid_voltage = params->pll_enabled ? params->pll_voltage : 220.0;

    // Reference signals
    double ref_current = params->control_ref_current * sin(2 * M_PI * params->frequency * time + params->phase);
    double ref_voltage = params->control_ref_voltage * sin(2 * M_PI * params->frequency * time + params->phase);
    // Measured current (from plant model with current duty)
    double meas_current = plant_model(params->control_output, time, &current, grid_voltage, params->voltage, params->frequency, params->phase);
    double error = ref_current - meas_current; // Current control

    double control_signal = 0.0;
    switch (params->control) {
        case CONTROL_PI: {
            // PI control: u = kp*e + ki*∫e
            const double kp = 0.1;
            const double ki = 5.0;
            integral += error * dt;
            control_signal = kp * error + ki * integral;
            break;
        }
        case CONTROL_PR: {
            // PR control: u = kp*e + ki*∫e + kr*e_resonant
            const double kp = 0.1;
            const double ki = 5.0;
            const double kr = 50.0;
            const double w = 2 * M_PI * params->frequency;
            integral += error * dt;
            double resonant = kr * sin(w * time) * error; // Simplified resonant term
            control_signal = kp * error + ki * integral + resonant;
            break;
        }
        case CONTROL_SMC: {
            // Sliding Mode Control: s = e + c*de/dt
            const double c = 0.01;
            const double k = 0.5;
            double de_dt = (error - prev_error) / dt;
            double s = error + c * de_dt; // Sliding surface
            control_signal = k * (s > 0 ? 1.0 : -1.0); // Bang-bang control
            prev_error = error;
            break;
        }
        case CONTROL_MPC: {
            // Model Predictive Control: minimize cost over horizon
            const double horizon = 0.1; // 100ms
            const int steps = 2; // Prediction steps
            double min_cost = 1e6;
            double best_duty = params->control_output;
            for (int i = 0; i <= 10; i++) { // Test duty cycles 0 to 1
                double test_duty = i / 10.0;
                double cost = 0.0;
                double temp_current = current;
                for (int j = 0; j < steps; j++) {
                    double t_future = time + j * dt;
                    double i_future = plant_model(test_duty, t_future, &temp_current, grid_voltage, params->voltage, params->frequency, params->phase);
                    double ref_future = params->control_ref_current * sin(2 * M_PI * params->frequency * t_future + params->phase);
                    cost += (ref_future - i_future) * (ref_future - i_future);
                }
                if (cost < min_cost) {
                    min_cost = cost;
                    best_duty = test_duty;
                }
            }
            control_signal = best_duty;
            break;
        }
        default:
            control_signal = 1.0; // No control
            break;
    }

    // Clamp control signal (duty cycle 0 to 1)
    params->control_output = control_signal;
    if (params->control_output < 0.0) params->control_output = 0.0;
    if (params->control_output > 1.0) params->control_output = 1.0;
}