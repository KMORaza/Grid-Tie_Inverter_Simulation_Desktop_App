#include "inverter.h"
#include <math.h>
#include <complex.h>
#include <stdlib.h>
#include <stdio.h>

static void analysis_draw_callback(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    if (!app) {
        fprintf(stderr, "[Error] Invalid app in analysis_draw_callback\n");
        return;
    }

    // Black background
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    // White grid
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 0.5);
    for (int i = 0; i <= 10; i++) {
        double x = i * width / 10.0;
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, height);
        cairo_stroke(cr);
    }
    for (int i = 0; i <= 8; i++) {
        double y = i * height / 8.0;
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, width, y);
        cairo_stroke(cr);
    }

    double freq[1000], gain[1000], phase[1000];
    int n_points = 0;
    frequency_domain_analysis(app, freq, gain, phase, &n_points);
    if (n_points <= 0 || app->params.analysis_freq_max <= app->params.analysis_freq_min) {
        fprintf(stderr, "[Error] Invalid Bode data (n_points=%d, f_min=%f, f_max=%f)\n",
                n_points, app->params.analysis_freq_min, app->params.analysis_freq_max);
        return;
    }

    // Plot gain (top half)
    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    cairo_set_line_width(cr, 2.0);
    double max_gain = -1000, min_gain = 1000;
    for (int i = 0; i < n_points; i++) {
        if (gain[i] > max_gain) max_gain = gain[i];
        if (gain[i] < min_gain) min_gain = gain[i];
    }
    double gain_range = (max_gain - min_gain) > 0 ? (max_gain - min_gain) : 1.0;
    for (int i = 0; i < n_points; i++) {
        double x = log10(freq[i] / app->params.analysis_freq_min) / 
                   log10(app->params.analysis_freq_max / app->params.analysis_freq_min) * width;
        double y = (height / 2.0) * (1.0 - (gain[i] - min_gain) / gain_range);
        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_stroke(cr);

    // Plot phase (bottom half)
    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    for (int i = 0; i < n_points; i++) {
        double x = log10(freq[i] / app->params.analysis_freq_min) / 
                   log10(app->params.analysis_freq_max / app->params.analysis_freq_min) * width;
        double y = (height / 2.0) + (height / 2.0) * (1.0 - (phase[i] + 180.0) / 360.0);
        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_stroke(cr);
}

static void on_analysis_updated_debounced(gpointer user_data) {
    AppData *app = (AppData *)user_data;
    if (!app) {
        fprintf(stderr, "[Error] Invalid app in on_analysis_updated_debounced\n");
        return;
    }

    app->params.analysis_type = ANALYSIS_BODE; // Only Bode supported
    app->params.analysis_freq_min = gtk_range_get_value(GTK_RANGE(app->analysis_freq_min_scale));
    app->params.analysis_freq_max = gtk_range_get_value(GTK_RANGE(app->analysis_freq_max_scale));
    app->params.analysis_op_voltage = gtk_range_get_value(GTK_RANGE(app->analysis_op_voltage_scale));
    app->params.analysis_op_load = gtk_range_get_value(GTK_RANGE(app->analysis_op_load_scale));

    // Validate inputs
    if (app->params.analysis_freq_min <= 0.0) app->params.analysis_freq_min = 0.01;
    if (app->params.analysis_freq_max <= app->params.analysis_freq_min)
        app->params.analysis_freq_max = app->params.analysis_freq_min * 1000.0;
    if (app->params.analysis_op_load <= 0.0) app->params.analysis_op_load = 10.0;
    if (app->params.analysis_op_voltage <= 0.0) app->params.analysis_op_voltage = 220.0;

    fprintf(stderr, "[Info] Params updated: type=%d, f_min=%f, f_max=%f, V=%f, R=%f\n",
            app->params.analysis_type, app->params.analysis_freq_min, app->params.analysis_freq_max,
            app->params.analysis_op_voltage, app->params.analysis_op_load);

    if (app->analysis_drawing_area) {
        gtk_widget_queue_draw(app->analysis_drawing_area);
    }
}

static void on_analysis_param_changed(GtkWidget *widget, gpointer user_data) {
    g_timeout_add_once(50, on_analysis_updated_debounced, user_data);
}

static void on_analysis_run_clicked(GtkButton *button, gpointer user_data) {
    g_timeout_add_once(50, on_analysis_updated_debounced, user_data);
}

static void on_analysis_reset_clicked(GtkWidget *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    if (!app) {
        fprintf(stderr, "[Error] Invalid app in on_analysis_reset_clicked\n");
        return;
    }

    app->params.analysis_type = ANALYSIS_BODE;
    app->params.analysis_freq_min = 0.1;
    app->params.analysis_freq_max = 10000.0;
    app->params.analysis_op_voltage = 220.0;
    app->params.analysis_op_load = 10.0;
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->analysis_type_dropdown), 0);
    gtk_range_set_value(GTK_RANGE(app->analysis_freq_min_scale), app->params.analysis_freq_min);
    gtk_range_set_value(GTK_RANGE(app->analysis_freq_max_scale), app->params.analysis_freq_max);
    gtk_range_set_value(GTK_RANGE(app->analysis_op_voltage_scale), app->params.analysis_op_voltage);
    gtk_range_set_value(GTK_RANGE(app->analysis_op_load_scale), app->params.analysis_op_load);
    g_timeout_add_once(50, on_analysis_updated_debounced, app);
}

void analysis_window_create(AppData *app) {
    if (!app) {
        fprintf(stderr, "[Error] Invalid app in analysis_window_create\n");
        return;
    }

    app->analysis_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(app->analysis_window), "Frequency Analysis");
    gtk_window_set_default_size(GTK_WINDOW(app->analysis_window), 800, 600);
    gtk_window_set_transient_for(GTK_WINDOW(app->analysis_window), GTK_WINDOW(app->window));

    // Main box with horizontal layout
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_window_set_child(GTK_WINDOW(app->analysis_window), main_box);

    // Control box (vertical)
    GtkWidget *control_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_size_request(control_box, 200, -1);
    gtk_box_append(GTK_BOX(main_box), control_box);

    // Analysis type dropdown (Bode only)
    GtkWidget *type_label = gtk_label_new("Analysis Type:");
    gtk_box_append(GTK_BOX(control_box), type_label);
    const char *types[] = { "Bode", NULL };
    app->analysis_type_dropdown = gtk_drop_down_new_from_strings(types);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->analysis_type_dropdown), 0);
    gtk_box_append(GTK_BOX(control_box), app->analysis_type_dropdown);

    // Frequency range
    GtkWidget *freq_min_label = gtk_label_new("Min Freq (Hz):");
    gtk_box_append(GTK_BOX(control_box), freq_min_label);
    app->analysis_freq_min_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.01, 100.0, 0.01);
    gtk_range_set_value(GTK_RANGE(app->analysis_freq_min_scale), app->params.analysis_freq_min);
    gtk_box_append(GTK_BOX(control_box), app->analysis_freq_min_scale);

    GtkWidget *freq_max_label = gtk_label_new("Max Freq (Hz):");
    gtk_box_append(GTK_BOX(control_box), freq_max_label);
    app->analysis_freq_max_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 100.0, 100000.0, 10.0);
    gtk_range_set_value(GTK_RANGE(app->analysis_freq_max_scale), app->params.analysis_freq_max);
    gtk_box_append(GTK_BOX(control_box), app->analysis_freq_max_scale);

    // Operating conditions
    GtkWidget *op_voltage_label = gtk_label_new("Op Voltage (V):");
    gtk_box_append(GTK_BOX(control_box), op_voltage_label);
    app->analysis_op_voltage_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 100.0, 300.0, 1.0);
    gtk_range_set_value(GTK_RANGE(app->analysis_op_voltage_scale), app->params.analysis_op_voltage);
    gtk_box_append(GTK_BOX(control_box), app->analysis_op_voltage_scale);

    GtkWidget *op_load_label = gtk_label_new("Op Load (Ω):");
    gtk_box_append(GTK_BOX(control_box), op_load_label);
    app->analysis_op_load_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1.0, 1000.0, 5.0);
    gtk_range_set_value(GTK_RANGE(app->analysis_op_load_scale), app->params.analysis_op_load);
    gtk_box_append(GTK_BOX(control_box), app->analysis_op_load_scale);

    // Buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_append(GTK_BOX(control_box), button_box);

    app->analysis_run_button = gtk_button_new_with_label("Run Analysis");
    gtk_box_append(GTK_BOX(button_box), app->analysis_run_button);

    app->analysis_reset_button = gtk_button_new_with_label("Reset");
    gtk_box_append(GTK_BOX(button_box), app->analysis_reset_button);

    // Drawing area
    app->analysis_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(app->analysis_drawing_area, TRUE);
    gtk_widget_set_vexpand(app->analysis_drawing_area, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(app->analysis_drawing_area), analysis_draw_callback, app, NULL);
    gtk_box_append(GTK_BOX(main_box), app->analysis_drawing_area);

    // Signals
    g_signal_connect(app->analysis_type_dropdown, "notify::selected", G_CALLBACK(on_analysis_param_changed), app);
    g_signal_connect(app->analysis_freq_min_scale, "value-changed", G_CALLBACK(on_analysis_param_changed), app);
    g_signal_connect(app->analysis_freq_max_scale, "value-changed", G_CALLBACK(on_analysis_param_changed), app);
    g_signal_connect(app->analysis_op_voltage_scale, "value-changed", G_CALLBACK(on_analysis_param_changed), app);
    g_signal_connect(app->analysis_op_load_scale, "value-changed", G_CALLBACK(on_analysis_param_changed), app);
    g_signal_connect(app->analysis_run_button, "clicked", G_CALLBACK(on_analysis_run_clicked), app);
    g_signal_connect(app->analysis_reset_button, "clicked", G_CALLBACK(on_analysis_reset_clicked), app);

    // Initial draw
    gtk_widget_queue_draw(app->analysis_drawing_area);
}

void frequency_domain_analysis(AppData *app, double *freq, double *gain, double *phase, int *n_points) {
    if (!app) {
        fprintf(stderr, "[Error] Invalid app in frequency_domain_analysis\n");
        *n_points = 0;
        return;
    }

    double L = 1e-3; // Inductance (H)
    double R = app->params.analysis_op_load > 0.0 ? app->params.analysis_op_load : 10.0;
    double C = 10e-6; // Capacitance (F)
    double kp = (app->params.control == CONTROL_PI || app->params.control == CONTROL_PR) ? app->params.pll_kp : 0.5;
    double ki = (app->params.control == CONTROL_PI || app->params.control == CONTROL_PR) ? app->params.pll_ki : 10.0;

    int n = 1000;
    double f_min = app->params.analysis_freq_min > 0.0 ? app->params.analysis_freq_min : 0.1;
    double f_max = app->params.analysis_freq_max > f_min ? app->params.analysis_freq_max : f_min * 1000.0;

    for (int i = 0; i < n; i++) {
        double f = f_min * pow(f_max / f_min, (double)i / (n - 1));
        freq[i] = f;
        double w = 2.0 * M_PI * f;

        double complex s = w * I;
        double complex controller = kp + ki / s;
        double complex plant = 1.0 / (s * L + R + 1.0 / (s * C));
        double complex G = controller * plant;
        double mag = cabs(G);
        gain[i] = mag > 0.0 ? 20.0 * log10(mag) : -100.0;
        phase[i] = carg(G) * 180.0 / M_PI;
    }
    *n_points = n;
}