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
dli = DigitalLoggersClient()
state_manager = StateManager(settings, dli)
aggregator = StatusAggregator(dli, settings, state_manager)
sequence_engine = SequenceEngine(dli, settings, state_manager)

state_manager.set_aggregator(aggregator)
aggregator.start()

@app.route("/")
def index():
    return send_from_directory("web", "control-dashboard.html")

@app.route("/status")
def status():
    return jsonify(state_manager.get_json())

@app.route("/log")
def log():
    # The firmware serves its diagnostic ring buffer here.
    return "Simulator: no diagnostic log available.", 200, {"Content-Type": "text/plain"}

@app.route("/on")
def sequence_on():
    sequence_engine.trigger_sequence("ON")
    return jsonify({"success": True})

@app.route("/off")
def sequence_off():
    sequence_engine.trigger_sequence("OFF")
    return jsonify({"success": True})

@app.route("/rack/<int:index>/<cmd>")
def rack_control(index, cmd):
    racks = settings.get_racks()
    if 0 <= index < len(racks):
        success = dli.send_command(racks[index]["ip"], cmd.upper())
        state_manager.refresh_all()
        return jsonify({"success": success})
    return jsonify({"success": False}), 404

@app.route("/rack/<int:index>/outlet/<int:oid>/rename", methods=["POST"])
def rename_outlet(index, oid):
    racks = settings.get_racks()
    name = request.json.get("name")
    if 0 <= index < len(racks) and name:
        success = dli.set_outlet_name(racks[index]["ip"], oid, name)
        state_manager.refresh_all()
        return jsonify({"success": success})
    return jsonify({"success": False}), 400

@app.route("/save_config", methods=["POST"])
def save_config():
    new_racks = request.json
    settings.save({"racks": new_racks})
    state_manager.refresh_all()
    return "OK"

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8003)
