# AX25LIB

A basic C++ AX25 implementation with python bindings.

## TODO (AX.25 coverage)
- [x] Frame format basics
	- [x] UI frame control field (0x03) with PID 0xF0
	- [x] KISS escape/unescape helpers
- [x] Addressing
	- [x] Destination/source address encode/decode (callsign + SSID)
	- [ ] Digipeater chains (up to 8), repeated bit handling, and emit/parse multiple address blocks
- [x] Control field families
	- [ ] I frames: N(R)/N(S), P/F bit, segmentation rules
	- [ ] S frames: RR, RNR, REJ, SREJ with sequence numbers and P/F bit
	- [ ] U frames: SABM, SABME, DISC, UA, DM, FRMR, UI* variants with P/F bit semantics
- [ ] Layer 3 PID support
	- [ ] NET/ROM, IP, ARP, and reserved PID handling (beyond 0xF0)
	- [ ] Expose PID in decoder output and validate against control type
- [ ] Framing robustness
	- [ ] HDLC flags (0x7E) and bit-stuffing/unstuffing for over-the-air frames
	- [ ] CRC/FCS-16 (CCITT) generation and validation; drop frames with bad FCS
	- [ ] Length and field sanity checks (address multiples of 7, minimum header sizes)
- [ ] Decode/encode outputs
	- [ ] Surface structured decode details (frame type, control subtype, PID, seq numbers, P/F bit) instead of only payload/callsigns
- [ ] Testing
	- [ ] Encoder/decoder round-trips for all frame types
	- [ ] KISS escaping edge cases
	- [ ] Multi-address routing and digipeater repetition bit cases
	- [ ] Negative cases: bad FCS, truncated headers, invalid escapes

## Python package

Install the Python bindings directly from GitHub with pip:

```bash
pip install "AX25Python @ git+https://github.com/jezza5400-org/AX25Lib.git"
```

This builds the extension from source, so the machine running pip needs a working C++17 toolchain and a Python environment that can compile native extensions.

After install, import the module as:

```python
import AX25Python
```