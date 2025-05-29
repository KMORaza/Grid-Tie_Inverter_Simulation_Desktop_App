#include "inverter.h"

void inverter_init(InverterParams *params) {
    params->voltage = 220.0; // Default 220V RMS
    params->frequency = 50.0; // Default 50 Hz
    params->phase = 0.0; // Default phase
    params->running = FALSE;
    params->type = SINGLE_PHASE; // Default to single-phase
    params->design = TRANSFORMERLESS; // Default to transformerless
    params->mppt = MPPT_NONE; // Default to no MPPT
    params->mppt_voltage = 220.0; // Initial MPPT voltage
    params->prev_power = 0.0;
    params->prev_voltage = 220.0;
    params->pll_enabled = FALSE; // Default to PLL disabled
    params->pll_phase = 0.0; // Initial PLL phase
    params->pll_frequency = 50.0; // Initial PLL frequency
    params->pll_voltage = 220.0; // Initial PLL voltage
    params->pll_locked = FALSE; // Initial PLL lock status
    params->pll_kp = 0.5; // Default proportional gain
    params->pll_ki = 10.0; // Default integral gain
    params->control = CONTROL_NONE; // Default to no control
    params->control_output = 1.0; // Default duty cycle
    params->control_ref_current = 10.0; // Default reference current (A, peak)
    params->control_ref_voltage = 220.0 * sqrt(2); // Default reference voltage (V, peak)
    params->islanding_enabled = FALSE; // Default to islanding detection disabled
    params->islanding_detected = FALSE; // Default to grid connected
    params->grid_condition = GRID_NORMAL; // Default to normal grid
    params->dc_source = DC_SOURCE_PV; // Default to PV
    params->dc_voltage = 220.0; // Default DC voltage
    params->dc_current = 10.0; // Default DC current
    params->pv_irradiance = 1000.0; // Default 1000 W/m²
    params->pv_temperature = 25.0; // Default 25 °C
    params->pv_ns = 6; // Default 6 panels in series
    params->pv_np = 2; // Default 2 strings in parallel
    params->battery_soc = 0.5; // Default 50% SoC
    params->battery_capacity = 100.0; // Default 100 Ah
    params->battery_charging = FALSE; // Default not charging
    params->fuel_cell_power = 500.0; // Default 500 W
    params->sim_time = 0.0; // Initial simulation time
    params->max_dt = 0.010; // Default max time step: 10ms
    params->prev_output[0] = 0.0; // Previous output initialization
    params->prev_output[1] = 0.0;
    params->prev_output[2] = 0.0;
}

void inverter_get_output(InverterParams *params, double time, double *output) {
    if (!params->running || params->islanding_detected) {
        output[0] = 0.0;
        output[1] = 0.0;
        output[2] = 0.0;
        return;
    }
    // Use MPPT-adjusted voltage if MPPT is active
    double original_voltage = params->voltage;
    if (params->mppt != MPPT_NONE) {
        params->voltage = params->mppt_voltage;
    }
    // Use PLL-adjusted phase, frequency, and voltage if PLL is enabled
    double original_phase = params->phase;
    double original_frequency = params->frequency;
    if (params->pll_enabled) {
        params->phase = params->pll_phase;
        params->frequency = params->pll_frequency;
        params->voltage = params->pll_voltage;
    }
    switch (params->type) {
        case SINGLE_PHASE:
            single_phase_output(params, time, output);
            break;
        case THREE_PHASE:
            three_phase_output(params, time, output);
            break;
        case NPC_INVERTER:
            npc_inverter_output(params, time, output);
            break;
        case FLYING_CAPACITOR:
            flying_capacitor_output(params, time, output);
            break;
        case CASCADED_H_BRIDGE:
            cascaded_h_bridge_output(params, time, output);
            break;
    }
    // Apply control type (duty cycle scaling)
    if (params->control != CONTROL_NONE) {
        for (int i = 0; i < 3; i++) {
            output[i] *= params->control_output;
        }
    }
    // Apply design-specific modifications
    if (params->design == TRANSFORMERLESS) {
        apply_transformerless(params, output);
    } else {
        apply_transformer_based(params, output);
    }
    // Restore original values
    params->voltage = original_voltage;
    params->phase = original_phase;
    params->frequency = original_frequency;
}