# Linux/Gitpod Build Notes

## Issues Encountered and Fixes

### 1. POSIX Functions Not Found (sigaction, clock_gettime, poll, getaddrinfo)
**Error:** `storage size of 'sa' isn't known`, `implicit declaration of function 'clock_gettime'`

**Cause:** Compiling with `-std=c17` enables strict ISO C mode, which hides POSIX extensions by default on Linux (glibc).

**Fix:** Added `-D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE` to CFLAGS in the Makefile. This exposes POSIX.1-2008 functions and BSD extensions needed by Mongoose.

### 3. Vite Not Accessible from Outside Container
**Error:** 503 Service Unavailable when accessing Gitpod URL

**Cause:** Vite binds to `localhost` by default, which isn't accessible externally.

**Fix:** Run with `--host 0.0.0.0`:
```bash
npm run dev -- --host 0.0.0.0
```

### 4. Vite Blocking External Hosts
**Error:** `This host is not allowed`

**Cause:** Vite's host allowlist doesn't include dynamic Gitpod URLs.

**Fix:** Added `allowedHosts: 'all'` to vite.config.ts server options.

### 5. Gitpod Port Forwarding
Ports 5173 (Vite) and 7331 (daemon) need to be exposed in Gitpod's Ports panel.


## Quick Start on Linux/Gitpod

```bash
# Build backend
make clean && make

# Run daemon
./build/netpulsed

# In another terminal, run frontend
cd frontend
npm install
npm run dev -- --host 0.0.0.0
```
