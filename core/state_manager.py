from .controller_state import GlobalState, SystemState
import threading

class StateManager:
    def __init__(self, settings_store, dli_client):
        self.state = GlobalState()
        self.settings = settings_store
        self.dli = dli_client
        self._lock = threading.Lock()
        self._aggregator = None
        self._sequencing = False

    def set_aggregator(self, aggregator):
        self._aggregator = aggregator

    def is_sequencing(self):
        return self._sequencing

    def set_sequencing(self, val):
        with self._lock:
            self._sequencing = val
            if val:
                self.state.power = SystemState.SEQUENCING

    def refresh_all(self):
        if not self._aggregator: return

        racks, total = self._aggregator.poll_all()

        with self._lock:
            self.state.racks = racks
            self.state.total_current = total

            if not self._sequencing:
                self.state.power = self._calculate_system_state(racks)

    def _calculate_system_state(self, racks):
        if not racks: return SystemState.OFF

        any_error = any(not r.online for r in racks)
        if any_error: return SystemState.ERROR

        states = [r.state for r in racks]
        if all(s == "on" for s in states): return SystemState.ON
        if all(s == "off" for s in states): return SystemState.OFF
        return SystemState.MIXED

    def get_json(self):
        with self._lock:
            return {
                "power": self.state.power.value,
                "total_current": self.state.total_current,
                "strips": [
                    {
                        "name": r.name,
                        "ip": r.ip,
                        "state": r.state,
                        "current": r.current,
                        "current_available": r.current_available
                    } for r in self.state.racks
                ]
            }
