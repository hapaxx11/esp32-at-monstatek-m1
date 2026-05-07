/*
 * at_custom_cmd_get_status.h — M1 Status Query AT Command
 */

#ifndef AT_CUSTOM_CMD_GET_STATUS_H
#define AT_CUSTOM_CMD_GET_STATUS_H

#include <stdbool.h>

/**
 * Register the M1 status AT command:
 *   AT+GETSTATUSHEX    Query this firmware's capability payload as hex
 *   AT+GETSTATUSHEX?   Query form (same response)
 *
 * Response format:
 *   +GETSTATUSHEX:<82-hex-char-payload>
 *
 * Payload bytes are m1_esp32_status_payload_t:
 *   [0]      proto_ver (M1_ESP32_CAPS_PROTO_VER)
 *   [1..8]   cap_bitmap (uint64_t, little-endian, M1_ESP32_CAP_* bits)
 *   [9..40]  fw_name[32] (null-terminated firmware identifier)
 *
 * This allows the M1 host to discover the AT firmware's actual capability
 * set using the same wire payload shape consumed by CMD_GET_STATUS parsing.
 */
bool esp_at_custom_cmd_get_status_register(void);

#endif /* AT_CUSTOM_CMD_GET_STATUS_H */
