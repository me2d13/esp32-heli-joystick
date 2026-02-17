let ws;
let updateCount = 0;
let lastSecond = Date.now();
let currentHz = 0;

// Create button grid
const buttonsGrid = document.getElementById('buttonsGrid');
for (let i = 0; i < 32; i++) {
    const btn = document.createElement('div');
    btn.className = 'button';
    btn.id = 'btn' + i;
    btn.textContent = i;
    buttonsGrid.appendChild(btn);
}

function updateAxisBar(barId, value) {
    const bar = document.getElementById(barId);
    // New range: 0-10000, center at 5000
    const center = 5000;
    const offset = value - center;
    const percent = Math.abs(offset) / center * 50;

    bar.className = 'axis-bar';
    if (offset >= 0) {
        bar.classList.add('positive');
        bar.style.width = percent + '%';
        bar.style.marginLeft = '50%';
        bar.style.marginRight = '';
    } else {
        bar.classList.add('negative');
        bar.style.width = percent + '%';
        bar.style.marginLeft = (50 - percent) + '%';
        bar.style.marginRight = '';
    }
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

            // Update axes
            document.getElementById('axisX').textContent = data.axes[0];
            document.getElementById('axisY').textContent = data.axes[1];
            document.getElementById('axisZ').textContent = data.axes[2];

            updateAxisBar('axisXBar', data.axes[0]);
            updateAxisBar('axisYBar', data.axes[1]);
            updateAxisBar('axisZBar', data.axes[2]);

            // Update raw values
            document.getElementById('rawX').textContent = data.rawX;
            document.getElementById('rawY').textContent = data.rawY;
            document.getElementById('rawZ').textContent = data.rawZ;

            // Update cyclic status
            const cyclicStatus = document.getElementById('cyclicStatus');
            if (data.cyclicValid) {
                cyclicStatus.textContent = 'Receiving';
                cyclicStatus.className = 'status-value status-online';
            } else {
                cyclicStatus.textContent = 'No Data';
                cyclicStatus.className = 'status-value status-offline';
            }

            // Update buttons
            for (let i = 0; i < 32; i++) {
                const btn = document.getElementById('btn' + i);
                const pressed = (data.buttons & (1 << i)) !== 0;
                btn.className = pressed ? 'button pressed' : 'button';
            }

            // Calculate update rate
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

// Start connection and load logs
connect();
loadLogs();
