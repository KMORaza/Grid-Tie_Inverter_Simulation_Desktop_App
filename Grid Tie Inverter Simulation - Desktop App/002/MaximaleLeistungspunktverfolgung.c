#include "inverter.h"

// DC source power calculation
double dc_source_get_power(AppData *app, double voltage, double *current) {
    // Call dc_source_update to get Vdc and Idc
    dc_source_update(app);
    *current = app->params.dc_current;
    // If voltage is specified (e.g., by MPPT), adjust current accordingly
    if (voltage > 0.0 && app->params.dc_voltage > 0.0) {
        *current *= voltage / app->params.dc_voltage;
    }
    return voltage * (*current);
}

// Simplified PV model for compatibility
double pv_model_power(double voltage) {
    // Placeholder: Replaced by dc_source_get_power
    return voltage * 10.0; // 10A constant current (simplified)
}

void mppt_perturb_observe(InverterParams *params) {
    const double delta_v = 1.0; // Voltage perturbation (V)
    double power = params->mppt_voltage * params->control_ref_current; // Current power
    if (power > params->prev_power) {
        // Moving towards MPP, continue in same direction
        params->mppt_voltage += (params->mppt_voltage > params->prev_voltage) ? delta_v : -delta_v;
    } else {
        // Moving away from MPP, reverse direction
        params->mppt_voltage += (params->mppt_voltage > params->prev_voltage) ? -delta_v : delta_v;
    }
    // Update previous values
    params->prev_power = power;
    params->prev_voltage = params->mppt_voltage;
    // Limit voltage
    if (params->mppt_voltage < 100.0) params->mppt_voltage = 100.0;
    if (params->mppt_voltage > 300.0) params->mppt_voltage = 300.0;
}

void mppt_incremental_conductance(InverterParams *params) {
    const double delta_v = 1.0; // Voltage step
    double i = params->control_ref_current;
    double v = params->mppt_voltage;
    double delta_i = i - params->prev_power / params->prev_voltage; // Approximate dI
    double delta_v_actual = v - params->prev_voltage; // dV
    if (fabs(delta_v_actual) > 0.01) {
        double g_inc = delta_i / delta_v_actual; // Incremental conductance
        double g = i / v; // Conductance
        if (fabs(g_inc + g) < 0.01) {
            // At MPP
        } else if (g_inc + g > 0) {
            // Left of MPP
            params->mppt_voltage += delta_v;
        } else {
            // Right of MPP
            params->mppt_voltage -= delta_v;
        }
    } else {
        // Small voltage change, perturb
        params->mppt_voltage += delta_v;
    }
    // Update previous values
    params->prev_power = i * v;
    params->prev_voltage = v;
    // Limit voltage
    if (params->mppt_voltage < 100.0) params->mppt_voltage = 100.0;
    if (params->mppt_voltage > 300.0) params->mppt_voltage = 300.0;
}