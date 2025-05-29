#include "inverter.h"

void waveform_draw(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    cairo_set_source_rgb(cr, 1, 1, 1); // White background (default theme)
    cairo_paint(cr);

    // Draw grid
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8); // Gray grid
    cairo_set_line_width(cr, 1);
    for (int i = 0; i <= 10; i++) {
        double x = i * width / 10.0;
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, height);
        double y = i * height / 10.0;
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, width, y);
    }
    cairo_stroke(cr);

    // Draw waveform(s)
    double time_step = 0.02 / app->params.frequency; // Show one cycle
    // Scale for transformer-based design (1.1 turns ratio)
    double scale_factor = (app->params.design == TRANSFORMER_BASED) ? 1.1 : 1.0;
    double scale_y = height / (2.0 * app->params.voltage * sqrt(2) * scale_factor); // Adjust for peak voltage
    double time = g_get_monotonic_time() / 1e6; // Current time in seconds
    double output[3] = {0.0, 0.0, 0.0};

    // Colors for waveforms: Blue, Green, Red
    double colors[3][3] = {{0.0, 0.6, 1.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}};
    int num_phases = (app->params.type == SINGLE_PHASE || app->params.type == NPC_INVERTER || app->params.type == FLYING_CAPACITOR) ? 1 : 3;

    for (int phase = 0; phase < num_phases; phase++) {
        cairo_set_source_rgb(cr, colors[phase][0], colors[phase][1], colors[phase][2]);
        cairo_set_line_width(cr, 2);
        for (int x = 0; x < width; x++) {
            double t = x * time_step / width;
            inverter_get_output(&app->params, time + t, output);
            double y = height / 2.0 - output[phase] * scale_y;
            if (x == 0) {
                cairo_move_to(cr, x, y);
            } else {
                cairo_line_to(cr, x, y);
            }
        }
        cairo_stroke(cr);
    }

    // Draw grid reference (zero line)
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5); // Gray reference
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, 0, height / 2.0);
    cairo_line_to(cr, width, height / 2.0);
    cairo_stroke(cr);
}