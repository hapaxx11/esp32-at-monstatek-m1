# Agent Instructions for esp32-at-monstatek-m1

This file captures important context for AI coding agents working in this repository.

## Project Overview

This is a fork of the [ESP-AT](https://github.com/espressif/esp-at) firmware project targeting ESP32C6 (and other ESP32 variants), adding custom AT commands for Wi-Fi deauthentication (`AT+DEAUTH`), station scanning (`AT+STASCAN`), and a binary capability probe (`CMD_GET_STATUS`).

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
