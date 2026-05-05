/*
 * at_custom_cmd_get_status.h — M1 Status Query AT Command
 */

#ifndef AT_CUSTOM_CMD_GET_STATUS_H
#define AT_CUSTOM_CMD_GET_STATUS_H

#include <stdbool.h>

/**
 * Register the M1 status AT command:
 *   AT+M1STATUS?   Query this firmware's capability bitmaps
 *
 * Response format:
 *   +M1STATUS:<proto_ver>,<at_cmd_bitmap_hex>,<ext_bitmap_hex>,<fw_name>
 *
 *   proto_ver       — protocol version (matches M1_ESP32_CAPS_PROTO_VER)
 *   at_cmd_bitmap   — 64-bit hex bitmask of supported standard AT commands
 *                     (M1_AT_CMD_* bits from m1_protocol.h)
 *   ext_bitmap      — 32-bit hex bitmask of supported extension AT commands
 *                     (M1_EXT_CMD_* bits from m1_protocol.h)
 *   fw_name         — firmware identifier string
 *
 * This allows the M1 host to discover the AT firmware's actual capability
 * set after falling back from binary-protocol detection (CMD_GET_STATUS
 * timeout). The bitmask values use the same M1_AT_CMD_* / M1_EXT_CMD_*
 * definitions shared with the binary SiN360 firmware.
 */
bool esp_at_custom_cmd_get_status_register(void);

#endif /* AT_CUSTOM_CMD_GET_STATUS_H */
