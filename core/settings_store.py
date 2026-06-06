import json
import os

class SettingsStore:
    def __init__(self, filepath="settings.json"):
        self.filepath = filepath
        self.settings = self.load()

    def load(self):
        if os.path.exists(self.filepath):
            with open(self.filepath, 'r') as f:
                return json.load(f)
        return {
            "racks": [
                {"name": "Rack 1", "ip": "192.168.0.101"},
                {"name": "Rack 2", "ip": "192.168.0.102"}
            ],
            "sequence_delay_ms": 1500,
            "poll_interval_ms": 5000,
            "demo_mode": True
        }

    def save(self, new_settings):
        self.settings.update(new_settings)
        with open(self.filepath, 'w') as f:
            json.dump(self.settings, f, indent=2)

    def get_racks(self):
        return self.settings.get("racks", [])
