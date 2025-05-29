#include "inverter.h"

void build_interface(AppData *app) {
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_window_set_child(GTK_WINDOW(app->window), main_box);

    // Control panel with scrollable window
    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scrolled_window, 250, -1); // Wider to accommodate content
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_append(GTK_BOX(main_box), scrolled_window);

    GtkWidget *control_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(control_box, 10);
    gtk_widget_set_margin_end(control_box, 10);
    gtk_widget_set_margin_top(control_box, 10);
    gtk_widget_set_margin_bottom(control_box, 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), control_box);

    // Inverter type dropdown
    GtkWidget *type_label = gtk_label_new("Inverter Type:");
    gtk_box_append(GTK_BOX(control_box), type_label);
    const char *types[] = { "Single-Phase", "Three-Phase", "NPC", "Flying Capacitor", "Cascaded H-Bridge", NULL };
    app->type_dropdown = gtk_drop_down_new_from_strings(types);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->type_dropdown), app->params.type);
    gtk_box_append(GTK_BOX(control_box), app->type_dropdown);

    // Design type dropdown
    GtkWidget *design_label = gtk_label_new("Design Type:");
    gtk_box_append(GTK_BOX(control_box), design_label);
    const char *designs[] = { "Transformerless", "Transformer-based", NULL };
    app->design_dropdown = gtk_drop_down_new_from_strings(designs);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->design_dropdown), app->params.design);
    gtk_box_append(GTK_BOX(control_box), app->design_dropdown);

    // MPPT type dropdown
    GtkWidget *mppt_label = gtk_label_new("MPPT Type:");
    gtk_box_append(GTK_BOX(control_box), mppt_label);
    const char *mppts[] = { "None", "Perturb & Observe", "Incremental Conductance", NULL };
    app->mppt_dropdown = gtk_drop_down_new_from_strings(mppts);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->mppt_dropdown), app->params.mppt);
    gtk_box_append(GTK_BOX(control_box), app->mppt_dropdown);

    // Control type dropdown
    GtkWidget *control_label = gtk_label_new("Control Type:");
    gtk_box_append(GTK_BOX(control_box), control_label);
    const char *controls[] = { "None", "PI", "PR", "SMC", "MPC", NULL };
    app->control_dropdown = gtk_drop_down_new_from_strings(controls);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->control_dropdown), app->params.control);
    gtk_box_append(GTK_BOX(control_box), app->control_dropdown);

    // PLL switch
    GtkWidget *pll_label = gtk_label_new("PLL (Grid Sync):");
    gtk_box_append(GTK_BOX(control_box), pll_label);
    app->pll_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->pll_switch), app->params.pll_enabled);
    gtk_box_append(GTK_BOX(control_box), app->pll_switch);

    // PLL lock status
    app->pll_lock_label = gtk_label_new("PLL Lock: Not Locked");
    gtk_box_append(GTK_BOX(control_box), app->pll_lock_label);

    // Islanding detection switch
    GtkWidget *islanding_label = gtk_label_new("Islanding Detection:");
    gtk_box_append(GTK_BOX(control_box), islanding_label);
    app->islanding_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->islanding_switch), app->params.islanding_enabled);
    gtk_box_append(GTK_BOX(control_box), app->islanding_switch);

    // Islanding status label
    app->islanding_status_label = gtk_label_new("Grid Connected");
    gtk_box_append(GTK_BOX(control_box), app->islanding_status_label);

    // Grid condition dropdown
    GtkWidget *grid_label = gtk_label_new("Grid Condition:");
    gtk_box_append(GTK_BOX(control_box), grid_label);
    const char *grids[] = { "Normal", "Weak", "Fault (Sag)", "Fault (Swell)", "Fault (Harmonics)", "Fault (Freq Shift)", NULL };
    app->grid_dropdown = gtk_drop_down_new_from_strings(grids);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->grid_dropdown), app->params.grid_condition);
    gtk_box_append(GTK_BOX(control_box), app->grid_dropdown);

    // Grid status label
    app->grid_status_label = gtk_label_new("Normal Grid");
    gtk_box_append(GTK_BOX(control_box), app->grid_status_label);

    // DC source dropdown
    GtkWidget *dc_source_label = gtk_label_new("DC Source:");
    gtk_box_append(GTK_BOX(control_box), dc_source_label);
    const char *dc_sources[] = { "PV", "Battery", "Fuel Cell", "Hybrid", NULL };
    app->dc_source_dropdown = gtk_drop_down_new_from_strings(dc_sources);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->dc_source_dropdown), app->params.dc_source);
    gtk_box_append(GTK_BOX(control_box), app->dc_source_dropdown);

    // DC source config button
    app->dc_source_button = gtk_button_new_with_label("Configure DC Source");
    gtk_box_append(GTK_BOX(control_box), app->dc_source_button);

    // DC source status labels
    app->dc_voltage_label = gtk_label_new("Vdc: 0.00 V");
    gtk_box_append(GTK_BOX(control_box), app->dc_voltage_label);
    app->dc_current_label = gtk_label_new("Idc: 0.00 A");
    gtk_box_append(GTK_BOX(control_box), app->dc_current_label);
    app->dc_soc_label = gtk_label_new("Battery SoC: 0.0%");
    gtk_box_append(GTK_BOX(control_box), app->dc_soc_label);
    app->dc_power_label = gtk_label_new("Power: 0.00 W");
    gtk_box_append(GTK_BOX(control_box), app->dc_power_label);

    // PLL kp slider
    GtkWidget *kp_label = gtk_label_new("PLL Kp:");
    gtk_box_append(GTK_BOX(control_box), kp_label);
    app->pll_kp_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 2.0, 0.01);
    gtk_range_set_value(GTK_RANGE(app->pll_kp_scale), app->params.pll_kp);
    gtk_box_append(GTK_BOX(control_box), app->pll_kp_scale);

    // PLL ki slider
    GtkWidget *ki_label = gtk_label_new("PLL Ki:");
    gtk_box_append(GTK_BOX(control_box), ki_label);
    app->pll_ki_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1.0, 50.0, 0.1);
    gtk_range_set_value(GTK_RANGE(app->pll_ki_scale), app->params.pll_ki);
    gtk_box_append(GTK_BOX(control_box), app->pll_ki_scale);

    // Max time-step slider
    GtkWidget *timestep_label = gtk_label_new("Max Time Step (ms):");
    gtk_box_append(GTK_BOX(control_box), timestep_label);
    app->timestep_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 10.0, 0.1);
    gtk_range_set_value(GTK_RANGE(app->timestep_scale), app->params.max_dt * 1000.0); // Convert s to ms
    gtk_box_append(GTK_BOX(control_box), app->timestep_scale);

    // Voltage slider
    GtkWidget *voltage_label = gtk_label_new("Voltage (V):");
    gtk_box_append(GTK_BOX(control_box), voltage_label);
    app->voltage_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 100, 300, 1);
    gtk_range_set_value(GTK_RANGE(app->voltage_scale), app->params.voltage);
    gtk_box_append(GTK_BOX(control_box), app->voltage_scale);

    // Frequency slider
    GtkWidget *frequency_label = gtk_label_new("Frequency (Hz):");
    gtk_box_append(GTK_BOX(control_box), frequency_label);
    app->frequency_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 40, 60, 0.1);
    gtk_range_set_value(GTK_RANGE(app->frequency_scale), app->params.frequency);
    gtk_box_append(GTK_BOX(control_box), app->frequency_scale);

    // Phase slider
    GtkWidget *phase_label = gtk_label_new("Phase (rad):");
    gtk_box_append(GTK_BOX(control_box), phase_label);
    app->phase_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 2 * M_PI, 0.01);
    g_object_set(app->phase_scale, "value", app->params.phase, NULL);
    gtk_box_append(GTK_BOX(control_box), app->phase_scale);

    // Button box for Start, Pause, Reset
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(control_box), button_box);

    // Start button
    app->start_button = gtk_toggle_button_new_with_label("Start");
    gtk_box_append(GTK_BOX(button_box), app->start_button);

    // Pause button
    app->pause_button = gtk_button_new_with_label("Pause");
    gtk_box_append(GTK_BOX(button_box), app->pause_button);

    // Reset button
    app->reset_button = gtk_button_new_with_label("Reset");
    gtk_box_append(GTK_BOX(button_box), app->reset_button);

    // Waveform drawing area
    app->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(app->drawing_area, TRUE);
    gtk_widget_set_vexpand(app->drawing_area, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(app->drawing_area), waveform_draw, app, NULL);
    gtk_box_append(GTK_BOX(main_box), app->drawing_area);
}