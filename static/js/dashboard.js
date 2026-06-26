const POLL_INTERVAL_MS = 1000;   // how often to fetch sensor data
var lastSensorReadingTimestamp = null;

const temperatureEl = document.getElementById("temperature");
const lastUpdatedEl = document.getElementById("last-updated");
const serverConnectionBadge = document.getElementById("server-connection-status");
const ESPConnectionBadge = document.getElementById("esp-connection-status");
const activePatternEl = document.getElementById("active-pattern");
const patternButtons = document.querySelectorAll(".btn[data-pattern]");


function getLiveLabel(timestamp) {
  if (!timestamp) return new Date().toLocaleTimeString();

  const hours = parseInt(timeframeSelector.value);
  // timestamp is 'YYYY-MM-DD HH:MM:SS'
  const [datePart, timePart] = timestamp.split(' ');
  const [h, m] = timePart.split(':').map(Number);
  const dayNames = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];

  if (hours <= 1) {
    // 1-minute buckets -> HH:MM as-is
    return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}`;
  } else if (hours <= 24) {
    // 15-minute buckets -> round down to nearest 15
    const bucketM = Math.floor(m / 15) * 15;
    return `${String(h).padStart(2, '0')}:${String(bucketM).padStart(2, '0')}`;
  } else {
    // 6-hour buckets -> round down to nearest 6 hours
    const bucketH = Math.floor(h / 6) * 6;
    const date = new Date(timestamp);
    const dayName = dayNames[date.getDay()];
    return `${dayName} ${String(bucketH).padStart(2, '0')}:00`;
  }
}

// SENSOR POLLING
async function fetchSensorData() {
  try {
    const response = await fetch("/api/data");
    if (!response.ok) throw new Error(`HTTP ${response.status}`);

    const data = await response.json();

    // Update temperature display
    temperatureEl.textContent =
      data.temperature !== null ? data.temperature.toFixed(1) : "--";

    if(data.temperature !== null){
      setESPConnectionStatus("online");
    }
    else{
      setESPConnectionStatus("offline");
    }

    // Update timestamp
    lastUpdatedEl.textContent = new Date().toLocaleString('en-GB');

    // push to chart
    if(data.temperature !== null && tempChart){

      if (timeframeSelector.value === 'live') {
        const label = data.timestamp ? data.timestamp.slice(11, 16) : new Date().toLocaleTimeString();
        tempChart.data.labels.push(label);
        tempChart.data.datasets[0].data.push(data.temperature);
        tempChart.update();
      }
      else{
        const label = getLiveLabel(data.timestamp);
        const labels = tempChart.data.labels;
        const dataset = tempChart.data.datasets[0].data;

        if (labels.length > 0 && labels[labels.length - 1] === label) {
          // Same bucket -> overwrite the last point
          dataset[dataset.length - 1] = data.temperature;
        } else {
          // New bucket -> push and shift to keep the window fixed
          labels.push(label);
          dataset.push(data.temperature);
          labels.shift();
          dataset.shift();
        }

        tempChart.update();
      }
    }

    // if we are able to fetch anything from the server, its online.
    setServerConnectionStatus("online");

  } catch (err) {
    console.error("Failed to fetch sensor data:", err);
    setServerConnectionStatus("offline");
    setESPConnectionStatus("offline");
  }
}

function setServerConnectionStatus(state) {
  const labels = { online: "Connected to Server", offline: "Disconnected from Server"};
  const classes = { online: "badge--online", offline: "badge--offline"};

  serverConnectionBadge.textContent = labels[state] ?? state;
  serverConnectionBadge.className = `badge ${classes[state] ?? "badge--unknown"}`;
}

function setESPConnectionStatus(state){
  const labels = { online: "ESP Online", offline: "ESP Offline" };
  const classes = { online: "badge--online", offline: "badge--offline"};

  ESPConnectionBadge.textContent = labels[state] ?? state;
  ESPConnectionBadge.className = `badge ${classes[state] ?? "badge--unknown"}`;
}

// Start polling immediately, then repeat
fetchSensorData();
setInterval(fetchSensorData, POLL_INTERVAL_MS);

// LED PATTERN CONTROL
patternButtons.forEach((btn) => {
  btn.addEventListener("click", () => {
    const pattern = btn.dataset.pattern;

    sendLedCommand(pattern);

    patternButtons.forEach((b) => b.classList.remove("active"));
    btn.classList.add("active");

    activePatternEl.textContent = pattern;
  });
});

async function sendLedCommand(pattern) {
  try {
    const response = await fetch("/api/led", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ pattern }),
    });

    if (!response.ok) {
      const err = await response.json();
      console.error("LED command rejected:", err);
    }
  } catch (err) {
    console.error("Failed to send LED command:", err);
  }
}

// TEMPERATURE CHART SETUP
const ctx = document.getElementById('tempChart').getContext('2d');
const timeframeSelector = document.getElementById('timeframe-selector');
let tempChart;

function initChart() {
  tempChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: [], // timestamps
      datasets: [{
        label: 'Temperature (°C)',
        data: [], // temp vals
        borderColor: '#4f8ef7', 
        backgroundColor: 'rgba(79, 142, 247, 0.1)',
        borderWidth: 2,
        fill: true,
        tension: 0.3, // line curviness
        pointRadius: 0,
        spanGaps: false
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        x: {
          ticks: { color: '#8892a4' },
          grid: { color: '#2e3250' }
        },
        y: {
          ticks: { color: '#8892a4' },
          grid: { color: '#2e3250' }
        }
      },
      plugins: {
        legend: { display: false }
      }
    }
  });
}

async function fetchHistoricalData(hours) {
  try {
    const response = await fetch(`/api/history?hours=${hours}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const data = await response.json();

    tempChart.data.labels = data.map(r => r.timestamp);
    tempChart.data.datasets[0].data = data.map(r => r.temperature);
    tempChart.update('none'); // skip transition animation on full reload
  } catch (err) {
    console.error("Failed to fetch historical data:", err);
  }
}
// Listen for timeframe changes
timeframeSelector.addEventListener('change', (e) => {
  if (e.target.value === 'live') {
    // Clear the chart and let live pushes fill it from scratch
    tempChart.data.labels = [];
    tempChart.data.datasets[0].data = [];
    tempChart.update('none');
  } else {
    fetchHistoricalData(e.target.value);
  }
});

initChart();
fetchHistoricalData(1);