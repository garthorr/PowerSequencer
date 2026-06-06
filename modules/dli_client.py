import requests
import json

class DigitalLoggersClient:
    def __init__(self, user="admin", password="1234"):
        self.user = user
        self.password = password
        self.auth = (user, password)
        self.working_endpoints = {}

    def get_status(self, ip):
        status = {"online": False, "outlets": [], "state": "unknown", "amps": 0.0}
        try:
            # 1. Fetch Outlet States
            r = requests.get(f"http://{ip}/restapi/relay/outlets/", auth=self.auth, timeout=2)
            if r.status_code == 200:
                data = r.json()
                status["online"] = True
                status["outlets"] = [{"name": o.get("name", f"Outlet {i+1}"), "state": o.get("state", False)} for i, o in enumerate(data)]
                any_on = any(o["state"] for o in status["outlets"])
                any_off = any(not o["state"] for o in status["outlets"])
                status["state"] = "mixed" if any_on and any_off else ("on" if any_on else "off")
            else:
                return status

            # 2. Fetch Amperage
            # We will try multiple keys if it's JSON, or direct text if not
            amps = 0.0
            found_amps = False
            endpoints = ["/restapi/relay/amps/", "/restapi/relay/current/", "/restapi/relay/status/", "/amps"]

            # Helper to extract from various formats
            def extract(text):
                try:
                    # Case 1: Plain number
                    return float(text.strip())
                except ValueError:
                    try:
                        # Case 2: JSON object
                        data = json.loads(text)
                        if isinstance(data, dict):
                            # Try common keys
                            for k in ["value", "amps", "current", "reading"]:
                                if k in data: return float(data[k])
                        elif isinstance(data, list):
                            # Some APIs return an array of values
                            return float(data[0])
                    except: pass
                return None

            cached = self.working_endpoints.get(ip)
            if cached:
                try:
                    r_amps = requests.get(f"http://{ip}{cached}", auth=self.auth, timeout=1)
                    val = extract(r_amps.text)
                    if val is not None:
                        amps = val
                        found_amps = True
                except Exception: pass

            if not found_amps:
                for ep in endpoints:
                    try:
                        r_amps = requests.get(f"http://{ip}{ep}", auth=self.auth, timeout=1)
                        if r_amps.status_code == 200:
                            val = extract(r_amps.text)
                            if val is not None:
                                amps = val
                                self.working_endpoints[ip] = ep
                                found_amps = True
                                break
                    except Exception: continue

            status["amps"] = amps
            return status
        except Exception:
            return status

    def send_command(self, ip, command):
        try:
            r = requests.get(f"http://{ip}/outlet?a={command.upper()}", auth=self.auth, timeout=5)
            return r.status_code == 200
        except Exception:
            return False

    def set_outlet_name(self, ip, index, name):
        try:
            url = f"http://{ip}/restapi/relay/outlets/{index}/name/"
            r = requests.put(url, auth=self.auth, data={"value": name}, headers={"X-CSRF": "x"}, timeout=5)
            return r.status_code in [200, 204]
        except Exception:
            return False
