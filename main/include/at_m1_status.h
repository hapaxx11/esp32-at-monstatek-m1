/*
 * at_m1_status.h — M1 binary CMD_GET_STATUS protocol
 *
 * This header defines the binary SPI protocol used by the Monstatek M1 host
 * (STM32H573) to query ESP32 firmware capabilities at runtime.
 *
 * ─── Protocol overview ────────────────────────────────────────────────────
 *
 * The M1 host first tries a binary CMD_GET_STATUS probe (opcode 0x02) before
 * falling back to the AT+CMD? text enumeration. Responding to opcode 0x02
 * eliminates the ~5 s AT+CMD? timeout that would otherwise occur on every
 * STM32 boot when the host waits for the AT task to become ready.
 *
 * Wire format (master → slave):
 *   1 byte: opcode (M1_CMD_GET_STATUS_OPCODE = 0x02)
 *
 * Wire format (slave → master):
 *   41 bytes: m1_esp32_status_payload_t (packed, little-endian)
 *     [0]    proto_ver  (must be 0x01)
 *     [1..8] cap_bitmap (uint64_t, little-endian, M1_ESP32_CAP_* bits)
 *     [9..40] fw_name   (null-terminated ASCII string, zero-padded to 32 bytes)
 *
 * ─── Adding CMD_GET_STATUS to a new ESP32 firmware ────────────────────────
 *
 * 1. Include this header.
 * 2. Define your firmware's capability bitmap by OR-ing the M1_ESP32_CAP_*
 *    bits that match the features your firmware supports.
 * 3. In the SPI receive loop, after the DMA read from the master, check:
 *      if (data_buf[0] == M1_CMD_GET_STATUS_OPCODE)
 *    If true, build and transmit the payload using at_m1_status_build_payload()
 *    and write it to the SPI TX path — do NOT forward the opcode to the AT
 *    framework.
 *
 * ─── Reference ────────────────────────────────────────────────────────────
 *
 * The M1 host-side structures and probe logic live in:
 *   m1_csrc/m1_esp32_caps.h / m1_esp32_caps.c
 * in the hapaxx11/M1 repository (branch copilot/add-abstraction-layer-esp32-firmware).
 */

#ifndef AT_M1_STATUS_H
#define AT_M1_STATUS_H

#include <stdint.h>
#include <string.h>

/* ── Protocol constants ──────────────────────────────────────────────────── */

/** Binary opcode sent by the M1 host to request a capability descriptor. */
#define M1_CMD_GET_STATUS_OPCODE    0x02u

/** Protocol version field value.  Must be 0x01 for the current layout. */
#define M1_ESP32_STATUS_PROTO_VER   0x01u

/** Total size of the response payload in bytes (1 + 8 + 32). */
#define M1_ESP32_STATUS_PAYLOAD_SIZE 41u

/* ── Capability bits (cap_bitmap) ───────────────────────────────────────── */
/*
 * Each bit indicates that the firmware supports the corresponding feature
 * group.  The M1 host gates UI options and runtime calls on these bits.
 * Bits 17-63 are reserved for future use; leave them clear.
 *
 * These definitions mirror M1_ESP32_CAP_* in the M1 host firmware.
 */

#define M1_ESP32_CAP_WIFI_SCAN      (1ULL << 0)   /**< CMD_WIFI_SCAN_* */
#define M1_ESP32_CAP_STA_SCAN       (1ULL << 1)   /**< AT+STASCAN */
#define M1_ESP32_CAP_BLE_SCAN       (1ULL << 2)   /**< CMD_BLE_SCAN_* */
#define M1_ESP32_CAP_BLE_ADV        (1ULL << 3)   /**< CMD_BLE_ADV_* */
#define M1_ESP32_CAP_DEAUTH         (1ULL << 4)   /**< AT+DEAUTH */
#define M1_ESP32_CAP_BEACON         (1ULL << 5)   /**< CMD_BEACON_* */
#define M1_ESP32_CAP_PROBE_FLOOD    (1ULL << 6)   /**< CMD_PROBE_FLOOD_* */
#define M1_ESP32_CAP_KARMA          (1ULL << 7)   /**< CMD_KARMA_* */
#define M1_ESP32_CAP_PKTMON         (1ULL << 8)   /**< CMD_PKTMON_* */
#define M1_ESP32_CAP_PORTAL         (1ULL << 9)   /**< CMD_PORTAL_* / CMD_SSID_* */
#define M1_ESP32_CAP_WIFI_JOIN      (1ULL << 10)  /**< AT+CWJAP / AT+CWQAP */
#define M1_ESP32_CAP_WIFI_SET_MAC   (1ULL << 11)  /**< CMD_WIFI_SET_MAC */
#define M1_ESP32_CAP_WIFI_SET_CHAN  (1ULL << 12)  /**< CMD_WIFI_SET_CHANNEL */
#define M1_ESP32_CAP_NETSCAN        (1ULL << 13)  /**< CMD_NETSCAN_* */
#define M1_ESP32_CAP_BLE_HID        (1ULL << 14)  /**< AT+BLEHIDINIT / AT+BLEHIDKB */
#define M1_ESP32_CAP_BT_MANAGE      (1ULL << 15)  /**< AT BT device management */
#define M1_ESP32_CAP_802154         (1ULL << 16)  /**< AT+ZIGSNIFF (IEEE 802.15.4) */
/* bits 17-63: reserved */

/* ── This firmware's capability bitmap ──────────────────────────────────── */
/*
 * Set this to the OR of every M1_ESP32_CAP_* bit that this firmware supports.
 * The M1 host reads this value and uses it to gate feature availability.
 *
 * neddy299 fork capabilities:
 *   - WiFi join/disconnect:  AT+CWJAP / AT+CWQAP  (standard AT-C6)
 *   - Station scanning:      AT+STASCAN           (custom)
 *   - Deauth attack:         AT+DEAUTH            (custom)
 *   - BLE HID keyboard:      AT+BLEHIDINIT / AT+BLEHIDKB (custom)
 *   - IEEE 802.15.4 sniffer: AT+ZIGSNIFF          (custom)
 */
#define M1_ESP32_THIS_FW_CAP_BITMAP \
    ( M1_ESP32_CAP_WIFI_JOIN  \
    | M1_ESP32_CAP_STA_SCAN   \
    | M1_ESP32_CAP_DEAUTH     \
    | M1_ESP32_CAP_BLE_HID    \
    | M1_ESP32_CAP_802154     )

/** Short identifier string written into the fw_name field. */
#define M1_ESP32_THIS_FW_NAME  "AT-neddy299-1.0.1"

/* ── Response payload struct ─────────────────────────────────────────────── */

/**
 * 41-byte packed response to opcode M1_CMD_GET_STATUS_OPCODE.
 *
 * Must be packed so the host can read fields by offset without padding:
 *   offset 0:    proto_ver  (1 byte)
 *   offset 1..8: cap_bitmap (8 bytes, little-endian uint64_t)
 *   offset 9..40: fw_name   (32 bytes, null-terminated, zero-padded)
 */
typedef struct __attribute__((packed)) {
    uint8_t  proto_ver;     /**< Protocol version — must be M1_ESP32_STATUS_PROTO_VER */
    uint64_t cap_bitmap;    /**< Capability bits — OR of M1_ESP32_CAP_* values */
    char     fw_name[32];   /**< Firmware identifier string, null-terminated */
} m1_esp32_status_payload_t;

/* Compile-time guard: the struct must be exactly 41 bytes. */
_Static_assert(sizeof(m1_esp32_status_payload_t) == M1_ESP32_STATUS_PAYLOAD_SIZE,
               "m1_esp32_status_payload_t size mismatch");

/* ── Helper ──────────────────────────────────────────────────────────────── */

/**
 * Populate @p out with the capability descriptor for this firmware.
 *
 * Call this when a received SPI buffer starts with M1_CMD_GET_STATUS_OPCODE,
 * then transmit the resulting 41 bytes back to the master in place of the
 * normal AT text response.
 *
 * @param out  Pointer to caller-allocated m1_esp32_status_payload_t.
 */
static inline void at_m1_status_build_payload(m1_esp32_status_payload_t *out)
{
    out->proto_ver  = M1_ESP32_STATUS_PROTO_VER;
    out->cap_bitmap = M1_ESP32_THIS_FW_CAP_BITMAP;
    memset(out->fw_name, 0, sizeof(out->fw_name));
    strncpy(out->fw_name, M1_ESP32_THIS_FW_NAME, sizeof(out->fw_name) - 1);
}

#endif /* AT_M1_STATUS_H */
