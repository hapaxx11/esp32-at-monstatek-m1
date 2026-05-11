# Neddy299 M1 ESP-AT Test Lab

In addition to the AT commands provided by the parent project [cd3daddy's M1 ESP-AT](https://github.com/bedge117/esp32-at-monstatek-m1), this fork provides the following AT commands:


## AT+DEAUTH

Conduct Wi-Fi deauth attack.

Deauth commands
- Execute deauth: `AT+DEAUTH=<enable_mode>,<ch>,<mac>,<bssid>`
  - enable_mode: 0 = off, 1 to [NUM_ENABLE_MODES](https://github.com/neddy299/esp32-at-monstatek-m1/blob/c49f5b955dc76126346ca0dd41dcbe5e45356aaa/main/at_custom_deauth.c#L10) = start attack
  - ch: wireless channel
  - mac: target station MAC address
  - bssid: target wireless access point BSSID

- Stop deauth: `AT+DEAUTH=0`

- Query status: `AT+DEAUTH?`
  - Response: `+DEAUTH:(<enabled>,<channel>,<enable_mode>,<num_modes>)`
    - enabled: 0 = attack stopped, 1 = attack started
    - channel: wifi channel
    - enable_mode: active deauth enable mode
    - num_modes: number of available deauth modes ([NUM_ENABLE_MODES](https://github.com/neddy299/esp32-at-monstatek-m1/blob/c49f5b955dc76126346ca0dd41dcbe5e45356aaa/main/at_custom_deauth.c#L10))

## AT+STASCAN

This command will sniff and report the wireless clients connected to a wireless access point.

Note: this firmware's [M1 project](https://github.com/neddy299/M1) project uses the built in ESP `AT+CWLAP` command to collect a list of available access points and then uses this AT+STASCAN command to collect a list of available targets.

- Execute station scanning: `AT+STASCAN=<enable>,<ch>,<bssid>`
  - enable: 0 = off, 1 = start sniffing
  - ch: wireless channel
  - bssid: target wireless access point BSSID
  - Response: `+STASCAN:("<mac>")`
    - mac: MAC address of detected station

- Stop station scanning: `AT+STASCAN=0`

- Query status: `AT+STASCAN?`
  - Response: `+STASCAN:(<enabled>,<channel>)`
    - enabled: 0 = sniff stopped, 1 = snifff started
    - channel: wifi channel

## CMD_GET_STATUS binary protocol

In addition to the AT text interface, this firmware responds to the M1 host's
binary capability probe (opcode `0x02`) over SPI.  This eliminates the ~5 s
`AT+CMD?` timeout the M1 would otherwise wait through on every boot before
falling back to AT text enumeration.

**Wire format** (host → firmware): 1 byte `0x02`

**Response** (firmware → host): 41-byte packed payload

| Offset | Size | Field | Value |
|--------|------|-------|-------|
| 0 | 1 | `proto_ver` | `0x01` |
| 1–8 | 8 | `cap_bitmap` | `0x14412` (little-endian) |
| 9–40 | 32 | `fw_name` | `"AT-neddy299-1.0.1"` (null-terminated) |

**Capability bits set** (matching the commands this firmware implements):

| Bit | Constant | Command |
|-----|----------|---------|
| 1 | `M1_ESP32_CAP_STA_SCAN` | `AT+STASCAN` |
| 4 | `M1_ESP32_CAP_DEAUTH` | `AT+DEAUTH` |
| 10 | `M1_ESP32_CAP_WIFI_JOIN` | `AT+CWJAP` / `AT+CWQAP` |
| 14 | `M1_ESP32_CAP_BLE_HID` | `AT+BLEHIDINIT` / `AT+BLEHIDKB` |
| 16 | `M1_ESP32_CAP_802154` | `AT+ZIGSNIFF` |

The implementation lives in `main/interface/spi/at_spi_task_esp32_series.c`
(receive loop) and `main/include/at_m1_status.h` (constants, struct, helper).
Other ESP32 firmware variants can copy `at_m1_status.h`, adjust
`M1_ESP32_THIS_FW_CAP_BITMAP` and `M1_ESP32_THIS_FW_NAME`, and add the same
opcode check to their own SPI receive loop.

## Building

The following instructions are for Linux, but with minor changes will work on Windows and macOS.

### Prerequisites

- **ESP-IDF 5.1+**
- **PlatformIO** - for wifi patch

### Building

```bash
# Activate ESP-IDF (update version number to match your IDF version)
source ~/.espressif/tools/activate_idf_v5.5.3.sh

# Build
./build.py build
```

ESP32 firmware will be saved in `./build/factory`. Use [qMonstatek](https://github.com/bedge117/qMonstatek) to flash ESP32 firmware or copy both firmware files to your M1 and flash through the device UI.

### Patch ESP32 Wi-Fi library

> IMPORTANT: the ESP32 Wi-Fi library (libnet80211.a) must be patched for the deauth attack to function.

This only needs to be done once. Make sure that you have built your project at least once before running this.

```bash
./patch-libnet.sh
```

## Additional Notes

The functionality implemented by these AT commands are based on and inspired by the [ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder) project.


-----------------------------
-----------------------------

# ESP-AT
[![Documentation Version](./docs/_static/at_doc_latest.svg)](https://docs.espressif.com/projects/esp-at/en/latest/)

- [中文版](#esp-at-项目)

esp-at project was started and powered by Espressif Systems (@[espressif](https://github.com/espressif/)) as an official project, for the **ESP32-C2**, **ESP32-C3**, **ESP32-C6**, **ESP32**, and **ESP8266** Series SoCs provided for Windows, Linux, and macOS.
It is now supported and maintained by Espressif esp-at team (@[esp-at](https://github.com/espressif/esp-at)).

# Introduction
Espressif Wi-Fi and Bluetooth® chipsets are often used as add-on modules to seamlessly integrate wireless connectivity features into new and existing products.
In an effort to facilitate this and cut down on engineering costs, Espressif Systems has developed a set of AT commands that can be used to interface with Espressif products.

"AT" means 'attention'. Each command string is prefixed with "AT", and a number of discrete commands can be concatenated after the "AT".

The AT command firmware allows for rapid integration by providing:

- In-built TCP/IP stack and data buffering
- Easy integration with resource-constrained host platforms
- Easy-to-parse command-response protocols
- Customized, user-defined AT commands

# Resources
- There are several guides for esp-at developers and users. These guides can be rendered in a number of formats, like HTML and PDF.
  Documentation for the latest version: [https://docs.espressif.com/projects/esp-at/en/latest/index.html](https://docs.espressif.com/projects/esp-at/en/latest/index.html). This documentation is built from the [docs directory](https://github.com/espressif/esp-at/tree/master/docs) of this repository.

- The Changelogs of historic released versions: https://github.com/espressif/esp-at/releases

- [Check the Issues section on GitHub](https://github.com/espressif/esp-at/issues) if you find a bug or have a feature request. Please check existing Issues before opening a new one.

- The [esp-at forum](https://www.esp32.com/viewforum.php?f=42) is a place to ask questions and find community resources.

- The [ESP Product Selector](https://products.espressif.com/#/product-selector?language=en&names=) is a tool to help you compare different chips or modules.

# ESP-AT Support Policy for ESP Chip Series

- ESP32-C2 Series
  - **Preferred recommended chip for using ESP-AT on**
  - Recommended released version: [v3.1.0.0](https://github.com/espressif/esp-at/releases/tag/v3.1.0.0)

- ESP32-C3 Series
  - **Preferred recommended chip for using ESP-AT on**
  - Recommended released version: [v3.2.0.0](https://github.com/espressif/esp-at/releases/tag/v3.2.0.0)

- ESP32-C6 Series
  - **Preferred recommended chip for using ESP-AT on**
  - Recommended released version: [v4.0.0.0](https://github.com/espressif/esp-at/releases/tag/v4.0.0.0)

- ESP32 Series
  - Recommended released version: [v3.2.0.0](https://github.com/espressif/esp-at/releases/tag/v3.2.0.0)

- ESP8266 Series
  - **ESP32-C2 is recommended to use instead**
  - ESP-AT will not release the major version for ESP8266.
  - ESP-AT no longer adds new features to ESP8266.
  - [v2.2.1.0_esp8266](https://github.com/espressif/esp-at/releases/tag/v2.2.1.0_esp8266) is the last version of ESP-AT for ESP8266, corresponding to branch [release/v2.2.0.0_esp8266](https://github.com/espressif/esp-at/tree/release/v2.2.0.0_esp8266), corresponding to documentation [https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266](https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/).
  - ESP-AT will regularly update [release/v2.2.0.0_esp8266](https://github.com/espressif/esp-at/tree/release/v2.2.0.0_esp8266) branch for ESP8266. Update includes vital bugfix and security repair.

- Other Series
  - ESP-AT will support ESP32-C5 series of chips.
  - ESP-AT will not support ESP32-S and ESP32-H series of chips.

# ESP-AT 项目
esp-at 作为由 Espressif Systems (@[espressif](https://github.com/espressif/)) 发起和提供技术支持的官方项目，适用于 Windows、Linux、macOS 上的 **ESP32-C2**、**ESP32-C3**、**ESP32-C6**、**ESP32**、和 **ESP8266** 系列芯片。
当前该项目由 Espressif esp-at 团队 (@[esp-at](https://github.com/espressif/esp-at)) 负责技术支持和维护。

# 简介
乐鑫 Wi-Fi 和蓝牙芯片可以用作附加模块，完美集成在其他现有产品上，提供无线通讯功能。
为降低客户开发成本，乐鑫开发了一套 AT 指令集，方便客户简单快速地使用 AT 指令来控制芯片。

乐鑫提供的 AT 指令固件具有以下特色，利于芯片集成到应用中：

- 内置 TCP/IP 堆栈和数据缓冲
- 能便捷地集成到资源受限的主机平台中
- 主机对指令的回应易于解析
- 用户可自定义 AT 指令

# 资源
- 这里为开发者和用户提供了一些指南，这些指南可以以多种格式呈现，比如 HTML 和 PDF。
  最新文档见：[https://docs.espressif.com/projects/esp-at/zh_CN/latest/index.html](https://docs.espressif.com/projects/esp-at/zh_CN/latest/index.html)。该文档是由本仓库 [docs 目录](https://github.com/espressif/esp-at/tree/master/docs) 自动编译构建的。

- 已发布版本的修改记录见：https://github.com/espressif/esp-at/releases

- 如果你有 bug 上报或者功能需求，可以在 [GitHub Issues](https://github.com/espressif/esp-at/issues) 里提交。请在提交前检索下是否有相同的 Issue。

- [esp-at 社区](https://www.esp32.com/viewforum.php?f=34) 可以用来询问问题或寻找一些社区资源。

- [产品选型工具](https://products.espressif.com/#/product-selector?language=zh&names=) 可以帮助你对比不同的芯片或模组。

# ESP-AT 对不同芯片系列支持策略

- ESP32-C2 系列
  - **ESP-AT 首选推荐芯片**
  - 推荐使用的发布版本：[v3.1.0.0](https://github.com/espressif/esp-at/releases/tag/v3.1.0.0)

- ESP32-C3 系列
  - **ESP-AT 首选推荐芯片**
  - 推荐使用的发布版本：[v3.2.0.0](https://github.com/espressif/esp-at/releases/tag/v3.2.0.0)

- ESP32-C6 系列
  - **ESP-AT 首选推荐芯片**
  - 推荐使用的发布版本：[v4.0.0.0](https://github.com/espressif/esp-at/releases/tag/v4.0.0.0)

- ESP32 系列
  - 推荐使用的发布版本：[v3.2.0.0](https://github.com/espressif/esp-at/releases/tag/v3.2.0.0)

- ESP8266 系列
  - **推荐使用 ESP32-C2 芯片**
  - ESP-AT 不再为 ESP8266 添加新功能
  - ESP-AT 不再为 ESP8266 发布大版本
  - [v2.2.1.0_esp8266](https://github.com/espressif/esp-at/releases/tag/v2.2.1.0_esp8266) 是 ESP-AT 为 ESP8266 发布的最后一个版本，对应分支为 [release/v2.2.0.0_esp8266](https://github.com/espressif/esp-at/tree/release/v2.2.0.0_esp8266)，对应文档为 [https://docs.espressif.com/projects/esp-at/zh_CN/release-v2.2.0.0_esp8266](https://docs.espressif.com/projects/esp-at/zh_CN/release-v2.2.0.0_esp8266/)
  - ESP-AT 将定期在 [release/v2.2.0.0_esp8266](https://github.com/espressif/esp-at/tree/release/v2.2.0.0_esp8266) 分支为 ESP8266 进行重要 bug 修复，安全修补等更新

- 其它系列
  - ESP-AT 将计划支持 ESP32-C5 系列芯片
  - ESP-AT 无计划支持 ESP32-S、ESP32-H 系列芯片
