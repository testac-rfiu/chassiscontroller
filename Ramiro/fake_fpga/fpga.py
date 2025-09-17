# fake_fpga.py — Tiny SCPI server to simulate a device with 2 switches × 4 paths
# Commands:
#   SWITCH:SELECT <sw> <p>   (sw ∈ {1,2}, p ∈ {1..4})   -> OK
#   PATH:SELECT n            (n ∈ {1..8})              -> OK  (back-compat)
#   STATE?                   -> {"switch": S, "path": P, "busy": false, "switchReadback": "0x...."}
#
# Usage: python fake_fpga.py [port]

import socket, threading, sys, time, re

HOST = "0.0.0.0"
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 5025

state = { "switch": 1, "path": 0, "busy": False }

# Unique demo readback codes per (switch, path)
BITS = {
    (1,1): "0x1101", (1,2): "0x1102", (1,3): "0x1103", (1,4): "0x1104",
    (2,1): "0x2101", (2,2): "0x2102", (2,3): "0x2103", (2,4): "0x2104",
}

def id_to_pair(n):
    if 1 <= n <= 4:   return (1, n)
    if 5 <= n <= 8:   return (2, n-4)
    return None

def handle(conn, addr):
    with conn:
        conn.settimeout(5)
        buffer = b""
        while True:
            try:
                chunk = conn.recv(1024)
                if not chunk: break
                buffer += chunk
                while b"\n" in buffer:
                    line, buffer = buffer.split(b"\n", 1)
                    cmd = line.decode("utf-8", errors="ignore").strip()
                    if not cmd: continue
                    print(f"[SCPI] {addr[0]}: {cmd}")
                    upper = cmd.upper()

                    # NEW: SWITCH:SELECT <sw> <p>  (also accepts comma)
                    if upper.startswith("SWITCH:SELECT"):
                        nums = re.findall(r"[-]?\d+", cmd)
                        if len(nums) == 2:
                            sw, p = int(nums[0]), int(nums[1])
                            if sw in (1,2) and 1 <= p <= 4:
                                state["busy"] = True; time.sleep(0.15)
                                state["switch"], state["path"] = sw, p
                                state["busy"] = False
                                conn.sendall(b"OK\n")
                            else:
                                conn.sendall(b"ERR,INVALID_ARGS\n")
                        else:
                            conn.sendall(b"ERR,BAD_SYNTAX\n")

                    elif upper.startswith("PATH:SELECT"):
                        parts = cmd.split()
                        if len(parts) == 2 and parts[1].isdigit():
                            n = int(parts[1])
                            pair = id_to_pair(n)
                            if pair:
                                sw, p = pair
                                state["busy"] = True; time.sleep(0.15)
                                state["switch"], state["path"] = sw, p
                                state["busy"] = False
                                conn.sendall(b"OK\n")
                            else:
                                conn.sendall(b"ERR,INVALID_PATH\n")
                        else:
                            conn.sendall(b"ERR,BAD_SYNTAX\n")

                    elif upper == "STATE?":
                        sw, p = state["switch"], state["path"]
                        bits = BITS.get((sw, p), "0x0000")
                        busy = "true" if state["busy"] else "false"
                        resp = f'{{"switch": {sw}, "path": {p}, "busy": {busy}, "switchReadback": "{bits}"}}\n'
                        conn.sendall(resp.encode("utf-8"))

                    else:
                        conn.sendall(b"ERR,UNKNOWN\n")
            except socket.timeout:
                break
            except Exception as e:
                print("Conn error:", e)
                break

print(f"[FakeFPGA] Listening on {HOST}:{PORT}")
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen()
    while True:
        c, a = s.accept()
        threading.Thread(target=handle, args=(c, a), daemon=True).start()
