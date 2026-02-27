# Autopilot Implementation & Tuning Notes

## Simulator Data Source

Simulator data (pitch, roll, heading, altitude, vertical speed, etc.) is received from the flight sim via UDP/JSON. For **Microsoft Flight Simulator (MSFS)**, the data is provided by the [msfs-web-api](https://github.com/me2d13/msfs-web-api) tool, which streams SimConnect variables via UDP.

## Vertical Speed (VS) Mode Enhancement
The Vertical Speed mode uses a cascaded PI architecture to maintain a selected climb or descent rate.

### 1. Cascaded PI Control Loop
Instead of a simple proportional controller, VS mode uses a dual-loop strategy:
*   **Outer Loop (Navigation)**: Operates on VS error (ft/min). It calculates the required **Target Pitch** angle using a PI controller.
*   **Inner Loop (Attitude)**: A high-speed PID that holds the **Target Pitch** by adjusting the joystick cyclic Y output.

### 2. Implementation Details
*   **Sign Convention**: In this simulator/flight model, **Positive Pitch angle means Nose DOWN**. 
*   **Error Calculation**: To achieve negative feedback, the VS error is calculated as `(Simulator VS - Target VS)`. 
    *   Example: If Sim is climbing faster than target, error is positive, commanding a positive pitch increase (Nose DOWN).
*   **Bumpless Transfer**:
    *   **Integrator Seeding**: When VS mode is engaged, the integrator is "seeded" with the current pitch attitude. This prevents sudden jumps and ensures a smooth capture starting from the pilot's manual trim.
    *   **Stick Baseline**: The inner PID loops are initialized with the physical stick position at the moment of engagement.
*   **Anti-Windup**: The integrator is clamped to the `AP_MAX_PITCH_ANGLE` (10°) to ensure the autopilot stays within safe helicopter flight envelopes.
*   **Target Smoothing**: To prevent jerky movements, the navigation loop's output is smoothed using a low-pass filter (alpha=0.1).

### 3. Tuning Constants (Current)
These values are defined in `include/config.h`:
*   `AP_VS_KP`: `0.005` (Low gain to prevent pendulum oscillations)
*   `AP_VS_KI`: `0.0002` (Dampened integral to eliminate steady-state error slowly/stably)
*   `Inner Loop Mode`: `REVERSE` (Confirmed correct for joystick -> Sim interaction)
*   `Smoothing Alpha`: `0.1` (10% transition per 20Hz update)

## Telemetry-Based Tuning
The system uses the `web_server` telemetry stream to capture `(sim_vs, target_vs, target_pitch, joystick_out)`. This data was critical in identifying the non-standard pitch convention and stabilizing the outer loop gains.