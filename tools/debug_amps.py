import requests

def debug(ip):
    auth = ("admin", "1234")
    endpoints = [
        "/restapi/relay/amps/",
        "/restapi/relay/current/",
        "/restapi/relay/status/",
        "/restapi/status/",
        "/amps",
        "/status.xml"
    ]
    for ep in endpoints:
        try:
            r = requests.get(f"http://{ip}{ep}", auth=auth, timeout=2)
            print(f"EP: {ep} | Code: {r.status_code} | Content: {r.text[:100]}")
        except:
            print(f"EP: {ep} | FAILED")

# This is for the user to run if they have access to the real hardware
# Since I am in a sandbox, I will just refactor the client to be more aggressive
