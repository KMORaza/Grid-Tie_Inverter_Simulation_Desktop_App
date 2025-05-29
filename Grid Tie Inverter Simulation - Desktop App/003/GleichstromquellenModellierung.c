#include "inverter.h"
#include <math.h>
#include <stdlib.h>

static double pv_current(AppData *app, double V) {
    // PV parameters (per panel, e.g., 250W module)
    const double Isc = 8.21; // Short-circuit current (A) at STC
    const double Voc = 37.6; // Open-circuit voltage (V) at STC
    const double Ki = 0.00065; // Current temp coefficient (A/°C)
    const double Kv = -0.123; // Voltage temp coefficient (V/°C)
    const double Gref = 1000.0; // Reference irradiance (W/m²)
    const double Tref = 25.0; // Reference temperature (°C)
    const double q = 1.602e-19; // Electron charge (C)
    const double k = 1.381e-23; // Boltzmann constant (J/K)
    const double n = 1.3; // Diode ideality factor
    const double Rs = 0.221; // Series resistance (Ω)
    const double Rsh = 415.405; // Shunt resistance (Ω)

    double G = app->params.pv_irradiance;
    double T = app->params.pv_temperature + 273.15; // Convert to K
    int Ns = app->params.pv_ns;
    int Np = app->params.pv_np;

    // Adjust for temperature and irradiance
    double Iph = (Isc + Ki * (T - 273.15 - Tref)) * (G / Gref) * Np;
    double Vt = Ns * n * k * T / q; // Thermal voltage
    double Io = Isc / (exp((Voc + Kv * (T - 273.15 - Tref)) / Vt) - 1); // Saturation current

    // Solve I = Iph - Io * [exp((V + I*Rs)/(n*Vt)) - 1] - (V + I*Rs)/Rsh
    // Use approximation for simplicity
    double I = Iph;
    for (int i = 0; i < 5; i++) { // Newton-Raphson iteration
        double V_Rs = V / Ns + I * Rs;
        double exp_term = exp(V_Rs / Vt);
        I = Iph - Io * (exp_term - 1) - V_Rs / Rsh;
    }
    return I * Np;
}

static double battery_voltage(AppData *app, double I) {
    // Battery parameters (Li-ion, 48V nominal)
    double Vnom = 48.0; // Nominal voltage
    double capacity = app->params.battery_capacity; // Ah
    double SoC = app->params.battery_soc;
    double eta = app->params.battery_charging ? 0.95 : 0.98; // Charge/discharge efficiency
    int battery_type = gtk_drop_down_get_selected(GTK_DROP_DOWN(app->dc_battery_type_dropdown));
    if (battery_type == 1) { // Lead-acid
        Vnom = 12.0;
        eta *= 0.9;
    }

    // Kinetic battery model approximation
    double V0 = Vnom * (1.0 + 0.1 * (SoC - 0.5)); // Open-circuit voltage
    double R_int = 0.05 * (1.0 / SoC); // Internal resistance
    double V = V0 - I * R_int;

    // Update SoC
    double dt = 0.05; // 50ms step
    double dAh = (app->params.battery_charging ? I * eta : -I / eta) * (dt / 3600.0);
    app->params.battery_soc += dAh / capacity;
    if (app->params.battery_soc > 0.9) app->params.battery_soc = 0.9; // Prevent overcharge
    if (app->params.battery_soc < 0.2) app->params.battery_soc = 0.2; // Prevent deep discharge

    return V;
}

static double fuel_cell_voltage(AppData *app, double I) {
    // PEM fuel cell parameters
    const double V0 = 48.0; // Nominal stack voltage
    const double A = 0.06; // Tafel slope
    const double R = 0.01; // Ohmic resistance
    const double m = 0.05; // Mass transport coefficient
    const double n = 0.0001; // Mass transport constant

    double P_demand = app->params.fuel_cell_power;
    double V = V0 - A * log(I + 1.0) - I * R - m * exp(n * I);
    if (V * I > P_demand) {
        I = P_demand / V; // Limit to demanded power
        V = V0 - A * log(I + 1.0) - I * R - m * exp(n * I);
    }
    return V;
}

void dc_source_update(AppData *app) {
    double V = app->params.mppt_voltage; // Use MPPT voltage if active
    double I = 0.0;

    switch (app->params.dc_source) {
        case DC_SOURCE_PV:
            I = pv_current(app, V);
            app->params.dc_voltage = V;
            app->params.dc_current = I;
            break;
        case DC_SOURCE_BATTERY:
            app->params.dc_voltage = battery_voltage(app, I);
            app->params.dc_current = app->params.control_ref_current; // Assume inverter demand
            break;
        case DC_SOURCE_FUEL_CELL:
            I = app->params.fuel_cell_power / V;
            app->params.dc_voltage = fuel_cell_voltage(app, I);
            app->params.dc_current = I;
            break;
        case DC_SOURCE_HYBRID:
            // Hybrid: Fuel cell provides base, battery handles transient
            I = app->params.fuel_cell_power / V;
            app->params.dc_voltage = fuel_cell_voltage(app, I);
            app->params.dc_current = I;
            double I_extra = app->params.control_ref_current - I;
            if (I_extra > 0) {
                app->params.dc_voltage = battery_voltage(app, I_extra);
                app->params.dc_current += I_extra;
            }
            break;
    }
}

static void on_dc_param_changed(GtkRange *range, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.pv_irradiance = gtk_range_get_value(GTK_RANGE(app->dc_irradiance_scale));
    app->params.pv_temperature = gtk_range_get_value(GTK_RANGE(app->dc_temperature_scale));
    app->params.pv_ns = (int)gtk_range_get_value(GTK_RANGE(app->dc_ns_scale));
    app->params.pv_np = (int)gtk_range_get_value(GTK_RANGE(app->dc_np_scale));
    app->params.battery_soc = gtk_range_get_value(GTK_RANGE(app->dc_soc_scale)) / 100.0;
    app->params.fuel_cell_power = gtk_range_get_value(GTK_RANGE(app->dc_fuel_cell_power_scale));
    if (app->params.running) {
        dc_source_update(app);
    }
}

static void on_dc_charge_toggled(GtkToggleButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->params.battery_charging = gtk_toggle_button_get_active(button);
    if (app->params.running) {
        dc_source_update(app);
    }
}

static void on_dc_fuel_cell_toggled(GtkSwitch *switch_widget, gboolean state, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    if (state && app->params.dc_source != DC_SOURCE_FUEL_CELL && app->params.dc_source != DC_SOURCE_HYBRID) {
        gtk_switch_set_active(switch_widget, FALSE); // Disable if not applicable
    }
    if (app->params.running) {
        dc_source_update(app);
    }
}

void dc_source_window_create(AppData *app) {
    app->dc_source_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(app->dc_source_window), "DC Source Configuration");
    gtk_window_set_default_size(GTK_WINDOW(app->dc_source_window), 400, 600);
    gtk_window_set_transient_for(GTK_WINDOW(app->dc_source_window), GTK_WINDOW(app->window));

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_window_set_child(GTK_WINDOW(app->dc_source_window), box);

    // PV configuration
    GtkWidget *pv_label = gtk_label_new("PV Parameters:");
    gtk_box_append(GTK_BOX(box), pv_label);

    // Irradiance
    GtkWidget *irradiance_label = gtk_label_new("Irradiance (W/m²):");
    gtk_box_append(GTK_BOX(box), irradiance_label);
    app->dc_irradiance_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 200, 1000, 10);
    gtk_range_set_value(GTK_RANGE(app->dc_irradiance_scale), app->params.pv_irradiance);
    gtk_box_append(GTK_BOX(box), app->dc_irradiance_scale);

    // Temperature
    GtkWidget *temp_label = gtk_label_new("Temperature (°C):");
    gtk_box_append(GTK_BOX(box), temp_label);
    app->dc_temperature_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 50, 1);
    gtk_range_set_value(GTK_RANGE(app->dc_temperature_scale), app->params.pv_temperature);
    gtk_box_append(GTK_BOX(box), app->dc_temperature_scale);

    // Ns
    GtkWidget *ns_label = gtk_label_new("Panels in Series:");
    gtk_box_append(GTK_BOX(box), ns_label);
    app->dc_ns_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 10, 1);
    gtk_range_set_value(GTK_RANGE(app->dc_ns_scale), app->params.pv_ns);
    gtk_box_append(GTK_BOX(box), app->dc_ns_scale);

    // Np
    GtkWidget *np_label = gtk_label_new("Parallel Strings:");
    gtk_box_append(GTK_BOX(box), np_label);
    app->dc_np_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 5, 1);
    gtk_range_set_value(GTK_RANGE(app->dc_np_scale), app->params.pv_np);
    gtk_box_append(GTK_BOX(box), app->dc_np_scale);

    // Battery configuration
    GtkWidget *battery_label = gtk_label_new("Battery Parameters:");
    gtk_box_append(GTK_BOX(box), battery_label);

    // Battery type
    GtkWidget *battery_type_label = gtk_label_new("Battery Type:");
    gtk_box_append(GTK_BOX(box), battery_type_label);
    const char *battery_types[] = { "Li-ion", "Lead-acid", NULL };
    app->dc_battery_type_dropdown = gtk_drop_down_new_from_strings(battery_types);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->dc_battery_type_dropdown), 0);
    gtk_box_append(GTK_BOX(box), app->dc_battery_type_dropdown);

    // SoC
    GtkWidget *soc_label = gtk_label_new("State of Charge (%):");
    gtk_box_append(GTK_BOX(box), soc_label);
    app->dc_soc_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 20, 90, 1);
    gtk_range_set_value(GTK_RANGE(app->dc_soc_scale), app->params.battery_soc * 100);
    gtk_box_append(GTK_BOX(box), app->dc_soc_scale);

    // Charge/Discharge
    app->dc_charge_button = gtk_toggle_button_new_with_label("Charge");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->dc_charge_button), app->params.battery_charging);
    gtk_box_append(GTK_BOX(box), app->dc_charge_button);

    // Fuel cell configuration
    GtkWidget *fuel_cell_label = gtk_label_new("Fuel Cell Parameters:");
    gtk_box_append(GTK_BOX(box), fuel_cell_label);

    // Enable fuel cell
    app->dc_fuel_cell_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->dc_fuel_cell_switch), FALSE);
    gtk_box_append(GTK_BOX(box), app->dc_fuel_cell_switch);

    // Power demand
    GtkWidget *fc_power_label = gtk_label_new("Power Demand (W):");
    gtk_box_append(GTK_BOX(box), fc_power_label);
    app->dc_fuel_cell_power_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1000, 10);
    gtk_range_set_value(GTK_RANGE(app->dc_fuel_cell_power_scale), app->params.fuel_cell_power);
    gtk_box_append(GTK_BOX(box), app->dc_fuel_cell_power_scale);

    // Connect signals
    g_signal_connect(app->dc_irradiance_scale, "value-changed", G_CALLBACK(on_dc_param_changed), app);
    g_signal_connect(app->dc_temperature_scale, "value-changed", G_CALLBACK(on_dc_param_changed), app);
    g_signal_connect(app->dc_ns_scale, "value-changed", G_CALLBACK(on_dc_param_changed), app);
    g_signal_connect(app->dc_np_scale, "value-changed", G_CALLBACK(on_dc_param_changed), app);
    g_signal_connect(app->dc_soc_scale, "value-changed", G_CALLBACK(on_dc_param_changed), app);
    g_signal_connect(app->dc_fuel_cell_power_scale, "value-changed", G_CALLBACK(on_dc_param_changed), app);
    g_signal_connect(app->dc_charge_button, "toggled", G_CALLBACK(on_dc_charge_toggled), app);
    g_signal_connect(app->dc_fuel_cell_switch, "state-set", G_CALLBACK(on_dc_fuel_cell_toggled), app);
}