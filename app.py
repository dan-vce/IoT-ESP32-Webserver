"""
Flask web server for ESP32 IoT dashboard.

Receives sensor data from the ESP32 via POST /sensor,
serves live data to the browser via GET /api/data,
and relays LED pattern commands from the browser to the ESP32.
"""

from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
import sqlite3
import threading
from datetime import datetime, timedelta
from apscheduler.schedulers.background import BackgroundScheduler

app = Flask(__name__)
CORS(app)
DB_FILE = "sensor_data.db"

# In-memory state (sufficient for a single-device assignment)
latest_sensor_data = {
    "temperature": None,
    "timestamp": None,
}

# Pending LED command for the ESP32 to pick up.
# Reset to None after the ESP32 acknowledges it.
pending_led_command = {
    "pattern": None,
}

# ESP TIMEOUT
esp_timeout_timer = None
ESP_TIMEOUT_SECONDS = 6

def on_esp_timeout():
    """Called when no sensor reading has been received for ESP_TIMEOUT_SECONDS."""
    latest_sensor_data["temperature"] = None
    latest_sensor_data["timestamp"] = None

def reset_esp_timer():
    """Cancel any existing timer and start a fresh one."""
    global esp_timeout_timer
    if esp_timeout_timer is not None:
        esp_timeout_timer.cancel()
    esp_timeout_timer = threading.Timer(ESP_TIMEOUT_SECONDS, on_esp_timeout)
    esp_timeout_timer.daemon = True  # don't block server shutdown
    esp_timeout_timer.start()


# DATABSE SETUP
def init_db():
    """Creates the database and table if they don't exist."""
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME,
            temperature REAL
        )
    ''')
    conn.commit()
    conn.close()

# ESP32-FACING ENDPOINTS
@app.route("/sensor", methods=["POST"])
def receive_sensor():
    """ESP32 POSTs JSON sensor readings here every 2-5 seconds."""
    data = request.get_json(silent=True)
    if not data:
        return jsonify({"error": "invalid JSON"}), 400

    latest_sensor_data["temperature"] = data.get("temperature")
    latest_sensor_data["timestamp"] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    reset_esp_timer()

    # save data to db
    if latest_sensor_data["temperature"] is not None:
        conn = sqlite3.connect(DB_FILE)
        cursor = conn.cursor()
        cursor.execute("INSERT INTO readings (timestamp, temperature) VALUES (?, ?)", (latest_sensor_data["timestamp"], latest_sensor_data["temperature"]))
        conn.commit()
        conn.close()

    # Return any pending LED command so the ESP32 can act on it immediately.
    response = {"status": "ok"}
    if pending_led_command["pattern"] is not None:
        response["command"] = {
            "pattern": pending_led_command["pattern"],
        }
        # Clear after delivery
        pending_led_command["pattern"] = None

    return jsonify(response), 200


@app.route("/api/led/command", methods=["GET"])
def get_led_command():
    """
    Optional polling endpoint for the ESP32.
    Returns the pending LED command and clears it.
    """
    if pending_led_command["pattern"] is None:
        return jsonify({"command": None}), 200

    response = {
        "command": {
            "pattern": pending_led_command["pattern"],
        }
    }
    pending_led_command["pattern"] = None
    return jsonify(response), 200


# BROWSER FACING ENDPOINTS
@app.route("/")
def index():
    """Serve the main dashboard page."""
    return render_template("index.html")


@app.route("/api/data", methods=["GET"])
def get_sensor_data():
    """Return the latest sensor reading as JSON for AJAX polling."""
    return jsonify(latest_sensor_data), 200


@app.route("/api/led", methods=["POST"])
def set_led_pattern():
    """Browser POSTs the desired LED pattern here."""
    data = request.get_json(silent=True)
    if not data or "pattern" not in data:
        return jsonify({"error": "pattern field required"}), 400

    valid_patterns = {"chase", "blink", "alternating", "flicker", "twinkle"}
    pattern = data["pattern"]
    if pattern not in valid_patterns:
        return jsonify({"error": f"unknown pattern '{pattern}'"}), 400

    pending_led_command["pattern"] = pattern

    return jsonify({"status": "queued", "pattern": pattern}), 200

@app.route('/api/history', methods=['GET'])
def get_historical_data():
    hours_back = request.args.get('hours', default=1, type=int)
    now = datetime.now()
    cutoff_time = now - timedelta(hours=hours_back)

    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    cursor.execute(
        "SELECT timestamp, temperature FROM readings WHERE timestamp >= ? ORDER BY timestamp ASC",
        (cutoff_time.strftime('%Y-%m-%d %H:%M:%S'),)
    )
    rows = cursor.fetchall()
    conn.close()

    if hours_back <= 1:  # 1 minute intervals
        total_buckets = hours_back * 60
        def bucket_label(dt): return dt.strftime('%H:%M')
        def bucket_iter():
            for i in range(total_buckets + 1):
                yield cutoff_time + timedelta(minutes=i)
    elif hours_back <= 24: # 15 minute intervals
        total_buckets = hours_back * 4
        def bucket_label(dt): return dt.strftime('%H:%M')
        def bucket_iter():
            for i in range(total_buckets + 1):
                yield cutoff_time + timedelta(minutes=i * 15)
    else:  # 6 hour intervals
        total_buckets = (hours_back // 6)
        def bucket_label(dt): return dt.strftime('%a %H:%M')
        def bucket_iter():
            for i in range(total_buckets + 1):
                yield cutoff_time + timedelta(hours=i * 6)

    # map actual readings to their nearest bucket label
    reading_map = {}
    for row in rows:
        try:
            dt = datetime.strptime(row[0], '%Y-%m-%d %H:%M:%S')
        except ValueError:
            continue
        label = bucket_label(dt)
        # Last write wins (most recent reading in that bucket)
        reading_map[label] = row[1]

    # Build full skeleton, null where no reading exists
    history = []
    for bucket_dt in bucket_iter():
        label = bucket_label(bucket_dt)
        history.append({
            "timestamp": label,
            "temperature": reading_map.get(label, None)
        })

    return jsonify(history)

# DATABASE CLEANUP
def cleanup_old_data():
    """Delete readings older than 7 days."""
    cutoff = (datetime.now() - timedelta(days=7)).strftime('%Y-%m-%d %H:%M:%S')
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    cursor.execute("DELETE FROM readings WHERE timestamp < ?", (cutoff,))
    conn.commit()
    conn.close()

if __name__ == "__main__":
    init_db()
    cleanup_old_data()  # clean on startup

    scheduler = BackgroundScheduler()
    scheduler.add_job(cleanup_old_data, 'interval', hours=1)
    scheduler.start()
    app.run(host="0.0.0.0", port=5000, debug=True)
