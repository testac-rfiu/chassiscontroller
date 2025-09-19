# Fake FPGA SCPI Server (2×4)

`fake_fpga.py` is a tiny **SCPI-over-TCP** simulator for a chassis controller with **2 switches × 4 paths**. Use it to drive your Soft Front Panel or test with `curl`/`netcat` without real hardware.

---

## Prerequisites
- **Python 3.x** (no extra packages required)

---

## Run

```bash
# Default port 5025
python fake_fpga.py

# Custom port
python fake_fpga.py 6000
