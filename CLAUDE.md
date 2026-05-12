# Agent Instructions for esp32-at-monstatek-m1

This file captures important context for AI coding agents working in this repository.

## Project Overview

This is **ESP32-C6 SPI AT firmware for the Monstatek M1** — a pre-configured fork of [ESP-AT](https://github.com/espressif/esp-at) with the correct SPI pin mapping and SPI half-duplex fix for the M1 hardware.

Beyond the upstream ESP-AT baseline, this fork adds:

- **`AT+DEAUTH`** — Wi-Fi deauthentication attack (`main/at_custom_deauth.c`)
- **`AT+STASCAN`** — Sniff stations connected to an AP (`main/at_custom_stascan.c`)
- **`CMD_GET_STATUS` binary opcode `0x02`** — M1 capability probe response (`main/interface/spi/at_spi_task_esp32_series.c` + `main/include/at_m1_status.h`)

### CMD_GET_STATUS purpose (PR #2)

The M1 host probes firmware capabilities via binary opcode `0x02` over SPI before falling back to a ~5 s `AT+CMD?` text enumeration. Without a handler, every boot paid that full timeout cost. This firmware intercepts opcode `0x02` in the SPI receive loop and responds with a packed 41-byte payload:

| Field | Size | Value |
|-------|------|-------|
| `proto_ver` | 1 byte | `0x01` |
| `cap_bitmap` | 8 bytes | `0x14412` (little-endian) — `STA_SCAN \| DEAUTH \| WIFI_JOIN \| BLE_HID \| 802154` |
| `fw_name` | 32 bytes | `"AT-neddy299-1.0.1"` (null-terminated) |

The AT framework never sees the binary opcode — it is consumed entirely in the SPI task. Constants and struct are defined in `main/include/at_m1_status.h`, which is designed to be portable to other ESP32 firmware variants (adjust `M1_ESP32_THIS_FW_CAP_BITMAP` and `M1_ESP32_THIS_FW_NAME`, then add the same opcode check to their SPI receive loop).

---

## Critical: libnet80211 Symbol Weakening (Deauth Prerequisite)

**Do NOT remove or disable the `ieee80211_raw_frame_sanity_check` function in `main/at_custom_deauth.c`.**

This function is an intentional link-time override required for the `AT+DEAUTH` command to work. The ESP32 WiFi library (`libnet80211.a`) contains a strong symbol with this name that blocks raw 802.11 management frame transmission. The override makes raw deauthentication frames pass through by returning `1` when `arg == 31337`.

### Why the linker error occurs

On ESP-IDF 5.1+ / ESP32C6, `libnet80211.a` exports `ieee80211_raw_frame_sanity_check` as a **strong** symbol. Linking this together with the custom override in `at_custom_deauth.c` causes a **multiple definition** linker error.

### The correct fix: weaken the library symbol

The strong symbol in `libnet80211.a` must be downgraded to a **weak** symbol using `objcopy` so the custom override wins at link time. This is what `patch-libnet.sh` does manually, and it is also automated in the CI build pipeline (`release.yml`):

```bash
riscv32-esp-elf-objcopy \
  --weaken-symbol=ieee80211_raw_frame_sanity_check \
  esp-idf/components/esp_wifi/lib/esp32c6/libnet80211.a \
  esp-idf/components/esp_wifi/lib/esp32c6/libnet80211.a.patched
```

**Never attempt to fix a duplicate-symbol linker error by deleting this function.** That would break deauth functionality silently — the build would succeed but `AT+DEAUTH` would stop working because the ROM's implementation blocks raw frame injection.

### For local builds

Run `./patch-libnet.sh` once after building at least once. This requires PlatformIO (`pio`) to be available.

### For CI builds

The `release.yml` workflow applies the symbol weakening automatically before `./build.py build`, using the `riscv32-esp-elf-objcopy` from the ESP-IDF toolchain installed by `./build.py install`.

---

## Build Instructions

The build system is `./build.py` (requires Python with `xlrd` and other packages from `requirements.txt`).

```bash
# Install prerequisites
pip install -r requirements.txt

# Install ESP-IDF and toolchains (one-time)
./build.py install

# Build firmware (interactive: prompts for platform, module, silence mode)
./build.py build
```

Build output lands in `./build/factory`. Flash using [qMonstatek](https://github.com/bedge117/qMonstatek) or via the device UI.

---

## Custom AT Commands

| Command | File | Description |
|---------|------|-------------|
| `AT+DEAUTH` | `main/at_custom_deauth.c` | Wi-Fi deauthentication attack |
| `AT+STASCAN` | `main/at_custom_stascan.c` | Sniff stations connected to an AP |
| Binary `0x02` probe | `main/interface/spi/at_spi_task_esp32_series.c` | M1 capability probe response |

---

## Module Configurations

Module configs live under `module_config/`. Each directory must contain an `IDF_VERSION` file. The primary target is `module_esp32c6-spi` (ESP32C6, SPI interface).
