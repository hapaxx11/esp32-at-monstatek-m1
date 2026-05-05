/*
 * at_custom_cmd_get_status.c — M1 Status Query AT Command
 *
 * Implements AT+M1STATUS? which returns this firmware's capability bitmaps
 * in the same format as CMD_GET_STATUS in the binary SPI protocol defined
 * in m1_protocol.h.
 *
 * Detection flow (host side):
 *   1. Host sends CMD_GET_STATUS on the binary SPI channel.
 *   2. Binary firmware responds with m1_resp_t → binary mode.
 *   3. AT firmware: timeout (different SPI protocol) → host falls back to
 *      M1_AT_CMD_PROFILE_DEFAULT with ext_bitmap = 0.
 *   4. Host issues AT+M1STATUS? to discover the AT firmware's actual
 *      capability set, which may exceed M1_AT_CMD_PROFILE_DEFAULT.
 *
 * Response format:
 *   +M1STATUS:<proto_ver>,<at_cmd_bitmap_hex>,<ext_bitmap_hex>,<fw_name>
 */

#include <stdio.h>
#include <string.h>

#include "esp_at.h"
#include "esp_log.h"

#include "m1_protocol.h"
#include "at_custom_cmd_get_status.h"

#define TAG "m1status"

/* AT+M1STATUS? — return capability bitmaps */
static uint8_t at_query_cmd_m1status(uint8_t *cmd_name)
{
    (void)cmd_name;

    char buf[96];   /* "+M1STATUS:" (10) + proto_ver (3) + "," (1) + at_bitmap hex (16) +
                     * "," (1) + ext_bitmap hex (8) + "," (1) + fw_name (≤31) +
                     * "\r\n" (2) + NUL (1) → max ~73 bytes; 96 gives headroom */
    int n = snprintf(buf, sizeof(buf),
                     "+M1STATUS:%u,%016llX,%08lX,%s\r\n",
                     (unsigned)M1_ESP32_CAPS_PROTO_VER,
                     (unsigned long long)M1_AT_CMD_PROFILE_ESP32AT,
                     (unsigned long)M1_EXT_CMD_PROFILE_ESP32AT,
                     M1_FW_NAME_ESP32AT);
    esp_at_port_write_data((uint8_t *)buf, (size_t)n);
    return ESP_AT_RESULT_CODE_OK;
}

static const esp_at_cmd_struct s_m1status_cmd_list[] = {
    {"+M1STATUS", NULL, at_query_cmd_m1status, NULL, NULL},
};

bool esp_at_custom_cmd_get_status_register(void)
{
    return esp_at_custom_cmd_array_regist(
        s_m1status_cmd_list,
        sizeof(s_m1status_cmd_list) / sizeof(s_m1status_cmd_list[0]));
}
