# NetPulse

A lightweight network quality monitor with a C daemon backend and Vite + React dashboard. Monitors network latency, packet loss, and jitter in real-time.

## Features

- **Real-time monitoring**: Probes targets every 500ms, updates metrics every second
- **Two probe modes**: TCP connect timing (default) or ICMP ping (Linux only)
- **Live dashboard**: React frontend with time-series charts and health grades
- **No root required**: TCP mode works without elevated permissions
- **Multiple targets**: Monitor Cloudflare, Google, or custom endpoints
- **Bad minute detection**: Alerts when network quality degrades

## Quick Start

```bash
# Build and run the daemon
make
./build/netpulsed

# In another terminal, start the frontend
cd frontend
npm install
npm run dev
```

Open http://localhost:5173 to view the dashboard.

## Screenshots

The dashboard displays:
- Per-target RTT charts with failure markers
- Packet loss, jitter, P50/P95 latency metrics
- Overall network health grade (A-F)
- Event log for bad minutes

## Probe Modes

| Mode | Platform | Permissions | Use Case |
|------|----------|-------------|----------|
| TCP (default) | All | None | General monitoring, works everywhere |
| ICMP | Linux | CAP_NET_RAW | Lower latency, true network RTT |

### TCP Mode (Default)
```bash
./build/netpulsed
```
Measures TCP handshake time. Typical RTT: 7-15ms to major DNS providers.

### ICMP Mode (Linux Only)
```bash
# Grant capability once
sudo setcap cap_net_raw+ep ./build/netpulsed

# Run with ICMP
./build/netpulsed --probe-type icmp
```
Measures ICMP Echo round-trip time. Typical RTT: 1-5ms to major DNS providers.

## Requirements

### Backend
- GCC with C17 support
- POSIX-compliant OS (macOS, Linux)

### Frontend
- Node.js 18+
- npm

## Project Structure

```
.
├── src/                    # C daemon source
│   ├── main.c              # Entry point and main loop
│   ├── core/               # Config, scheduler, stats, ring buffer
│   ├── net/                # DNS, TCP probe, ICMP probe
│   ├── server/             # HTTP/WebSocket server (Mongoose)
│   └── platform/           # Cross-platform time and filesystem
├── frontend/               # React + TypeScript dashboard
│   ├── src/
│   │   ├── pages/          # Dashboard and Settings views
│   │   ├── components/     # TargetCard, RttChart, HealthGrade
│   │   └── stores/         # Zustand state management
│   └── vite.config.ts
├── third_party/mongoose/   # Embedded HTTP/WebSocket library
└── Makefile
```

## API

### WebSocket (ws://localhost:7331/ws)

Real-time updates pushed to connected clients:
- `snapshot`: Initial state on connect
- `sample`: New probe result (every 500ms per target)
- `metrics`: Updated statistics (every second)
- `event`: Bad minute detection alerts

### HTTP

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/health` | GET | Health check with uptime |
| `/api/config` | GET/POST | Get or update configuration |
| `/api/targets` | POST | Add or remove monitoring targets |

## Configuration

Default targets: Cloudflare (1.1.1.1:443) and Google (8.8.8.8:443)

Add custom targets via the Settings page or API:
```bash
curl -X POST http://localhost:7331/api/targets \
  -H "Content-Type: application/json" \
  -d '{"action":"add","id":"custom","label":"My Server","host":"example.com","port":443}'
```

## Metrics

- **RTT**: Round-trip time in milliseconds
- **Packet Loss**: Percentage of failed probes (120-sample window)
- **Jitter**: Mean absolute deviation between consecutive RTTs
- **P50/P95**: 50th and 95th percentile latency

## Linux/Gitpod Setup

See [LINUX_BUILD.md](LINUX_BUILD.md) for container and Gitpod-specific instructions.

## License

MIT
