#include "inverter.h"
#include <math.h>

double calculate_time_step(AppData *app) {
    // Adaptive time step based on grid frequency deviation and output change
    double nominal_freq = 50.0; // Hz
    double freq_dev = fabs(app->params.pll_frequency - nominal_freq) / nominal_freq;
    
    // Calculate output change (RMS difference)
    double output[3];
    inverter_get_output(&app->params, app->params.sim_time, output);
    double output_change = 0.0;
    for (int i = 0; i < 3; i++) {
        output_change += (output[i] - app->params.prev_output[i]) * 
                         (output[i] - app->params.prev_output[i]);
        app->params.prev_output[i] = output[i];
    }
    output_change = sqrt(output_change);

    // Scale time step: small dt for large deviations, up to max_dt
    double dt = app->params.max_dt;
    if (freq_dev > 0.05 || output_change > 10.0) {
        dt *= 0.1; // Fast transients: 10% of max_dt
    } else if (freq_dev > 0.02 || output_change > 5.0) {
        dt *= 0.5; // Moderate transients
    }
    // Ensure dt is within bounds (0.1ms to max_dt)
    if (dt < 0.0001) dt = 0.0001;
    if (dt > app->params.max_dt) dt = app->params.max_dt;
    return dt;
}

void simulation_step(AppData *app, double dt) {
    // Update simulation time
    app->params.sim_time += dt;

    // Update MPPT if active
    if (app->params.mppt == MPPT_PERTURB_OBSERVE) {
        mppt_perturb_observe(&app->params);
    } else if (app->params.mppt == MPPT_INCREMENTAL_CONDUCTANCE) {
        mppt_incremental_conductance(&app->params);
    }

    // Update PLL if enabled
    if (app->params.pll_enabled) {
        pll_update(&app->params, app->params.sim_time);
    }

    // Update control if active
    if (app->params.control != CONTROL_NONE) {
        control_update(&app->params, app->params.sim_time);
    }

    // Update islanding detection if enabled
    if (app->params.islanding_enabled) {
        islanding_detection_update(&app->params, app->params.sim_time);
    }

    // Update DC source
    dc_source_update(app);
}

gboolean simulation_update(gpointer user_data) {
    AppData *app = (AppData *)user_data;
    if (!app->params.running) {
        return G_SOURCE_CONTINUE;
    }

    // Calculate adaptive time step
    double dt = calculate_time_step(app);

    // Perform simulation step
    simulation_step(app, dt);

    // Update GUI elements
    if (app->params.pll_enabled) {
        char lock_text[32];
        snprintf(lock_text, sizeof(lock_text), "PLL Lock: %s", 
                 app->params.pll_locked ? "Locked" : "Not Locked");
        gtk_label_set_text(GTK_LABEL(app->pll_lock_label), lock_text);
    }

    if (app->params.islanding_enabled) {
        gtk_label_set_text(GTK_LABEL(app->islanding_status_label), 
                           app->params.islanding_detected ? "Islanding Detected" : "Grid Connected");
        if (app->params.islanding_detected) {
            app->params.running = FALSE;
            if (app->timeout_id != 0) {
                g_source_remove(app->timeout_id);
                app->timeout_id = 0;
            }
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->start_button), FALSE);
            gtk_button_set_label(GTK_BUTTON(app->pause_button), "Pause");
        }
    }

    const char *grid_status;
    switch (app->params.grid_condition) {
        case GRID_NORMAL: grid_status = "Normal Grid"; break;
        case GRID_WEAK: grid_status = "Weak Grid"; break;
        case GRID_FAULT_SAG: grid_status = "Voltage Sag"; break;
        case GRID_FAULT_SWELL: grid_status = "Voltage Swell"; break;
        case GRID_FAULT_HARMONICS: grid_status = "Harmonics"; break;
        case GRID_FAULT_FREQ_SHIFT: grid_status = "Frequency Shift"; break;
        default: grid_status = "Unknown";
    }
    gtk_label_set_text(GTK_LABEL(app->grid_status_label), grid_status);

    // Update DC source labels
    char text[64];
    snprintf(text, sizeof(text), "Vdc: %.2f V", app->params.dc_voltage);
    gtk_label_set_text(GTK_LABEL(app->dc_voltage_label), text);
    snprintf(text, sizeof(text), "Idc: %.2f A", app->params.dc_current);
    gtk_label_set_text(GTK_LABEL(app->dc_current_label), text);
    snprintf(text, sizeof(text), "Battery SoC: %.1f%%", app->params.battery_soc * 100);
    gtk_label_set_text(GTK_LABEL(app->dc_soc_label), text);
    snprintf(text, sizeof(text), "Power: %.2f W", app->params.dc_voltage * app->params.dc_current);
    gtk_label_set_text(GTK_LABEL(app->dc_power_label), text);

    gtk_widget_queue_draw(app->drawing_area);
    return G_SOURCE_CONTINUE;
}