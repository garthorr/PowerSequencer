import time
import threading

class SequenceEngine:
    def __init__(self, dli_client, settings_store, state_manager):
        self.dli = dli_client
        self.settings = settings_store
        self.manager = state_manager
        self._lock = threading.Lock()

    def trigger_sequence(self, command):
        thread = threading.Thread(target=self._run_sequence, args=(command,))
        thread.start()

    def _run_sequence(self, command):
        with self._lock:
            self.manager.set_sequencing(True)
            racks = self.settings.get_racks()
            delay = self.settings.settings.get("sequence_delay_ms", 1500) / 1000.0

            for i, rack in enumerate(racks):
                print(f"Sequencing {rack['name']} -> {command}")
                self.dli.send_command(rack["ip"], command)
                if i < len(racks) - 1:
                    time.sleep(delay)

            self.manager.set_sequencing(False)
            self.manager.refresh_all()
