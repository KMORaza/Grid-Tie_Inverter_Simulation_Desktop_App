#include "inverter.h"

void waveform_draw(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data) {
    AppData *app = (AppData *)user_data;

    // Set black background
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black
    cairo_paint(cr);

    // Draw white grid
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White
    cairo_set_line_width(cr, 0.5);

    // Vertical grid lines (time)
    for (int i = 0; i <= 10; i++) {
        double x = i * width / 10.0;
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, height);
        cairo_stroke(cr);
    }

    // Horizontal grid lines (amplitude)
    for (int i = -4; i <= 4; i++) {
        double y = height / 2.0 - (i * height / 8.0);
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, width, y);
        cairo_stroke(cr);
    }

    // Draw waveforms
    double time = g_get_monotonic_time() / 1e6; // Current time in seconds
    double output[3] = {0.0, 0.0, 0.0}; // For single or three-phase output

    // Time step for drawing (1ms)
    double dt = 0.001;
    int samples = width; // One sample per pixel

    // Plot three-phase or single-phase output
    for (int phase = 0; phase < (app->params.type == SINGLE_PHASE ? 1 : 3); phase++) {
        // Set color for each phase
        switch (phase) {
            case 0: cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); break; // Red
            case 1: cairo_set_source_rgb(cr, 0.0, 1.0, 0.0); break; // Green
            case 2: cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); break; // Blue
        }
        cairo_set_line_width(cr, 2.0);

        // Start path
        for (int i = 0; i < samples; i++) {
            double t = time - (samples - i) * dt;
            inverter_get_output(&app->params, t, output);
            double value = output[phase];
            double x = i * width / (double)samples;
            double y = height / 2.0 - (value / 400.0) * (height / 2.0); // Scale to ±400V
            if (i == 0) {
                cairo_move_to(cr, x, y);
            } else {
                cairo_line_to(cr, x, y);
            }
        }
        cairo_stroke(cr);
    }
}