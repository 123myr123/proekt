import os
import sqlite3

from flask import Flask, jsonify, render_template

def _base_dir():
    return os.path.dirname(os.path.abspath(__file__))

def _default_db_path():
    return os.path.join(_base_dir(), "..", "data", "device.db")

DB_PATH = os.environ.get("IOT_DB_PATH", _default_db_path())

app = Flask(__name__)

def fetch_readings(limit):
    if not os.path.isfile(DB_PATH):
        return []
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    cur = conn.execute(
        "SELECT id, recorded_at, temperature_c, humidity_percent, distance_cm, "
        "button_active, led_on, relay_on FROM sensor_readings ORDER BY id DESC LIMIT ?",
        (limit,),
    )
    rows = [dict(r) for r in cur.fetchall()]
    conn.close()
    return rows

@app.route("/")
def index():
    rows = fetch_readings(100)
    return render_template("index.html", rows=rows)

@app.route("/api/readings")
def api_readings():
    return jsonify(fetch_readings(200))

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", "8080")))
