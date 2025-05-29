#include "inverter.h"

static gboolean update_waveform(gpointer user_data) {
    AppData *app = (AppData *)user_data;
    if (app->params.running) {
        // Update MPPT if active
        if (app->params.mppt == MPPT_PERTURB_OBSERVE) {
            mppt_perturb_observe(&app->params);
        } else if (app->params.mppt == MPPT_INCREMENTAL_CONDUCTANCE) {
            mppt_incremental_conductance(&app->params);
        }
        // Update PLL if enabled
        if (app->params.pll_enabled) {
            pll_update(&app->params, g_get_monotonic_time() / 1e6);
            // Update lock status label
            char lock_text[32];
            snprintf(lock_text, sizeof(lock_text), "PLL Lock: %s", app->params.pll_locked ? "Locked" : "Not Locked");
            gtk_label_set_text(GTK_LABEL(app->pll_lock_label), lock_text);
        }
        // Update control if active
        if (app->params.control != CONTROL_NONE) {
            control_update(&app->params, g_get_monotonic_time() / 1e6);
        }
        // Update islanding detection if enabled
        if (app->params.islanding_enabled) {
            islanding_detection_update(&app->params, g_get_monotonic_time() / 1e6);
            // Update islanding status label
            gtk_label_set_text(GTK_LABEL(app->islanding_status_label), 
                app->params.islanding_detected ? "Islanding Detected" : "Grid Connected");
            if (app->params.islanding_detected) {
                // Stop simulation
                app->params.running = FALSE;
                if (app->timeout_id != 0) {
                    g_source_remove(app->timeout_id);
                    app->timeout_id = 0;
                }
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->start_button), FALSE);
                gtk_button_set_label(GTK_BUTTON(app->pause_button), "Pause");
            }
        }
        // Update grid status label
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
        // Update DC source
        dc_source_update(app);
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
    }
    return G_SOURCE_CONTINUE;
}

static void on_start_button_toggled(GtkToggleButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.running = gtk_toggle_button_get_active(button);
    if (app->params.running && app->timeout_id == 0) {
        app->timeout_id = g_timeout_add(50, update_waveform, app); // 20 FPS
        gtk_button_set_label(GTK_BUTTON(app->pause_button), "Pause");
    } else if (!app->params.running && app->timeout_id != 0) {
        g_source_remove(app->timeout_id);
        app->timeout_id = 0;
        gtk_button_set_label(GTK_BUTTON(app->pause_button), "Resume");
    }
}

static void on_pause_button_clicked(GtkButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.running = !app->params.running;
    if (app->params.running && app->timeout_id == 0) {
        app->timeout_id = g_timeout_add(50, update_waveform, app); // Resume
        gtk_button_set_label(GTK_BUTTON(button), "Pause");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->start_button), TRUE);
    } else if (!app->params.running && app->timeout_id != 0) {
        g_source_remove(app->timeout_id);
        app->timeout_id = 0;
        gtk_button_set_label(GTK_BUTTON(button), "Resume");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->start_button), FALSE);
    }
}

static void on_reset_button_clicked(GtkButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    // Stop simulation
    if (app->timeout_id != 0) {
        g_source_remove(app->timeout_id);
        app->timeout_id = 0;
    }
    app->params.running = FALSE;
    // Reset parameters
    inverter_init(&app->params);
    // Update GUI
    gtk_range_set_value(GTK_RANGE(app->voltage_scale), app->params.voltage);
    gtk_range_set_value(GTK_RANGE(app->frequency_scale), app->params.frequency);
    gtk_range_set_value(GTK_RANGE(app->phase_scale), app->params.phase);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->type_dropdown), app->params.type);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->design_dropdown), app->params.design);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->mppt_dropdown), app->params.mppt);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->control_dropdown), app->params.control);
    gtk_switch_set_active(GTK_SWITCH(app->pll_switch), app->params.pll_enabled);
    gtk_switch_set_active(GTK_SWITCH(app->islanding_switch), app->params.islanding_enabled);
    gtk_range_set_value(GTK_RANGE(app->pll_kp_scale), app->params.pll_kp);
    gtk_range_set_value(GTK_RANGE(app->pll_ki_scale), app->params.pll_ki);
    gtk_label_set_text(GTK_LABEL(app->pll_lock_label), "PLL Lock: Not Locked");
    gtk_label_set_text(GTK_LABEL(app->islanding_status_label), "Grid Connected");
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->grid_dropdown), app->params.grid_condition);
    gtk_label_set_text(GTK_LABEL(app->grid_status_label), "Normal Grid");
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->dc_source_dropdown), app->params.dc_source);
    if (app->dc_source_window) {
        gtk_range_set_value(GTK_RANGE(app->dc_irradiance_scale), app->params.pv_irradiance);
        gtk_range_set_value(GTK_RANGE(app->dc_temperature_scale), app->params.pv_temperature);
        gtk_range_set_value(GTK_RANGE(app->dc_ns_scale), app->params.pv_ns);
        gtk_range_set_value(GTK_RANGE(app->dc_np_scale), app->params.pv_np);
        gtk_range_set_value(GTK_RANGE(app->dc_soc_scale), app->params.battery_soc * 100);
        gtk_drop_down_set_selected(GTK_DROP_DOWN(app->dc_battery_type_dropdown), 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->dc_charge_button), app->params.battery_charging);
        gtk_switch_set_active(GTK_SWITCH(app->dc_fuel_cell_switch), FALSE);
        gtk_range_set_value(GTK_RANGE(app->dc_fuel_cell_power_scale), app->params.fuel_cell_power);
    }
    gtk_label_set_text(GTK_LABEL(app->dc_voltage_label), "Vdc: 0.00 V");
    gtk_label_set_text(GTK_LABEL(app->dc_current_label), "Idc: 0.00 A");
    gtk_label_set_text(GTK_LABEL(app->dc_soc_label), "Battery SoC: 0.0%");
    gtk_label_set_text(GTK_LABEL(app->dc_power_label), "Power: 0.00 W");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->start_button), FALSE);
    gtk_button_set_label(GTK_BUTTON(app->pause_button), "Pause");
    gtk_widget_queue_draw(app->drawing_area);
}

static void on_scale_value_changed(GtkRange *range, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.voltage = gtk_range_get_value(GTK_RANGE(app->voltage_scale));
    app->params.frequency = gtk_range_get_value(GTK_RANGE(app->frequency_scale));
    app->params.phase = gtk_range_get_value(GTK_RANGE(app->phase_scale));
    if (app->params.running) {
        gtk_widget_queue_draw(app->drawing_area);
    }
}

static void on_pll_param_changed(GtkRange *range, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.pll_kp = gtk_range_get_value(GTK_RANGE(app->pll_kp_scale));
    app->params.pll_ki = gtk_range_get_value(GTK_RANGE(app->pll_ki_scale));
    if (app->params.running) {
        gtk_widget_queue_draw(app->drawing_area);
    }
}

static void on_type_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.type = gtk_drop_down_get_selected(dropdown);
    if (app->params.running) {
        gtk_widget_queue_draw(app->drawing_area);
    }
}

static void on_design_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.design = gtk_drop_down_get_selected(dropdown);
    if (app->params.running) {
        gtk_widget_queue_draw(app->drawing_area);
    }
}

static void on_mppt_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.mppt = gtk_drop_down_get_selected(dropdown);
    if (app->params.running) {
        gtk_widget_queue_draw(app->drawing_area);
    }
}

static void on_control_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.control = gtk_drop_down_get_selected(dropdown);
    if (app->params.running) {
        gtk_widget_queue_draw(app->drawing_area);
    }
}

static void on_pll_toggled(GtkSwitch *switch_widget, gboolean state, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.pll_enabled = state;
    if (app->params.running) {
        gtk_widget_queue_draw(app->drawing_area);
    }
}

static void on_islanding_toggled(GtkSwitch *switch_widget, gboolean state, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.islanding_enabled = state;
    if (!state) {
        app->params.islanding_detected = FALSE;
        gtk_label_set_text(GTK_LABEL(app->islanding_status_label), "Grid Connected");
    }
    if (app->params.running) {
        gtk_widget_queue_draw(app->drawing_area);
    }
}

static void on_grid_condition_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.grid_condition = gtk_drop_down_get_selected(dropdown);
    if (app->params.running) {
        gtk_widget_queue_draw(app->drawing_area);
    }
}

static void on_dc_source_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.dc_source = gtk_drop_down_get_selected(dropdown);
    if (app->params.running) {
        dc_source_update(app);
        gtk_widget_queue_draw(app->drawing_area);
    }
}

static void on_dc_source_button_clicked(GtkButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    if (!app->dc_source_window) {
        dc_source_window_create(app);
    }
    gtk_window_present(GTK_WINDOW(app->dc_source_window));
}

static void activate(GtkApplication *gtk_app, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->window), "Grid-Tie Inverter Simulator");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 800, 600);

    inverter_init(&app->params);
    build_interface(app);

    // Connect signals
    g_signal_connect(app->start_button, "toggled", G_CALLBACK(on_start_button_toggled), app);
    g_signal_connect(app->pause_button, "clicked", G_CALLBACK(on_pause_button_clicked), app);
    g_signal_connect(app->reset_button, "clicked", G_CALLBACK(on_reset_button_clicked), app);
    g_signal_connect(app->voltage_scale, "value-changed", G_CALLBACK(on_scale_value_changed), app);
    g_signal_connect(app->frequency_scale, "value-changed", G_CALLBACK(on_scale_value_changed), app);
    g_signal_connect(app->phase_scale, "value-changed", G_CALLBACK(on_scale_value_changed), app);
    g_signal_connect(app->pll_kp_scale, "value-changed", G_CALLBACK(on_pll_param_changed), app);
    g_signal_connect(app->pll_ki_scale, "value-changed", G_CALLBACK(on_pll_param_changed), app);
    g_signal_connect(app->type_dropdown, "notify::selected", G_CALLBACK(on_type_changed), app);
    g_signal_connect(app->design_dropdown, "notify::selected", G_CALLBACK(on_design_changed), app);
    g_signal_connect(app->mppt_dropdown, "notify::selected", G_CALLBACK(on_mppt_changed), app);
    g_signal_connect(app->control_dropdown, "notify::selected", G_CALLBACK(on_control_changed), app);
    g_signal_connect(app->pll_switch, "state-set", G_CALLBACK(on_pll_toggled), app);
    g_signal_connect(app->islanding_switch, "state-set", G_CALLBACK(on_islanding_toggled), app);
    g_signal_connect(app->grid_dropdown, "notify::selected", G_CALLBACK(on_grid_condition_changed), app);
    g_signal_connect(app->dc_source_dropdown, "notify::selected", G_CALLBACK(on_dc_source_changed), app);
    g_signal_connect(app->dc_source_button, "clicked", G_CALLBACK(on_dc_source_button_clicked), app);

    gtk_window_present(GTK_WINDOW(app->window));
}

int main(int argc, char *argv[]) {
    AppData app = {0};
    GtkApplication *gtk_app = gtk_application_new("com.example.inverter", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(gtk_app, "activate", G_CALLBACK(activate), &app);
    int status = g_application_run(G_APPLICATION(gtk_app), argc, argv);
    g_object_unref(gtk_app);
    return status;
}