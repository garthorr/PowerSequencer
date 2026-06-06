import requests

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
            amps = 0.0
            found_amps = False
            endpoints = ["/restapi/relay/amps/", "/restapi/relay/current/", "/amps"]

            cached = self.working_endpoints.get(ip)
            if cached:
                try:
                    r_amps = requests.get(f"http://{ip}{cached}", auth=self.auth, timeout=1)
                    if r_amps.status_code == 200:
                        amps = float(r_amps.text)
                        found_amps = True
                except Exception: pass

            if not found_amps:
                for ep in endpoints:
                    try:
                        r_amps = requests.get(f"http://{ip}{ep}", auth=self.auth, timeout=1)
                        if r_amps.status_code == 200:
                            amps = float(r_amps.text)
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
            # DLI REST API typically uses PUT for setting values.
            # Index is 0-based in our system, but DLI might be 0-based in restapi too.
            url = f"http://{ip}/restapi/relay/outlets/{index}/name/"
            # Some DLI versions require X-CSRF header for PUT
            r = requests.put(url, auth=self.auth, data={"value": name}, headers={"X-CSRF": "x"}, timeout=5)
            return r.status_code == 200 or r.status_code == 204
        except Exception:
            return False
