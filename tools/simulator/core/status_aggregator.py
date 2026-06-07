import time
import threading
from .controller_state import RackStatus, OutletStatus, SystemState

class StatusAggregator:
    def __init__(self, dli_client, settings_store, state_manager):
        self.dli = dli_client
        self.settings = settings_store
        self.manager = state_manager
        self._stop_event = threading.Event()

    def start(self):
        self._thread = threading.Thread(target=self._poll_loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._stop_event.set()

    def _poll_loop(self):
        while not self._stop_event.is_set():
            if not self.manager.is_sequencing():
                self.manager.refresh_all()
            time.sleep(self.settings.settings.get("poll_interval_ms", 5000) / 1000.0)

    def poll_all(self):
        racks = self.settings.get_racks()
        results = []
        total_amps = 0.0

        for r in racks:
            status = self.dli.get_status(r["ip"])
            outlets = [OutletStatus(name=o["name"], state=o["state"]) for o in status.get("outlets", [])]

            rack_stat = RackStatus(
                name=r["name"],
                ip=r["ip"],
                state=status.get("state", "unknown"),
                current=status.get("amps", 0.0),
                current_available=status.get("online", False),
                online=status.get("online", False),
                outlets=outlets
            )
            results.append(rack_stat)
            if status.get("online"):
                total_amps += status.get("amps", 0.0)

        return results, total_amps
