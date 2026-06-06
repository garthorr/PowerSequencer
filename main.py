from flask import Flask, jsonify, request, send_from_directory
from core.settings_store import SettingsStore
from core.state_manager import StateManager
from core.sequence_engine import SequenceEngine
from core.status_aggregator import StatusAggregator
from modules.dli_client import DigitalLoggersClient
import time

app = Flask(__name__)

# Initialize Architecture
settings = SettingsStore()
dli = DigitalLoggersClient(demo_mode=settings.settings.get("demo_mode", True))
state_manager = StateManager(settings, dli)
aggregator = StatusAggregator(dli, settings, state_manager)
sequence_engine = SequenceEngine(dli, settings, state_manager)

state_manager.set_aggregator(aggregator)
aggregator.start()

@app.route("/")
def index():
    return send_from_directory(".", "control-dashboard.html")

@app.route("/status")
def status():
    return jsonify(state_manager.get_json())

@app.route("/on")
def sequence_on():
    sequence_engine.trigger_sequence("ON")
    return jsonify({"success": True})

@app.route("/off")
def sequence_off():
    sequence_engine.trigger_sequence("OFF")
    return jsonify({"success": True})

@app.route("/save_config", methods=["POST"])
def save_config():
    new_racks = request.json
    settings.save({"racks": new_racks})
    # In a real ESP32 we might reboot, here we just refresh
    state_manager.refresh_all()
    return "OK"

if __name__ == "__main__":
    # Small delay to let aggregator get first poll
    time.sleep(1)
    app.run(host="0.0.0.0", port=8003)
