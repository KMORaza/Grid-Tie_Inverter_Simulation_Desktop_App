#ifndef INVERTER_H
#define INVERTER_H

#include <gtk/gtk.h>
#include <math.h>

// Enum for inverter topology
typedef enum {
    SINGLE_PHASE,
    THREE_PHASE,
    NPC_INVERTER,
    FLYING_CAPACITOR,
    CASCADED_H_BRIDGE
} InverterType;

// Enum for design type
typedef enum {
    TRANSFORMERLESS,
    TRANSFORMER_BASED
} DesignType;

// Enum for MPPT type
typedef enum {
    MPPT_NONE,
    MPPT_PERTURB_OBSERVE,
    MPPT_INCREMENTAL_CONDUCTANCE
} MPPTType;

// Enum for control type
typedef enum {
    CONTROL_NONE,
    CONTROL_PI,
    CONTROL_PR,
    CONTROL_SMC,
    CONTROL_MPC
} ControlType;

// Enum for grid condition
typedef enum {
    GRID_NORMAL,
    GRID_WEAK,
    GRID_FAULT_SAG,
    GRID_FAULT_SWELL,
    GRID_FAULT_HARMONICS,
    GRID_FAULT_FREQ_SHIFT
} GridCondition;

// Enum for DC source
typedef enum {
    DC_SOURCE_PV,
    DC_SOURCE_BATTERY,
    DC_SOURCE_FUEL_CELL,
    DC_SOURCE_HYBRID
} DCSourceType;

// Structure to hold inverter parameters
typedef struct {
    double voltage; // Output voltage amplitude (V, RMS)
    double frequency; // Output frequency (Hz)
    double phase; // Phase shift (radians, for single-phase or first phase)
    gboolean running; // Simulation running state
    InverterType type; // Inverter type
    DesignType design; // Transformerless or transformer-based
    MPPTType mppt; // MPPT algorithm
    double mppt_voltage; // Voltage adjusted by MPPT
    double prev_power; // Previous power for MPPT
    double prev_voltage; // Previous voltage for MPPT
    gboolean pll_enabled; // PLL enabled state
    double pll_phase; // PLL-adjusted phase
    double pll_frequency; // PLL-adjusted frequency
    double pll_voltage; // PLL-adjusted voltage
    gboolean pll_locked; // PLL lock status
    double pll_kp; // PLL proportional gain
    double pll_ki; // PLL integral gain
    ControlType control; // Control algorithm
    double control_output; // Control-adjusted output (duty cycle)
    double control_ref_current; // Reference current for control
    double control_ref_voltage; // Reference voltage for control
    gboolean islanding_enabled; // Islanding detection enabled
    gboolean islanding_detected; // Islanding status
    GridCondition grid_condition; // Grid condition
    DCSourceType dc_source; // DC source type
    double dc_voltage; // DC source output voltage
    double dc_current; // DC source output current
    double pv_irradiance; // PV: W/m²
    double pv_temperature; // PV: °C
    int pv_ns; // PV: Panels in series
    int pv_np; // PV: Panels in parallel
    double battery_soc; // Battery: State of charge (0–1)
    double battery_capacity; // Battery: Ah
    gboolean battery_charging; // Battery: Charging state
    double fuel_cell_power; // Fuel cell: Power demand (W)
    double sim_time; // Simulation time (s)
    double max_dt; // Maximum time step (s)
    double prev_output[3]; // Previous inverter output for dynamics
} InverterParams;

// Structure to hold application data
typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *voltage_scale;
    GtkWidget *frequency_scale;
    GtkWidget *phase_scale;
    GtkWidget *type_dropdown;
    GtkWidget *design_dropdown;
    GtkWidget *mppt_dropdown;
    GtkWidget *pll_switch;
    GtkWidget *pll_kp_scale;
    GtkWidget *pll_ki_scale;
    GtkWidget *pll_lock_label;
    GtkWidget *control_dropdown;
    GtkWidget *start_button;
    GtkWidget *pause_button;
    GtkWidget *reset_button;
    GtkWidget *islanding_switch;
    GtkWidget *islanding_status_label;
    GtkWidget *grid_dropdown;
    GtkWidget *grid_status_label;
    GtkWidget *dc_source_dropdown;
    GtkWidget *dc_source_button;
    GtkWidget *dc_source_window;
    GtkWidget *dc_irradiance_scale;
    GtkWidget *dc_temperature_scale;
    GtkWidget *dc_ns_scale;
    GtkWidget *dc_np_scale;
    GtkWidget *dc_soc_scale;
    GtkWidget *dc_battery_type_dropdown;
    GtkWidget *dc_charge_button;
    GtkWidget *dc_fuel_cell_switch;
    GtkWidget *dc_fuel_cell_power_scale;
    GtkWidget *dc_voltage_label;
    GtkWidget *dc_current_label;
    GtkWidget *dc_soc_label;
    GtkWidget *dc_power_label;
    GtkWidget *timestep_scale; // New: Max time-step slider
    InverterParams params;
    guint timeout_id; // For animation
} AppData;

// Function prototypes
// inverter.c
void inverter_init(InverterParams *params);
void inverter_get_output(InverterParams *params, double time, double *output);

// interface.c
void build_interface(AppData *app);

// waveform.c
void waveform_draw(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data);

// Wechselrichtertopologie.c
void single_phase_output(InverterParams *params, double time, double *output);
void three_phase_output(InverterParams *params, double time, double *output);

// MehrstufigerWechselrichter.c
void npc_inverter_output(InverterParams *params, double time, double *output);
void flying_capacitor_output(InverterParams *params, double time, double *output);
void cascaded_h_bridge_output(InverterParams *params, double time, double *output);

// TransformatorlosUndTransformatorbasiert.c
void apply_transformerless(InverterParams *params, double *output);
void apply_transformer_based(InverterParams *params, double *output);

// MaximaleLeistungspunktverfolgung.c
double pv_model_power(double voltage);
void mppt_perturb_observe(InverterParams *params);
void mppt_incremental_conductance(InverterParams *params);
double dc_source_get_power(AppData *app, double voltage, double *current);

// Phasenregelkreis.c
void pll_update(InverterParams *params, double time);

// StromUndSpannungsregelung.c
void control_update(InverterParams *params, double time);

// IslandingDetectionMechanism.c
void islanding_detection_update(InverterParams *params, double time);

// GridSimulation.c
double grid_simulation_voltage(InverterParams *params, double t, double inverter_current, double *frequency, double *amplitude, gboolean *grid_connected);

// GleichstromquellenModellierung.c
void dc_source_window_create(AppData *app);
void dc_source_update(AppData *app);

// Zeitbereichssimulation.c
double calculate_time_step(AppData *app);
void simulation_step(AppData *app, double dt);
gboolean simulation_update(gpointer user_data);

#endif // INVERTER_H