import random
import requests

class DigitalLoggersClient:
    def __init__(self, user="admin", password="1234", demo_mode=False):
        self.user = user
        self.password = password
        self.demo_mode = demo_mode
        self.auth = (user, password)

    def get_status(self, ip):
        if self.demo_mode:
            return {
                "state": random.choice(["on", "off"]),
                "amps": round(random.uniform(0.5, 4.5), 2),
                "online": True
            }

        try:
            # Polling outlets for state
            r = requests.get(f"http://{ip}/restapi/relay/outlets/", auth=self.auth, timeout=2)
            if r.status_code != 200: return {"online": False}

            data = r.json()
            any_on = any(o.get("state") for o in data)
            any_off = any(not o.get("state") for o in data)
            state = "mixed" if any_on and any_off else ("on" if any_on else "off")

            # Polling current
            amps = 0.0
            r_amps = requests.get(f"http://{ip}/restapi/relay/amps/", auth=self.auth, timeout=2)
            if r_amps.status_code == 200:
                amps = float(r_amps.text)

            return {"state": state, "amps": amps, "online": True}
        except Exception:
            return {"online": False}

    def send_command(self, ip, command):
        # command is "ON" or "OFF"
        if self.demo_mode:
            print(f"[DEMO] Sending {command} to {ip}")
            return True

        try:
            r = requests.get(f"http://{ip}/outlet?a={command.upper()}", auth=self.auth, timeout=5)
            return r.status_code == 200
        except Exception:
            return False
