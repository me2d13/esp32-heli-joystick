let ws;
let updateCount = 0;
let lastSecond = Date.now();
let currentHz = 0;
let lastSimData = {};
let selectedVS = 0;
let selectedAltitude = 0;
let isRecording = false;

// Axis range: 0-10000
const AXIS_MIN = 0;
const AXIS_MAX = 10000;

// Create button grid (small squares with 1-based number)
const buttonsGrid = document.getElementById('buttonsGrid');
for (let i = 0; i < 32; i++) {
    const btn = document.createElement('div');
    btn.className = 'button';
    btn.id = 'btn' + i;
    btn.textContent = i + 1;
    buttonsGrid.appendChild(btn);
}

// Map axis value (0-10000) to X position % in box (0=left, 10000=right)
function axisToXPercent(value) {
    return Math.max(0, Math.min(100, (value / AXIS_MAX) * 100));
}

// Map axis value (0-10000) to Y position % in box (0=forward/top, 10000=back/bottom)
function axisToYPercent(value) {
    return Math.max(0, Math.min(100, (value / AXIS_MAX) * 100));
}

function updateCyclicXYPoint(element, x, y) {
    element.style.left = axisToXPercent(x) + '%';
    element.style.top = axisToYPercent(y) + '%';
}

function updateCollectiveBar(value) {
    const bar = document.getElementById('collectiveBar');
    const percent = Math.max(0, Math.min(100, (value / AXIS_MAX) * 100));
    bar.style.height = percent + '%';
    document.getElementById('collectiveValue').textContent = value;
}

function updateAPToggleButton(enabled) {
    const btn = document.getElementById('apToggleBtn');
    if (!btn) return;
    btn.textContent = enabled ? 'AP ON' : 'AP OFF';
    btn.classList.toggle('on', enabled);
}

function toggleAP() {
    const btn = document.getElementById('apToggleBtn');
    const currentlyOn = btn.classList.contains('on');
    const newState = !currentlyOn;

    fetch('/api/autopilot', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ enabled: newState })
    })
        .then(response => response.json())
        .then(data => {
            const ap = data.autopilot || {};
            updateAutopilotDisplay(ap);
            updateAPToggleButton(ap.enabled ?? false);
        })
        .catch(err => {
            console.error('AP toggle failed:', err);
        });
}

function toggleHDG() {
    const btn = document.getElementById('apHdgBtn');
    if (!btn) return;
    const currentlyOn = btn.classList.contains('on');
    const newMode = currentlyOn ? 'roll' : 'hdg';

    fetch('/api/autopilot', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ horizontalMode: newMode })
    })
        .then(response => response.json())
        .then(data => {
            updateAutopilotDisplay(data.autopilot || {});
        })
        .catch(err => console.error('HDG toggle failed:', err));
}

function updateHeading(value) {
    fetch('/api/autopilot', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ selectedHeading: parseFloat(value) })
    })
        .then(response => response.json())
        .catch(err => console.error('Heading update failed:', err));
}

function syncHeadingFromSim() {
    const hdg = lastSimData.heading;
    if (hdg === undefined || hdg === null) return;
    const value = Math.round(hdg) % 360;
    document.getElementById('hdgSlider').value = value;
    document.getElementById('hdgInput').value = value;
    updateHeading(value);
}

function syncVSFromSim() {
    const vs = lastSimData.verticalSpeed;
    if (vs === undefined || vs === null) return;
    selectedVS = Math.round(vs);
    document.getElementById('vsInput').value = selectedVS;
    updateVSOnServer();
}

function updateVSOnServer() {
    fetch('/api/autopilot', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ selectedVerticalSpeed: selectedVS })
    })
        .then(response => response.json())
        .then(data => updateAutopilotDisplay(data.autopilot || {}))
        .catch(err => console.error('VS update failed:', err));
}

function syncAltitudeFromSim() {
    const alt = lastSimData.altitude;
    if (alt === undefined || alt === null) return;
    selectedAltitude = Math.round(alt);
    document.getElementById('altInput').value = selectedAltitude;
    updateAltitudeOnServer();
}

function updateAltitudeOnServer() {
    fetch('/api/autopilot', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ selectedAltitude: selectedAltitude })
    })
        .then(response => response.json())
        .then(data => updateAutopilotDisplay(data.autopilot || {}))
        .catch(err => console.error('Altitude update failed:', err));
}

function updateVSDisplay() {
    document.getElementById('vsInput').value = selectedVS;
}

function updateAltitudeDisplay() {
    document.getElementById('altInput').value = selectedAltitude;
}

function adjustAPTarget(axis, delta) {
    // Current targets from state are updated in updateAutopilotDisplay
    const targets = {
        pitch: parseFloat(document.getElementById('apVertical').dataset.value || 0),
        roll: parseFloat(document.getElementById('apLateral').dataset.value || 0)
    };

    if (axis === 'pitch') targets.pitch += delta;
    if (axis === 'roll') targets.roll += delta;

    fetch('/api/autopilot', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            selectedPitch: targets.pitch,
            selectedRoll: targets.roll
        })
    })
        .then(response => response.json())
        .catch(err => console.error('AP target update failed:', err));
}

function updatePID() {
    const kp = parseFloat(document.getElementById('kpInput').value);
    const ki = parseFloat(document.getElementById('kiInput').value);
    const kd = parseFloat(document.getElementById('kdInput').value);
    const rkp = parseFloat(document.getElementById('rollKpInput').value);
    const rki = parseFloat(document.getElementById('rollKiInput').value);
    const rkd = parseFloat(document.getElementById('rollKdInput').value);
    const hkp = parseFloat(document.getElementById('hdgKpInput').value);
    const vskp = parseFloat(document.getElementById('vsKpInput').value);

    fetch('/api/pid', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            pitchKp: kp, pitchKi: ki, pitchKd: kd,
            rollKp: rkp, rollKi: rki, rollKd: rkd,
            headingKp: hkp, vsKp: vskp
        })
    })
        .then(response => response.json())
        .then(data => {
            console.log('PID values updated for both axes');
        })
        .catch(err => {
            console.error('PID update failed:', err);
        });
}

let pidInited = false;
function updateAutopilotDisplay(ap) {
    const lateralEl = document.getElementById('apLateral');
    const centerEl = document.getElementById('apCenter');
    const verticalEl = document.getElementById('apVertical');

    const enabled = ap.enabled ?? false;
    const hMode = ap.horizontalMode ?? 'off';
    const vMode = ap.verticalMode ?? 'off';

    // Update PID inputs once on load
    if (!pidInited && ap.pitchKp !== undefined && ap.rollKp !== undefined) {
        document.getElementById('kpInput').value = ap.pitchKp;
        document.getElementById('kiInput').value = ap.pitchKi;
        document.getElementById('kdInput').value = ap.pitchKd;
        document.getElementById('rollKpInput').value = ap.rollKp;
        document.getElementById('rollKiInput').value = ap.rollKi;
        document.getElementById('rollKdInput').value = ap.rollKd;
        document.getElementById('hdgKpInput').value = ap.headingKp || 1.5;
        document.getElementById('vsKpInput').value = ap.vsKp ?? 0.05;
        pidInited = true;
    }

    // Update HDG and VS mode buttons
    const hdgBtn = document.getElementById('apHdgBtn');
    if (hdgBtn) hdgBtn.classList.toggle('on', hMode === 'hdg');
    const vsBtn = document.getElementById('apVsBtn');
    if (vsBtn) vsBtn.classList.toggle('on', vMode === 'vs');

    // Update HDG input/slider only if not focused
    if (document.activeElement !== document.getElementById('hdgSlider') &&
        document.activeElement !== document.getElementById('hdgInput')) {
        document.getElementById('hdgSlider').value = Math.round(ap.selectedHeading || 0);
        document.getElementById('hdgInput').value = Math.round(ap.selectedHeading || 0);
    }

    // Sync VS/Altitude from server state only if not focused
    if (ap.selectedVerticalSpeed !== undefined && document.activeElement !== document.getElementById('vsInput')) {
        selectedVS = Math.round(ap.selectedVerticalSpeed);
        document.getElementById('vsInput').value = selectedVS;
    }
    if (ap.selectedAltitude !== undefined && document.activeElement !== document.getElementById('altInput')) {
        selectedAltitude = Math.round(ap.selectedAltitude);
        document.getElementById('altInput').value = selectedAltitude;
    }

    const latActiveEl = document.getElementById('apLateralActive');
    const latArmedEl = document.getElementById('apLateralArmed');
    const vertActiveEl = document.getElementById('apVerticalActive');
    const vertArmedEl = document.getElementById('apVerticalArmed');

    // Center: AP when enabled
    centerEl.textContent = enabled ? 'AP' : '--';
    centerEl.classList.toggle('inactive', !enabled);

    // Track current targets for D-Pad adjustments
    verticalEl.dataset.value = ap.selectedPitch || 0;
    lateralEl.dataset.value = ap.selectedRoll || 0;

    // Lateral Display
    let latActiveText = '--';
    let latArmedText = '';
    if (enabled) {
        if (hMode === 'roll') latActiveText = 'ROLL ' + Math.round(ap.selectedRoll) + '°';
        else if (hMode === 'hdg') latActiveText = 'HDG ' + Math.round(ap.selectedHeading ?? 0);
    }
    latActiveEl.textContent = latActiveText;
    latArmedEl.textContent = latArmedText;
    latActiveEl.classList.toggle('inactive', !enabled || hMode === 'off');

    // Vertical Display
    let vertActiveText = '--';
    let vertArmedText = '';

    if (enabled) {
        if (vMode === 'pitch') {
            vertActiveText = 'PITCH ' + Math.round(ap.selectedPitch) + '°';
        } else if (vMode === 'vs') {
            vertActiveText = 'VS ' + Math.round(ap.selectedVerticalSpeed || 0);
        } else if (vMode === 'alts') {
            const activeAlt = ap.capturedAltitude !== undefined && ap.capturedAltitude !== 0 ? ap.capturedAltitude : ap.selectedAltitude;
            vertActiveText = 'ALTS ' + Math.round(activeAlt || 0);
        }

        // Show armed ALT on second line if not active
        if (ap.altHoldArmed && vMode !== 'alts') {
            vertArmedText = 'ALTS ' + Math.round(ap.selectedAltitude || 0);
        }
    }

    vertActiveEl.textContent = vertActiveText;
    vertArmedEl.textContent = vertArmedText;
    vertActiveEl.classList.toggle('inactive', !enabled || vMode === 'off');

    // Update ALTS button state
    const altsBtn = document.getElementById('apAltsBtn');
    if (altsBtn) {
        altsBtn.classList.toggle('on', vMode === 'alts' || ap.altHoldArmed);
        if (ap.altHoldArmed && vMode !== 'alts') {
            altsBtn.style.boxShadow = '0 0 10px #fff'; // White glow for armed
        } else {
            altsBtn.style.boxShadow = '';
        }
    }
}

function connect() {
    const host = window.location.hostname;
    ws = new WebSocket('ws://' + host + ':81');

    ws.onopen = function () {
        document.getElementById('wsStatus').textContent = 'Connected';
        document.getElementById('wsStatus').className = 'status-value status-online';
        document.getElementById('connectionDot').className = 'connection-dot connected';
    };

    ws.onclose = function () {
        document.getElementById('wsStatus').textContent = 'Disconnected';
        document.getElementById('wsStatus').className = 'status-value status-offline';
        document.getElementById('connectionDot').className = 'connection-dot disconnected';
        // Reconnect after 2 seconds
        setTimeout(connect, 2000);
    };

    ws.onerror = function (err) {
        console.error('WebSocket error:', err);
        ws.close();
    };

    ws.onmessage = function (event) {
        try {
            const data = JSON.parse(event.data);

            // Support new state format (sensors + joystick) or legacy (axes, rawX, etc.)
            const sensors = data.sensors || {};
            const joystick = data.joystick || {};
            const axes = data.axes;
            if (axes) {
                // Legacy format: use axes for both sensor and joystick
                sensors.cyclicX = sensors.cyclicX ?? axes[0];
                sensors.cyclicY = sensors.cyclicY ?? axes[1];
                sensors.collective = sensors.collective ?? axes[2];
                joystick.cyclicX = joystick.cyclicX ?? axes[0];
                joystick.cyclicY = joystick.cyclicY ?? axes[1];
                joystick.buttons = joystick.buttons ?? data.buttons ?? 0;
                sensors.rawX = sensors.rawX ?? data.rawX;
                sensors.rawY = sensors.rawY ?? data.rawY;
                sensors.rawZ = sensors.rawZ ?? data.rawZ;
                sensors.cyclicValid = sensors.cyclicValid ?? data.cyclicValid;
            }

            // Cyclic feedback checkbox
            const cyclicFeedbackCheckbox = document.getElementById('cyclicFeedbackCheckbox');
            if (cyclicFeedbackCheckbox && data.cyclicFeedbackEnabled !== undefined) {
                cyclicFeedbackCheckbox.checked = data.cyclicFeedbackEnabled;
            }

            // Autopilot & Simulator display
            const ap = data.autopilot || {};
            const sim = data.simulator || {};
            lastSimData = sim;
            updateAutopilotDisplay(ap);
            updateAPToggleButton(ap.enabled);
            updateSimulatorDisplay(sim, ap);

            // Telemetry recording
            if (isRecording && data.telemetry) {
                const area = document.getElementById('telemetryArea');
                area.value += data.telemetry + "\n";
                // Auto-scroll only if we are already at the bottom
                const isAtBottom = area.scrollHeight - area.clientHeight <= area.scrollTop + 20;
                if (isAtBottom) {
                    area.scrollTop = area.scrollHeight;
                }
            }

            // Cyclic X-Y: two points (sensor = stick position, joystick = values sent to PC)
            const sensorPoint = document.getElementById('sensorPoint');
            const joystickPoint = document.getElementById('joystickPoint');
            updateCyclicXYPoint(sensorPoint, sensors.cyclicX ?? 5000, sensors.cyclicY ?? 5000);
            updateCyclicXYPoint(joystickPoint, joystick.cyclicX ?? 5000, joystick.cyclicY ?? 5000);

            // Collective: sensor reading only (rising bar)
            updateCollectiveBar(sensors.collective ?? 5000);

            // Raw values
            document.getElementById('rawX').textContent = sensors.rawX ?? '--';
            document.getElementById('rawY').textContent = sensors.rawY ?? '--';
            document.getElementById('rawZ').textContent = sensors.rawZ ?? '--';

            // Cyclic status
            const cyclicStatus = document.getElementById('cyclicStatus');
            if (sensors.cyclicValid) {
                cyclicStatus.textContent = 'Receiving';
                cyclicStatus.className = 'status-value status-online';
            } else {
                cyclicStatus.textContent = 'No Data';
                cyclicStatus.className = 'status-value status-offline';
            }

            // Buttons
            const buttons = joystick.buttons ?? 0;
            for (let i = 0; i < 32; i++) {
                const btn = document.getElementById('btn' + i);
                btn.className = (buttons & (1 << i)) ? 'button pressed' : 'button';
            }

            // Update rate
            updateCount++;
            const now = Date.now();
            if (now - lastSecond >= 1000) {
                currentHz = updateCount;
                updateCount = 0;
                lastSecond = now;
                document.getElementById('updateRate').textContent = currentHz + ' Hz';
            }

        } catch (e) {
            console.error('Parse error:', e);
        }
    };
}

// Load and display system logs
function loadLogs() {
    fetch('/logs')
        .then(response => response.json())
        .then(logs => {
            const container = document.getElementById('logsContainer');

            if (logs.length === 0) {
                container.innerHTML = '<div class="log-entry">No logs available</div>';
                return;
            }

            container.innerHTML = '';

            // Display logs in reverse order (newest first)
            logs.reverse().forEach(log => {
                const entry = document.createElement('div');
                const levelClass = log.level.trim().toLowerCase();
                entry.className = `log-entry ${levelClass}`;

                entry.innerHTML = `
                    <span class="log-timestamp">${log.timestamp}</span>
                    <span class="log-level ${levelClass}">[${log.level.trim()}]</span>
                    <span class="log-message">${log.message}</span>
                `;

                container.appendChild(entry);
            });
        })
        .catch(error => {
            console.error('Error loading logs:', error);
            document.getElementById('logsContainer').innerHTML =
                '<div class="log-entry error">Failed to load logs</div>';
        });
}

// Set initial positions (center) before first WebSocket message
updateCyclicXYPoint(document.getElementById('sensorPoint'), 5000, 5000);
updateCyclicXYPoint(document.getElementById('joystickPoint'), 5000, 5000);
updateCollectiveBar(5000);
updateAutopilotDisplay({});
updateSimulatorDisplay({}, {});
updateAPToggleButton(false);
updateVSDisplay();
updateAltitudeDisplay();

// AP toggle button
document.getElementById('apToggleBtn').addEventListener('click', toggleAP);

// PID Apply button
document.getElementById('pidApplyBtn').addEventListener('click', updatePID);

// AP Target Adjustment buttons (D-Pad)
document.getElementById('apPitchUpBtn').addEventListener('click', () => adjustAPTarget('pitch', -1));   // Pitch UP = lower degree (negative)
document.getElementById('apPitchDownBtn').addEventListener('click', () => adjustAPTarget('pitch', 1));   // Pitch DOWN = higher degree (positive)
document.getElementById('apRollLeftBtn').addEventListener('click', () => adjustAPTarget('roll', 1));     // Roll LEFT = positive (?) check sim convention
document.getElementById('apRollRightBtn').addEventListener('click', () => adjustAPTarget('roll', -1));   // Roll RIGHT = negative

// Mode controls
document.getElementById('apHdgBtn').addEventListener('click', toggleHDG);
document.getElementById('apVsBtn').addEventListener('click', toggleVS);
document.getElementById('apAltsBtn').addEventListener('click', toggleALTS);
document.getElementById('hdgInput').addEventListener('click', syncHeadingFromSim);
document.getElementById('vsInput').addEventListener('click', syncVSFromSim);
document.getElementById('altInput').addEventListener('click', syncAltitudeFromSim);

document.getElementById('altInput').addEventListener('change', (e) => {
    selectedAltitude = parseInt(e.target.value);
    updateAltitudeOnServer();
});
document.getElementById('vsInput').addEventListener('change', (e) => {
    selectedVS = parseInt(e.target.value);
    updateVSOnServer();
});

function toggleALTS() {
    const btn = document.getElementById('apAltsBtn');
    if (!btn) return;
    const currentlyOn = btn.classList.contains('on');
    const newState = !currentlyOn;

    fetch('/api/autopilot/alt_arm', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'armed=' + (newState ? 'true' : 'false')
    })
        .then(response => response.json())
        .then(data => {
            // State will be updated via WebSocket broadcast
        })
        .catch(err => console.error('ALTS arm failed:', err));
}

function toggleVS() {
    const btn = document.getElementById('apVsBtn');
    if (!btn) return;
    const currentlyOn = btn.classList.contains('on');
    const newMode = currentlyOn ? 'pitch' : 'vs';

    fetch('/api/autopilot', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ verticalMode: newMode })
    })
        .then(response => response.json())
        .then(data => updateAutopilotDisplay(data.autopilot || {}))
        .catch(err => console.error('VS toggle failed:', err));
}

// VS adjustment buttons - update local state and send to server
document.getElementById('vsMinus100').addEventListener('click', () => {
    selectedVS -= 100;
    updateVSDisplay();
    updateVSOnServer();
});
document.getElementById('vsPlus100').addEventListener('click', () => {
    selectedVS += 100;
    updateVSDisplay();
    updateVSOnServer();
});

// Altitude adjustment buttons
document.getElementById('altMinus1000').addEventListener('click', () => { selectedAltitude -= 1000; updateAltitudeDisplay(); updateAltitudeOnServer(); });
document.getElementById('altMinus100').addEventListener('click', () => { selectedAltitude -= 100; updateAltitudeDisplay(); updateAltitudeOnServer(); });
document.getElementById('altPlus100').addEventListener('click', () => { selectedAltitude += 100; updateAltitudeDisplay(); updateAltitudeOnServer(); });
document.getElementById('altPlus1000').addEventListener('click', () => { selectedAltitude += 1000; updateAltitudeDisplay(); updateAltitudeOnServer(); });

// Heading adjustment
const hdgSlider = document.getElementById('hdgSlider');
const hdgInput = document.getElementById('hdgInput');

hdgSlider.addEventListener('input', (e) => {
    hdgInput.value = e.target.value;
    updateHeading(e.target.value);
});

hdgInput.addEventListener('change', (e) => {
    hdgSlider.value = e.target.value;
    updateHeading(e.target.value);
});

// Cyclic feedback checkbox
document.getElementById('cyclicFeedbackCheckbox').addEventListener('change', function () {
    const enabled = this.checked;
    fetch('/api/cyclic_feedback', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ enabled })
    })
        .then(response => response.json())
        .catch(err => console.error('Cyclic feedback toggle failed:', err));
});

// Start connection and load logs
connect();
loadLogs();

// Map simulator angle to percentage for indicators (range +/- 30 deg)
function angleToPercent(angle, maxRange = 30) {
    let p = (angle + maxRange) / (maxRange * 2) * 100;
    return Math.max(0, Math.min(100, p));
}

function formatAge(ms) {
    if (ms === undefined || ms < 0) return 'never';
    if (ms < 1000) return ms + 'ms';
    const sec = ms / 1000;
    if (sec < 60) return sec.toFixed(1) + 's';
    const min = sec / 60;
    return min.toFixed(1) + 'm';
}

function updateSimulatorDisplay(sim, ap) {
    // Table data
    document.getElementById('simSpeed').textContent = sim.speed !== undefined ? sim.speed.toFixed(1) : '--';
    document.getElementById('simAlt').textContent = sim.altitude !== undefined ? Math.round(sim.altitude) : '--';
    document.getElementById('simHdg').textContent = sim.heading !== undefined ? Math.round(sim.heading) : '--';
    document.getElementById('simVS').textContent = sim.verticalSpeed !== undefined ? Math.round(sim.verticalSpeed) : '--';
    document.getElementById('simPitchText').textContent = sim.pitch !== undefined ? sim.pitch.toFixed(1) : '--';
    document.getElementById('simRollText').textContent = sim.roll !== undefined ? sim.roll.toFixed(1) : '--';
    document.getElementById('simLastUpdate').textContent = formatAge(sim.lastSimDataAgeMs);

    // Indicators
    const rollNeedle = document.getElementById('rollNeedle');
    const pitchNeedle = document.getElementById('pitchNeedle');
    const rollAPTarget = document.getElementById('rollAPTarget');
    const pitchAPTarget = document.getElementById('pitchAPTarget');

    if (sim.roll !== undefined) {
        // Invert: left is positive in sim, we want left on indicator
        rollNeedle.style.left = angleToPercent(-sim.roll, 45) + '%';
    }
    if (sim.pitch !== undefined) {
        // Sim sends pitch up as negative, we want top of the bar for pitch up
        pitchNeedle.style.top = angleToPercent(sim.pitch, 40) + '%';
    }

    if (ap.enabled) {
        const hMode = ap.horizontalMode || 'off';
        const vMode = ap.verticalMode || 'off';

        // Show targets if AP is holding specific angles
        if ((hMode === 'roll' || hMode === 'hdg') && ap.selectedRoll !== undefined) {
            rollAPTarget.style.display = 'block';
            rollAPTarget.style.left = angleToPercent(-ap.selectedRoll, 45) + '%';
        } else {
            rollAPTarget.style.display = 'none';
        }

        if ((vMode === 'pitch' || vMode === 'vs') && ap.selectedPitch !== undefined) {
            pitchAPTarget.style.display = 'block';
            pitchAPTarget.style.top = angleToPercent(ap.selectedPitch, 40) + '%';
        } else {
            pitchAPTarget.style.display = 'none';
        }
    } else {
        rollAPTarget.style.display = 'none';
        pitchAPTarget.style.display = 'none';
    }
}

// Telemetry Controls
document.getElementById('telemetryBtn').addEventListener('click', () => {
    isRecording = !isRecording;
    const btn = document.getElementById('telemetryBtn');
    btn.textContent = isRecording ? 'STOP RECORDING' : 'START RECORDING';
    btn.classList.toggle('recording', isRecording);

    // Tell server to send telemetry field
    fetch('/api/telemetry', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ enabled: isRecording })
    }).catch(err => console.error('Telemetry toggle failed:', err));

    if (isRecording) {
        const area = document.getElementById('telemetryArea');
        if (area.value === "") {
            area.value = "ms,ap,hMode,vMode,pitch,roll,hdg,vs,spd,sel_p,sel_r,sel_hdg,sel_vs,outY,outX\n";
        }
    }
});

document.getElementById('copyTelemetryBtn').addEventListener('click', () => {
    const area = document.getElementById('telemetryArea');
    area.select();
    try {
        document.execCommand('copy');
        alert('CSV data copied to clipboard!');
    } catch (err) {
        console.error('Copy failed:', err);
    }
});

document.getElementById('clearTelemetryBtn').addEventListener('click', () => {
    if (confirm('Clear telemetry data?')) {
        document.getElementById('telemetryArea').value = "";
    }
});
