/*
 * at_custom_cmd_get_status.c — M1 Status Query AT Command
 *
 * Implements:
 *   - AT+GETSTATUSHEX (new protocol response, hex-encoded raw payload)
 *
 * Detection flow (host side):
 *   1. Host sends CMD_GET_STATUS on the binary SPI channel.
 *   2. Binary firmware responds with m1_resp_t → binary mode.
 *   3. AT firmware: timeout (different SPI protocol) → host falls back to
 *      AT text probing.
 *   4. Host issues AT+GETSTATUSHEX to discover this AT firmware's capability
 *      set in the same payload shape used by CMD_GET_STATUS.
 *
 * Response format:
 *   +GETSTATUSHEX:<82-hex-char-payload>
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "esp_at.h"
#include "esp_log.h"

#include "m1_protocol.h"
#include "at_custom_cmd_get_status.h"

#define TAG "getstatushex"

static bool encode_bytes_to_hex(const uint8_t *in, size_t len, char *out, size_t out_size)
{
    static const char hexdigits[] = "0123456789ABCDEF";
    size_t i;

    if ((len * 2u) + 1u > out_size) {
        return false;
    }

    for (i = 0; i < len; ++i) {
        out[(i * 2u)]     = hexdigits[(in[i] >> 4) & 0x0F];
        out[(i * 2u) + 1] = hexdigits[in[i] & 0x0F];
    }
    out[len * 2u] = '\0';
    return true;
}

/* AT+GETSTATUSHEX / AT+GETSTATUSHEX? — return hex-encoded raw m1_esp32_status_payload_t */
static uint8_t at_cmd_getstatushex(uint8_t *cmd_name)
{
    m1_esp32_status_payload_t payload = {0};
    char hex[(sizeof(payload) * 2u) + 1u];
    char buf[120];
    int n;

    (void)cmd_name;

    payload.proto_ver = M1_ESP32_CAPS_PROTO_VER;
    payload.cap_bitmap = M1_ESP32_CAP_PROFILE_ESP32AT;
    strncpy(payload.fw_name, M1_FW_NAME_ESP32AT, sizeof(payload.fw_name) - 1u);
    payload.fw_name[sizeof(payload.fw_name) - 1u] = '\0';

    if (!encode_bytes_to_hex((const uint8_t *)&payload, sizeof(payload), hex, sizeof(hex))) {
        return ESP_AT_RESULT_CODE_ERROR;
    }
    n = snprintf(buf, sizeof(buf), "+GETSTATUSHEX:%s\r\n", hex);
    if (n < 0 || (size_t)n >= sizeof(buf)) {
        return ESP_AT_RESULT_CODE_ERROR;
    }
    esp_at_port_write_data((uint8_t *)buf, (size_t)n);
    return ESP_AT_RESULT_CODE_OK;
}

static const esp_at_cmd_struct s_m1status_cmd_list[] = {
    {"+GETSTATUSHEX", NULL, at_cmd_getstatushex, NULL, at_cmd_getstatushex},
};

bool esp_at_custom_cmd_get_status_register(void)
{
    return esp_at_custom_cmd_array_regist(
        s_m1status_cmd_list,
        sizeof(s_m1status_cmd_list) / sizeof(s_m1status_cmd_list[0]));
}
