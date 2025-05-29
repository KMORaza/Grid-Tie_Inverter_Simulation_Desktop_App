#include "inverter.h"

// Simplified PV model: returns power based on voltage
double pv_model_power(double voltage) {
    // Simulate I-V curve: I = Isc - k*V, P = V*I
    const double Isc = 8.0; // Short-circuit current (A)
    const double Voc = 300.0; // Open-circuit voltage (V)
    const double k = Isc / Voc; // Linear approximation
    if (voltage < 0 || voltage > Voc) return 0.0;
    double current = Isc - k * voltage;
    return voltage * current; // Power = V * I
}

void mppt_perturb_observe(InverterParams *params) {
    const double perturb_step = 1.0; // Voltage step size (V)
    double current_voltage = params->mppt_voltage;
    double current_power = pv_model_power(current_voltage);

    // Compare with previous state
    double power_diff = current_power - params->prev_power;
    double voltage_diff = current_voltage - params->prev_voltage;

    // P&O logic
    if (power_diff > 0) {
        // Power increased: continue in same direction
        if (voltage_diff > 0) {
            params->mppt_voltage += perturb_step;
        } else {
            params->mppt_voltage -= perturb_step;
        }
    } else {
        // Power decreased: reverse direction
        if (voltage_diff > 0) {
            params->mppt_voltage -= perturb_step;
        } else {
            params->mppt_voltage += perturb_step;
        }
    }

    // Clamp voltage within bounds
    if (params->mppt_voltage < 100.0) params->mppt_voltage = 100.0;
    if (params->mppt_voltage > 300.0) params->mppt_voltage = 300.0;

    // Update previous values
    params->prev_power = current_power;
    params->prev_voltage = current_voltage;
}

void mppt_incremental_conductance(InverterParams *params) {
    const double step_size = 1.0; // Voltage step size (V)
    double voltage = params->mppt_voltage;
    double power = pv_model_power(voltage);
    double current = power / voltage; // I = P / V

    // Approximate derivatives
    double delta_v = voltage - params->prev_voltage;
    double delta_i = (power / voltage) - (params->prev_power / params->prev_voltage);
    double dI_dV = delta_v != 0 ? delta_i / delta_v : 0.0;
    double I_V = current / voltage;

    // IncCond logic
    if (fabs(dI_dV + I_V) < 0.01) {
        // At MPP (dI/dV = -I/V)
        // No change
    } else if (dI_dV > -I_V) {
        // Left of MPP: increase voltage
        params->mppt_voltage += step_size;
    } else {
        // Right of MPP: decrease voltage
        params->mppt_voltage -= step_size;
    }

    // Clamp voltage within bounds
    if (params->mppt_voltage < 100.0) params->mppt_voltage = 100.0;
    if (params->mppt_voltage > 300.0) params->mppt_voltage = 300.0;

    // Update previous values
    params->prev_power = power;
    params->prev_voltage = voltage;
}