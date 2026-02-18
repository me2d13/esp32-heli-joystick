let ws;
let updateCount = 0;
let lastSecond = Date.now();
let currentHz = 0;

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

// Map axis value (0-10000) to Y position % in box (0=back/bottom, 10000=fwd/top)
function axisToYPercent(value) {
    return Math.max(0, Math.min(100, ((AXIS_MAX - value) / AXIS_MAX) * 100));
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

function connect() {
    const host = window.location.hostname;
    ws = new WebSocket('ws://' + host + ':81');

    ws.onopen = function() {
        document.getElementById('wsStatus').textContent = 'Connected';
        document.getElementById('wsStatus').className = 'status-value status-online';
        document.getElementById('connectionDot').className = 'connection-dot connected';
    };

    ws.onclose = function() {
        document.getElementById('wsStatus').textContent = 'Disconnected';
        document.getElementById('wsStatus').className = 'status-value status-offline';
        document.getElementById('connectionDot').className = 'connection-dot disconnected';
        // Reconnect after 2 seconds
        setTimeout(connect, 2000);
    };

    ws.onerror = function(err) {
        console.error('WebSocket error:', err);
        ws.close();
    };

    ws.onmessage = function(event) {
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

// Start connection and load logs
connect();
loadLogs();
