#pragma once

#include <stdint.h>

/* ---- Packet framing ---- */
#define M1_CMD_MAGIC   0xAB
#define M1_RESP_MAGIC  0xCD

#define M1_MAX_PAYLOAD 60

/* ---- Command IDs ---- */

/* System */
#define CMD_PING              0x01
#define CMD_GET_STATUS        0x02

/* CMD_GET_STATUS payload (protocol version 1) */
#define M1_ESP32_PROTO_VER 1u

/* Standard ESP-AT command bits reported in CMD_GET_STATUS.at_cmd_bitmap */
#define M1_AT_CMD_AT              (1ull << 0)
#define M1_AT_CMD_GMR             (1ull << 1)
#define M1_AT_CMD_CWMODE          (1ull << 2)
#define M1_AT_CMD_CWLAP           (1ull << 3)
#define M1_AT_CMD_CWJAP           (1ull << 4)
#define M1_AT_CMD_CWQAP           (1ull << 5)
#define M1_AT_CMD_CIPSTAMAC       (1ull << 6)
#define M1_AT_CMD_CWSTARTSMART    (1ull << 7)
#define M1_AT_CMD_CWSTOPSMART     (1ull << 8)
#define M1_AT_CMD_BLEINIT         (1ull << 9)
#define M1_AT_CMD_BLESCANPARAM    (1ull << 10)
#define M1_AT_CMD_BLESCAN         (1ull << 11)
#define M1_AT_CMD_BLEGAPADV       (1ull << 12)
#define M1_AT_CMD_BLEADVSTART     (1ull << 13)
#define M1_AT_CMD_BLEADVSTOP      (1ull << 14)
#define M1_AT_CMD_BLEGATTCPRIMSRV (1ull << 15)
#define M1_AT_CMD_BLEGATTCCHAR    (1ull << 16)
#define M1_AT_CMD_BLEGATTCWR      (1ull << 17)
#define M1_AT_CMD_BLEGATTCNTFY    (1ull << 18)

#define M1_ESP32_CMD_WIFI_SCAN     (1ull << 0)
#define M1_ESP32_CMD_STA_SCAN      (1ull << 1)
#define M1_ESP32_CMD_BLE_SCAN      (1ull << 2)
#define M1_ESP32_CMD_BLE_ADV       (1ull << 3)
#define M1_ESP32_CMD_DEAUTH        (1ull << 4)
#define M1_ESP32_CMD_BEACON        (1ull << 5)
#define M1_ESP32_CMD_PROBE_FLOOD   (1ull << 6)
#define M1_ESP32_CMD_KARMA         (1ull << 7)
#define M1_ESP32_CMD_PKTMON        (1ull << 8)
#define M1_ESP32_CMD_PORTAL        (1ull << 9)
#define M1_ESP32_CMD_WIFI_JOIN     (1ull << 10)
#define M1_ESP32_CMD_WIFI_SET_MAC  (1ull << 11)
#define M1_ESP32_CMD_WIFI_SET_CHAN (1ull << 12)
#define M1_ESP32_CMD_NETSCAN       (1ull << 13)
#define M1_ESP32_CMD_AT_BLE_HID    (1ull << 14)
#define M1_ESP32_CMD_AT_BT_MANAGE  (1ull << 15)
#define M1_ESP32_CMD_AT_802154     (1ull << 16)

#define M1_AT_CMD_PROFILE_SIN360 \
    (M1_AT_CMD_AT | \
     M1_AT_CMD_GMR | \
     M1_AT_CMD_CWMODE | \
     M1_AT_CMD_CWLAP | \
     M1_AT_CMD_CWJAP | \
     M1_AT_CMD_CWQAP | \
     M1_AT_CMD_CIPSTAMAC | \
     M1_AT_CMD_BLEINIT | \
     M1_AT_CMD_BLESCANPARAM | \
     M1_AT_CMD_BLESCAN | \
     M1_AT_CMD_BLEGAPADV | \
     M1_AT_CMD_BLEADVSTART | \
     M1_AT_CMD_BLEADVSTOP | \
     M1_AT_CMD_BLEGATTCPRIMSRV | \
     M1_AT_CMD_BLEGATTCCHAR | \
     M1_AT_CMD_BLEGATTCWR | \
     M1_AT_CMD_BLEGATTCNTFY)

/* Standard ESP-AT command profile assumed when CMD_GET_STATUS receives no response.
 * Detection flow (host side):
 *   1. Send CMD_GET_STATUS on the binary SPI channel.
 *   2. Valid response -> firmware reports m1_esp32_status_payload_t with
 *      unified command bitmap (M1_ESP32_CMD_* bits).
 *   3. Timeout / no magic -> host may use a static fallback profile for
 *      older firmware that does not self-report.
 *
 * GATT client commands (M1_AT_CMD_BLEGATTCPRIMSRV / BLEGATTCCHAR / BLEGATTCWR /
 * BLEGATTCNTFY) are intentionally excluded from this profile.  They require the
 * NimBLE-based ESP-AT build; stock Espressif AT binaries do not always enable
 * them.  Customised builds may report additional capabilities through
 * AT+GETSTATUSHEX.
 */
#define M1_AT_CMD_PROFILE_DEFAULT \
    (M1_AT_CMD_AT | \
     M1_AT_CMD_GMR | \
     M1_AT_CMD_CWMODE | \
     M1_AT_CMD_CWLAP | \
     M1_AT_CMD_CWJAP | \
     M1_AT_CMD_CWQAP | \
     M1_AT_CMD_CIPSTAMAC | \
     M1_AT_CMD_BLEINIT | \
     M1_AT_CMD_BLESCANPARAM | \
     M1_AT_CMD_BLESCAN | \
     M1_AT_CMD_BLEGAPADV | \
     M1_AT_CMD_BLEADVSTART | \
     M1_AT_CMD_BLEADVSTOP)

/* WiFi scan (preserve existing functionality) */
#define CMD_WIFI_SCAN_START   0x10
#define CMD_WIFI_SCAN_NEXT    0x11
#define CMD_WIFI_SCAN_STOP    0x12

/* BLE */
#define CMD_BLE_SCAN_START    0x20
#define CMD_BLE_SCAN_NEXT     0x21
#define CMD_BLE_SCAN_STOP     0x22
#define CMD_BLE_ADV_START     0x23
#define CMD_BLE_ADV_STOP      0x24
#define CMD_BLE_ADV_RAW       0x25
#define CMD_BLE_SCAN_NEXT_RAW 0x26
#define CMD_BLE_ADV_RAW_EX    0x27
#define CMD_BLE_GATT_START    0x28
#define CMD_BLE_GATT_NEXT     0x29
#define CMD_BLE_GATT_STOP     0x2A
#define CMD_BLE_GATT_WRITE    0x2B
#define CMD_BLE_GATT_SUB      0x2C
#define CMD_BLE_GATT_NOTIF    0x2D

/* Station scan (promiscuous client discovery) */
#define CMD_STA_SCAN_START    0x13
#define CMD_STA_SCAN_NEXT     0x14
#define CMD_STA_SCAN_STOP     0x15

/* WiFi attacks */
#define CMD_DEAUTH_START      0x30
#define CMD_DEAUTH_STOP       0x31
#define CMD_BEACON_START      0x32
#define CMD_BEACON_STOP       0x33
#define CMD_PROBE_FLOOD_START 0x34
#define CMD_PROBE_FLOOD_STOP  0x35
#define CMD_BEACON_SET_FLAGS  0x36
#define CMD_KARMA_START       0x37
#define CMD_KARMA_STOP        0x38
#define CMD_DEAUTH_MULTI      0x39
#define CMD_KARMA_STATUS      0x3A
#define CMD_KARMA_PORTAL_START 0x3B
#define CMD_WIFI_RAW_ATTACK_START 0x3C
#define CMD_WIFI_RAW_ATTACK_STOP  0x3D

/* Sniffer / packet monitor
 * START payload[0] = sniff type, [1] = channel (0=hop), [2] = hop interval */
#define CMD_PKTMON_START      0x40
#define CMD_PKTMON_NEXT       0x41
#define CMD_PKTMON_STOP       0x42
#define CMD_PKTMON_SET_CHAN   0x43
#define CMD_PKTMON_RAW_NEXT   0x44

/* Sniffer type constants */
#define SNIFF_ALL             0x00
#define SNIFF_BEACON          0x01
#define SNIFF_PROBE_REQ       0x02
#define SNIFF_DEAUTH          0x03
#define SNIFF_EAPOL           0x04
#define SNIFF_SIGNAL          0x05
#define SNIFF_PWNAGOTCHI      0x06
#define SNIFF_SAE             0x07

/* Evil portal */
#define CMD_PORTAL_START      0x50
#define CMD_PORTAL_STOP       0x51
#define CMD_PORTAL_CREDS      0x52
#define CMD_PORTAL_HTML_CLEAR 0x56
#define CMD_PORTAL_HTML_ADD   0x57

/* SSID management */
#define CMD_SSID_ADD          0x53
#define CMD_SSID_CLEAR        0x54
#define CMD_SSID_COUNT        0x55

/* WiFi general */
#define CMD_WIFI_JOIN         0x58
#define CMD_WIFI_DISCONNECT   0x59
#define CMD_WIFI_SET_MAC      0x5A
#define CMD_WIFI_SET_CHANNEL  0x5B
#define CMD_NETSCAN_START     0x5C
#define CMD_NETSCAN_NEXT      0x5D
#define CMD_NETSCAN_STOP      0x5E

/* ---- Response status ---- */
#define RESP_OK    0x00
#define RESP_ERR   0x01
#define RESP_BUSY  0x02

/* ---- Packet structures ---- */

/* Command: STM32 -> ESP32 (64 bytes) */
typedef struct {
    uint8_t  magic;           /* M1_CMD_MAGIC */
    uint8_t  cmd_id;
    uint8_t  payload_len;
    uint8_t  payload[61];
} __attribute__((packed)) m1_cmd_t;

/* Response: ESP32 -> STM32 (64 bytes) */
typedef struct {
    uint8_t  magic;           /* M1_RESP_MAGIC */
    uint8_t  cmd_id;
    uint8_t  status;
    uint8_t  payload_len;
    uint8_t  payload[M1_MAX_PAYLOAD];
} __attribute__((packed)) m1_resp_t;

typedef struct {
    uint8_t  proto_ver;
    uint64_t cmd_bitmap;
    char     fw_name[32];
} __attribute__((packed)) m1_esp32_status_payload_t;

_Static_assert(sizeof(m1_cmd_t) == 64, "m1_cmd_t must be 64 bytes");
_Static_assert(sizeof(m1_resp_t) == 64, "m1_resp_t must be 64 bytes");
/* 1 byte proto_ver + 8 bytes cmd_bitmap + 32 bytes fw_name = 41 bytes */
_Static_assert(sizeof(m1_esp32_status_payload_t) == 41,
               "m1_esp32_status_payload_t must be 41 bytes");

/* ---- ESP32-AT-M1 firmware capability profiles ---- */

/*
 * AT command profile for the ESP32-AT-M1 firmware.
 * This firmware provides the full SIN360 ESP-AT command set, including GATT
 * client operations, on top of the baseline M1_AT_CMD_PROFILE_DEFAULT.
 */
#define M1_AT_CMD_PROFILE_ESP32AT   M1_AT_CMD_PROFILE_SIN360

/* Unified profile bitmaps used by AT+GETSTATUSHEX (single cmd_bitmap payload). */
#define M1_ESP32_CMD_PROFILE_AT_BEDGE117 \
    (M1_ESP32_CMD_AT_BLE_HID | \
     M1_ESP32_CMD_AT_802154)

#define M1_ESP32_CMD_PROFILE_AT_NEDDY299 \
    (M1_ESP32_CMD_PROFILE_AT_BEDGE117 | \
     M1_ESP32_CMD_STA_SCAN            | \
     M1_ESP32_CMD_DEAUTH)

#define M1_ESP32_CMD_PROFILE_ESP32AT  M1_ESP32_CMD_PROFILE_AT_NEDDY299

/* Firmware identifier reported by AT+GETSTATUSHEX */
#define M1_FW_NAME_ESP32AT  "ESP32AT-M1"
