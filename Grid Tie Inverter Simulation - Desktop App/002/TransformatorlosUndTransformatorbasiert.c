#include "inverter.h"

void apply_transformerless(InverterParams *params, double *output) {
    // Transformerless: Direct output with 98% efficiency
    const double efficiency = 0.98;
    for (int i = 0; i < 3; i++) {
        output[i] *= efficiency;
    }
}

void apply_transformer_based(InverterParams *params, double *output) {
    // Transformer-based: Apply turns ratio (1:1.1) and 90% efficiency
    const double turns_ratio = 1.1; // Voltage boost
    const double efficiency = 0.90;
    for (int i = 0; i < 3; i++) {
        output[i] *= turns_ratio * efficiency;
    }
}