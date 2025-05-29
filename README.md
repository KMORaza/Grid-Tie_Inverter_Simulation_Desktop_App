# Grid-Tie Inverter Simulation Desktop App

This is a desktop-app for simulating grid-tied inverters, supporting various topologies, control strategies, DC sources, and grid conditions. It provides a graphical interface for configuring parameters, visualizing waveforms, and performing frequency-domain analysis (Bode plots). The software is designed for modeling inverter behavior under different operating conditions.

## Functioning Logic

1. **Main Application (`main.c`)**:
   - Initializes a GTK application and creates a main window titled "Grid-Tie Inverter Simulator" (800x600 pixels).
   - Manages the simulation loop using `g_timeout_add` (16ms interval, ~60 FPS) to call `simulation_update`.
   - Handles user interactions via signal callbacks for start, pause, reset, and parameter changes (e.g., voltage, frequency, inverter type).
   - Resets parameters to defaults and updates GUI elements when the reset button is clicked.
2. **User Interface (`interface.c`, `style.css`)**:
   - Features a horizontal layout with a scrollable control panel and a waveform drawing area.
   - Control panel includes:
     - Dropdowns: Inverter type (Single-Phase, Three-Phase, NPC, Flying Capacitor, Cascaded H-Bridge), design (Transformerless, Transformer-based), MPPT (None, Perturb & Observe, Incremental Conductance), control (None, PI, PR, SMC, MPC), DC source (PV, Battery, Fuel Cell, Hybrid), and grid condition (Normal, Weak, Faults).
     - Sliders: Voltage (100–300V), frequency (40–60 Hz), phase (0–2π rad), PLL gains (Kp: 0.1–2, Ki: 1–50), and max time step (0.1–10ms).
     - Buttons: Start (toggle), Pause/Resume, Reset, Configure DC, and Frequency/Small-Signal Analysis.
     - Status labels: PLL lock, islanding status, grid condition, DC voltage/current/SoC/power.
   - Uses a teal-themed CSS with Fixedsys font, outset/inset borders, and hover/active effects for a retro aesthetic.
3. **DC Source Configuration (`GleichstromquellenModellierung.c`)**:
   - Provides a separate window for configuring DC source parameters:
     - PV: Irradiance (200–1000 W/m²), temperature (0–50°C), series panels (1–10), parallel strings (1–5).
     - Battery: State of Charge (20–90%), type (Li-ion, Lead-acid), charging state.
     - Fuel Cell: Power demand (0–1000W).
   - Updates parameters in real-time and recalculates DC voltage/current when changed.
4. **Frequency Analysis (`FrequenzbereichsUndKleinsignalanalyse.c`)**:
   - Creates a window for frequency-domain analysis, currently supporting Bode plots.
   - Allows configuration of analysis type (Bode only), frequency range (0.01–100k Hz), operating voltage (100–300V), and load resistance (1–1000Ω).
   - Draws gain (top half) and phase (bottom half) plots using Cairo on a logarithmic frequency scale.
5. **Simulation Loop (`Zeitbereichssimulation.c`)**:
   - Manages the simulation by calculating an adaptive time step and updating the inverter state via `simulation_step`.
   - Updates MPPT, PLL, control, islanding detection, and DC source models each step.
   - Refreshes GUI labels (PLL lock, islanding status, grid condition, DC parameters) and redraws waveforms.
6. **Waveform Visualization (`waveform.c`)**:
   - Draws inverter output waveforms (single-phase or three-phase) using Cairo.
   - Uses a black background with a white grid (10 vertical, 9 horizontal lines) and colored lines (red for phase A, green for B, blue for C).
   - Scales output voltages to ±400V, plotting one sample per pixel over a 1ms time step.

## Simulation Logic


1. **Time Step Calculation (`calculate_time_step` in `Zeitbereichssimulation.c`)**:
   - Adapts the time step based on:
     - Frequency deviation: |PLL_frequency - 50| / 50
     - Output change: sqrt((output[0] - prev_output[0])^2 + (output[1] - prev_output[1])^2 + (output[2] - prev_output[2])^2)
   - Scales time step:
     - 10% of max_dt for large deviations (>5% frequency or >10V output change).
     - 50% of max_dt for moderate deviations (>2% frequency or >5V output change).
     - Clamps between 0.1ms and max_dt (default 10ms, user-configurable).
   - Ensures smooth simulation during transients while maintaining performance.
2. **Simulation Step (`simulation_step` in `Zeitbereichssimulation.c`)**:
   - Increments simulation time: sim_time = sim_time + dt
   - Updates:
     - MPPT (Perturb & Observe or Incremental Conductance) if enabled.
     - PLL (phase, frequency, voltage) if enabled.
     - Control algorithm (PI, PR, SMC, MPC) if enabled.
     - Islanding detection if enabled.
     - DC source (PV, battery, fuel cell, or hybrid).
   - Applies updates sequentially to reflect dependencies (e.g., MPPT affects DC voltage, PLL affects phase).
3. **Simulation Update (`simulation_update` in `Zeitbereichssimulation.c`)**:
   - Called every 16ms if the simulation is running.
   - Calculates adaptive time step, performs `simulation_step`, and updates GUI:
     - PLL lock: “Locked” if phase error < 0.1 * grid_amplitude * inverter_voltage * sqrt(2), else “Not Locked.”
     - Islanding: “Islanding Detected” or “Grid Connected” based on detection.
     - Grid condition: Reflects user selection (e.g., “Voltage Sag”).
     - DC source: Updates Vdc, Idc, Battery SoC, and Power labels.
   - Redraws waveforms via `gtk_widget_queue_draw`.
   - Stops simulation if islanding is detected, resetting start button and pause label.

## Physics Models

1. **Inverter Output (`inverter.c`, `Wechselrichtertopologie.c`, `MehrstufigerWechselrichter.c`)**:
   - **Single-Phase (`single_phase_output`)**:
     - output[0] = V_rms * sqrt(2) * sin(2 * pi * f * t + phase)
     - output[1] = 0, output[2] = 0
   - **Three-Phase (`three_phase_output`)**:
     - output[0] = V_rms * sqrt(2) * sin(2 * pi * f * t + phase)
     - output[1] = V_rms * sqrt(2) * sin(2 * pi * f * t + phase + 2 * pi / 3)
     - output[2] = V_rms * sqrt(2) * sin(2 * pi * f * t + phase + 4 * pi / 3)
   - **NPC Inverter (`npc_inverter_output`)**:
     - Peak voltage: V_peak = V_rms * sqrt(2)
     - Angle: angle = mod(2 * pi * f * t + phase, 2 * pi)
     - Output: -V_peak if angle < pi/3 or >= 5 * pi/3; V_peak if 2 * pi/3 <= angle < 4 * pi/3; else 0
     - output[0] = level, output[1] = 0, output[2] = 0
   - **Flying Capacitor (`flying_capacitor_output`)**:
     - Peak voltage: V_peak = V_rms * sqrt(2)
     - Angle: angle = mod(2 * pi * f * t + phase, 2 * pi)
     - Output: -V_peak if angle < pi/5 or >= 9 * pi/5; -V_peak/2 if angle < 2 * pi/5; V_peak/2 if angle < 4 * pi/5; V_peak if 4 * pi/5 <= angle < 7 * pi/5; else 0
     - output[0] = level, output[1] = 0, output[2] = 0
   - **Cascaded H-Bridge (`cascaded_h_bridge_output`)**:
     - Similar to NPC but applied to three phases with angles: phase, phase + 2 * pi/3, phase + 4 * pi/3
2. **Transformer Effects (`TransformatorlosUndTransformatorbasiert.c`)**:
   - **Transformerless (`apply_transformerless`)**:
     - output[i] = output[i] * 0.98 (98% efficiency)
   - **Transformer-Based (`apply_transformer_based`)**:
     - output[i] = output[i] * 1.1 * 0.90 (1.1 turns ratio, 90% efficiency)
3. **DC Sources (`GleichstromquellenModellierung.c`)**:
   - **PV Model (`pv_current`)**:
     - Photocurrent: Iph = (Isc + Ki * (T - 25)) * (G / 1000) * Np
       - Isc = 8.21A, Ki = 0.00065 A/°C, Gref = 1000 W/m², Tref = 25°C
     - Thermal voltage: Vt = Ns * n * k * T / q
       - n = 1.3, k = 1.381e-23 J/K, q = 1.602e-19 C
     - Saturation current: Io = Isc / (exp((Voc + Kv * (T - 25)) / Vt) - 1)
       - Voc = 37.6V, Kv = -0.123 V/°C
     - Current (Newton-Raphson, 5 iterations): I = Iph - Io * (exp((V / Ns + I * Rs) / Vt) - 1) - (V / Ns + I * Rs) / Rsh
       - Rs = 0.221Ω, Rsh = 415.405Ω
     - Total current: I * Np
   - **Battery Model (`battery_voltage`)**:
     - Nominal voltage: Vnom = 48V (Li-ion) or 12V (Lead-acid)
     - Efficiency: eta = 0.95 (charging) or 0.98 (discharging) for Li-ion; *0.9 for Lead-acid
     - Open-circuit voltage: V0 = Vnom * (1 + 0.1 * (SoC - 0.5))
     - Internal resistance: R_int = 0.05 * (1 / SoC)
     - Voltage: V = V0 - I * R_int
     - SoC update: dAh = (I * eta if charging else -I / eta) * (dt / 3600), SoC = SoC + dAh / capacity
       - Clamped: 0.2 <= SoC <= 0.9
   - **Fuel Cell Model (`fuel_cell_voltage`)**:
     - Nominal voltage: V0 = 48V
     - Voltage: V = V0 - A * ln(I + 1) - I * R - m * exp(n * I)
       - A = 0.06, R = 0.01Ω, m = 0.05, n = 0.0001
     - Limits I to meet power demand: I = P_demand / V if V * I > P_demand
   - **Hybrid Model**:
     - Fuel cell provides base current: I = P_demand / V
     - Battery supplies additional current: I_extra = control_ref_current - I
     - Voltage from fuel cell or battery based on demand.
4. **Grid Model (`GridSimulation.c`)**:
   - Grid impedance:
     - Normal: R = 0.4Ω, L = 0.001H
     - Weak: R = 1.0Ω, L = 0.005H
     - Reactance: X = 2 * pi * 50 * L
   - Nominal: V_nom = 220V RMS, f_nom = 50 Hz
   - Voltage: V_grid = V_nom * fault_factor * sqrt(2) * sin(2 * pi * (f_nom + freq_shift) * t) + harmonic
     - Faults (t = 1–1.5s):
       - Sag: fault_factor = 0.5
       - Swell: fault_factor = 1.2
       - Harmonics: harmonic = 0.05 * sin(3 * 2 * pi * f_nom * t) + 0.03 * sin(5 * 2 * pi * f_nom * t) + 0.02 * sin(7 * 2 * pi * f_nom * t)
       - Frequency Shift: freq_shift = 2 Hz
   - PCC voltage: V_pcc = V_grid - I_inverter * R - I_inverter * X * cos(2 * pi * f * t)
   - Random disconnection: 5% chance per second after t > 1s.

## Algorithms

1. **MPPT Algorithms (`MaximaleLeistungspunktverfolgung.c`)**:
   - **Perturb & Observe (`mppt_perturb_observe`)**:
     - Perturbs voltage by delta_v = 1V.
     - If power > prev_power, continue in same direction; else reverse.
     - Updates: mppt_voltage = mppt_voltage ± delta_v, prev_power = V * I, prev_voltage = mppt_voltage
     - Limits: 100V <= mppt_voltage <= 300V
   - **Incremental Conductance (`mppt_incremental_conductance`)**:
     - Calculates: g_inc = delta_i / delta_v, g = I / V
       - delta_i = I - (prev_power / prev_voltage), delta_v = V - prev_voltage
     - If |g_inc + g| < 0.01, at MPP; else if g_inc + g > 0, increase V; else decrease V.
     - Perturbs by delta_v = 1V if delta_v < 0.01.
     - Updates and limits similar to Perturb & Observe.
2. **PLL (`Phasenregelkreis.c`)**:
   - Simulates grid: V_grid = (220 + sin(0.2 * t) * 10) * sqrt(2) * sin(2 * pi * (50 + sin(0.1 * t)) * t)
   - Frequency estimation via zero-crossing:
     - Detects positive zero-crossings, calculates period after two crossings: period = (time - last_zero_cross) / (cross_count - 1)
     - Frequency: f = 1 / period
   - Voltage tracking: V_pll = V_pll + 0.1 * (grid_ampl - V_pll) * dt
   - Phase detector: error = V_grid * V_inv
   - PI controller: phase_correction = Kp * error + Ki * integral, integral = integral + error * dt
   - Phase update: pll_phase = mod(pll_phase + phase_correction * dt, 2 * pi)
   - Lock status: locked if |error| < 0.1 * grid_ampl * V_inv * sqrt(2)
3. **Control Algorithms (`StromUndSpannungsregelung.c`)**:
   - **Plant Model**:
     - V_inv = duty * V_rms * sqrt(2) * sin(2 * pi * f * t + phase)
     - V_grid = V_grid_rms * sqrt(2) * sin(2 * pi * f * t)
     - Current: di/dt = (V_inv - V_grid - R * I) / L, I = I_prev + di/dt * dt
       - R = 10Ω, L = 0.01H, dt = 0.05s
   - **PI Control**:
     - error = I_ref - I_meas, I_ref = I_ref_ampl * sin(2 * pi * f * t + phase)
     - u = Kp * error + Ki * integral, integral = integral + error * dt
     - Kp = 0.1, Ki = 5.0
   - **PR Control**:
     - Adds resonant term: u = Kp * error + Ki * integral + Kr * sin(2 * pi * f * t) * error
     - Kr = 50.0
   - **SMC (Sliding Mode Control)**:
     - Sliding surface: s = error + c * (error - prev_error) / dt
     - u = k * (s > 0 ? 1 : -1)
     - c = 0.01, k = 0.5
   - **MPC (Model Predictive Control)**:
     - Tests duty cycles (0 to 1, step 0.1) over 2 steps (100ms horizon).
     - Cost: sum((I_ref_future - I_future)^2) for t_future = t + j * dt
     - Selects duty with minimum cost.
   - Clamps duty: 0 <= control_output <= 1
4. **Islanding Detection (`IslandingDetectionMechanism.c`)**:
   - **Passive Methods**:
     - OUV: Detects if V < 193.6V or V > 242V (0.88–1.1 pu).
     - OUF: Detects if f < 49 Hz or f > 51 Hz.
     - ROCOF: |f - prev_f| / (t - prev_t) > 1 Hz/s.
   - **Active Method (AFS)**:
     - Perturbs frequency: f = f + 0.5 * sin(2 * pi * 0.1 * t) if grid connected.
     - Detects islanding if |f - 50| > 0.7 Hz in islanded mode.
5. **Frequency Analysis (`FrequenzbereichsUndKleinsignalanalyse.c`)**:
   - Models controller: G_controller = Kp + Ki / s
     - Kp = pll_kp (default 0.5), Ki = pll_ki (default 10)
   - Models plant: G_plant = 1 / (s * L + R + 1 / (s * C))
     - L = 0.001H, C = 10e-6F, R = analysis_op_load
   - Transfer function: G = G_controller * G_plant
   - Gain: 20 * log10(|G|) dB
   - Phase: arg(G) * 180 / pi degrees
   - Evaluates over 1000 points from f_min to f_max (logarithmic).

## Equations 

- **Inverter Output**:
  - Single-Phase: V = V_rms * sqrt(2) * sin(2 * pi * f * t + phase)
  - Three-Phase: V_i = V_rms * sqrt(2) * sin(2 * pi * f * t + phase + i * 2 * pi / 3), i = 0,1,2
  - NPC/Flying Capacitor: Multi-level outputs based on angle thresholds
  - Transformerless: V_out = V_in * 0.98
  - Transformer-Based: V_out = V_in * 1.1 * 0.90
- **PV Model**:
  - Iph = (8.21 + 0.00065 * (T - 25)) * (G / 1000) * Np
  - Vt = Ns * 1.3 * 1.381e-23 * T / 1.602e-19
  - Io = 8.21 / (exp((37.6 - 0.123 * (T - 25)) / Vt) - 1)
  - I = Iph - Io * (exp((V / Ns + I * 0.221) / Vt) - 1) - (V / Ns + I * 0.221) / 415.405
- **Battery Model**:
  - V0 = Vnom * (1 + 0.1 * (SoC - 0.5))
  - R_int = 0.05 * (1 / SoC)
  - V = V0 - I * R_int
  - dAh = (I * eta if charging else -I / eta) * (dt / 3600), SoC = SoC + dAh / capacity
- **Fuel Cell Model**:
  - V = 48 - 0.06 * ln(I + 1) - I * 0.01 - 0.05 * exp(0.0001 * I)
  - I = P_demand / V if V * I > P_demand
- **Grid Model**:
  - V_grid = V_nom * fault_factor * sqrt(2) * sin(2 * pi * (f_nom + freq_shift) * t) + harmonic
  - V_pcc = V_grid - I_inverter * R - I_inverter * X * cos(2 * pi * f * t)
- **PLL**:
  - error = V_grid * V_inv
  - phase_correction = Kp * error + Ki * integral
  - pll_phase = mod(pll_phase + phase_correction * dt, 2 * pi)
  - V_pll = V_pll + 0.1 * (grid_ampl - V_pll) * dt
- **Control**:
  - Plant: di/dt = (V_inv - V_grid - R * I) / L, I = I_prev + di/dt * dt
  - PI: u = Kp * error + Ki * integral
  - PR: u = Kp * error + Ki * integral + Kr * sin(2 * pi * f * t) * error
  - SMC: s = error + c * (error - prev_error) / dt, u = k * (s > 0 ? 1 : -1)
  - MPC: cost = sum((I_ref_future - I_future)^2), select min cost duty
- **Frequency Analysis**:
  - G = (Kp + Ki / s) * (1 / (s * L + R + 1 / (s * C)))
  - Gain = 20 * log10(|G|)
  - Phase = arg(G) * 180 / pi

## Screenshots 

| ![](https://github.com/KMORaza/Grid-Tie_Inverter_Simulation_Desktop_App/blob/main/Grid%20Tie%20Inverter%20Simulation%20-%20Desktop%20App/screenshot.png) |
|----------------------------------------------------------------------------------------------------------------------------------------------------------|
