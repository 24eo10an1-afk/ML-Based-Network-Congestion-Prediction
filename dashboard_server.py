from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import subprocess
import json
import os
import re

BASE_DIR = os.path.expanduser("~/ns-3.48")


class DashboardServer(BaseHTTPRequestHandler):

    def do_GET(self):
        parsed = urlparse(self.path)

        # Open dashboard
        if parsed.path == "/":
            try:
                with open(os.path.join(BASE_DIR, "dashboard.html"), "rb") as f:
                    data = f.read()

                self.send_response(200)
                self.send_header("Content-Type", "text/html")
                self.send_header("Content-Length", str(len(data)))
                self.end_headers()
                self.wfile.write(data)

            except Exception as e:
                self.send_error(500, str(e))

            return

        # Run NS-3 simulation
        if parsed.path == "/run":
            params = parse_qs(parsed.query)

            try:
                rate = float(params.get("rate", ["5"])[0])

                if rate < 1 or rate > 8:
                    raise ValueError("Traffic rate must be between 1 and 8 Mbps")

                command = [
                    "./ns3",
                    "run",
                    f"multipath --rate={rate}"
                ]

                result = subprocess.run(
                    command,
                    cwd=BASE_DIR,
                    capture_output=True,
                    text=True,
                    timeout=120
                )

                output = result.stdout + "\n" + result.stderr

                # Extract final NS-3 metrics
                def get_value(pattern, default=0):
                    match = re.search(pattern, output)
                    if match:
                        return float(match.group(1))
                    return default

                packets_sent = get_value(
                    r"Packets Sent\s*:\s*([0-9.]+)"
                )

                packets_recv = get_value(
                    r"Packets Recv\s*:\s*([0-9.]+)"
                )

                packets_lost = get_value(
                    r"Packets Lost\s*:\s*([0-9.]+)"
                )

                delay = get_value(
                    r"Average Delay\s*:\s*([0-9.eE+-]+)"
                )

                jitter = get_value(
                    r"Average Jitter\s*:\s*([0-9.eE+-]+)"
                )

                throughput = get_value(
                    r"Throughput\s*:\s*([0-9.eE+-]+)"
                )

                queue = get_value(
                    r"Maximum Queue\s*:\s*([0-9.]+)"
                )

                # Read ML decision
                decision = "UNKNOWN"
                action = "UNKNOWN"

                ml_file = os.path.join(BASE_DIR, "ml_decision.txt")

                if os.path.exists(ml_file):
                    with open(ml_file, "r") as f:
                        ml_text = f.read()

                    if "CONGESTION" in ml_text:
                        decision = "CONGESTION"

                    if "NO CONGESTION" in ml_text:
                        decision = "NO_CONGESTION"

                    if "REROUTE TO PATH B" in ml_text:
                        action = "REROUTE TO PATH B"

                    elif "KEEP CURRENT PATH" in ml_text:
                        action = "KEEP PATH A"

                data = {
                    "success": result.returncode == 0,
                    "trafficRate": rate,
                    "packetsSent": packets_sent,
                    "packetsReceived": packets_recv,
                    "packetsLost": packets_lost,
                    "delay": delay,
                    "jitter": jitter,
                    "throughput": throughput,
                    "queue": queue,
                    "decision": decision,
                    "action": action
                }

                response = json.dumps(data).encode()

                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.send_header("Content-Length", str(len(response)))
                self.end_headers()
                self.wfile.write(response)

            except Exception as e:
                error = json.dumps({
                    "success": False,
                    "error": str(e)
                }).encode()

                self.send_response(500)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(error)))
                self.end_headers()
                self.wfile.write(error)

            return

        # Serve dashboard image files
        if parsed.path.lower().endswith((".png", ".jpg", ".jpeg")):
            filename = os.path.basename(parsed.path)
            filepath = os.path.join(BASE_DIR, filename)

            if os.path.isfile(filepath):
                with open(filepath, "rb") as f:
                    data = f.read()

                if filename.lower().endswith(".png"):
                    content_type = "image/png"
                else:
                    content_type = "image/jpeg"

                self.send_response(200)
                self.send_header("Content-Type", content_type)
                self.send_header("Content-Length", str(len(data)))
                self.end_headers()
                self.wfile.write(data)
                return

        self.send_error(404)


print("======================================")
print(" NS-3 LIVE DASHBOARD SERVER")
print("======================================")
print("Dashboard: http://localhost:8000")
print("Press Ctrl+C to stop")
print("======================================")

server = HTTPServer(("0.0.0.0", 8000), DashboardServer)
server.serve_forever()
