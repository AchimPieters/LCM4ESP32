#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs.h"
#include "ota.h"

// the first function is the ONLY thing needed for a repo to support ota after having started with ota-boot
// in ota-boot the user gets to set the wifi and the repository details and it then installs the ota-main binary

extern nvs_handle_t lcm_handle;
static const char *TAG = "ota-api";

// Trigger the OTA process and restart into the temporary image
void ota_update(void *arg) {  //arg not used
    ota_temp_boot();
    esp_restart();
}

// Read OTA related parameters from NVS and compute HomeKit configuration hash
unsigned int  ota_read_sysparam(char **manufacturer, char **serial,
                                char **model, char **revision)
{
    if (!manufacturer || !serial || !model || !revision) {
        ESP_LOGE(TAG, "invalid argument");
        return 0;
    }

    size_t size;
    char *value;

    esp_err_t err = nvs_get_str(lcm_handle, "ota_repo", NULL, &size);
    if (err == ESP_OK) {
        value = malloc(size);
        if (!value) {
            ESP_LOGE(TAG, "no memory for repo");
            return 0;
        }
        err = nvs_get_str(lcm_handle, "ota_repo", value, &size);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "nvs get repo failed: %s", esp_err_to_name(err));
            free(value);
            return 0;
        }
        char *slash = strchr(value, '/');
        if (slash) {
            *slash = '\0';
        }
        *manufacturer = value;
        *model = slash ? slash + 1 : value + strlen(value);
    } else {
        *manufacturer = "manuf_unknown";
        *model = "model_unknown";
    }

    err = nvs_get_str(lcm_handle, "ota_version", NULL, &size);
    if (err == ESP_OK) {
        value = malloc(size);
        if (!value) {
            ESP_LOGE(TAG, "no memory for version");
            return 0;
        }
        err = nvs_get_str(lcm_handle, "ota_version", value, &size);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "nvs get version failed: %s", esp_err_to_name(err));
            free(value);
            return 0;
        }
        *revision = value;
    } else {
        *revision = "0.0.0";
    }

    uint8_t mac_addr[6];
    esp_read_mac(mac_addr, ESP_MAC_WIFI_STA);
    *serial = malloc(18);
    if (!*serial) {
        ESP_LOGE(TAG, "No memory for serial");
        return 0;
    }
    snprintf(*serial, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    unsigned int c_hash = 0;
    char version[16];
    char *rev = version;
    char *dot;
    strncpy(rev, *revision, sizeof(version));
    if ((dot = strchr(rev, '.'))) {
        *dot = 0;
        c_hash = atoi(rev);
        rev = dot + 1;
    }
    if ((dot = strchr(rev, '.'))) {
        *dot = 0;
        c_hash = c_hash * 1000 + atoi(rev);
        rev = dot + 1;
    }
    c_hash = c_hash * 1000 + atoi(rev);
    ESP_LOGI(TAG, "manuf='%s' serial='%s' model='%s' revision='%s' c#=%d", *manufacturer, *serial, *model, *revision, c_hash);
    return c_hash;
}




#include <homekit/characteristics.h>

static esp_timer_handle_t update_timer;
static void update_timer_cb(void* arg) {
    ota_update(arg);
}

void ota_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        ESP_LOGW(TAG, "Invalid ota-value format: %d", value.format);
        return;
    }
    if (value.bool_value) {
        // Provide user feedback via identify routine and delay reboot
        if (!update_timer) {
            const esp_timer_create_args_t args = {
                .callback = &update_timer_cb,
                .name = "ota_update"
            };
            esp_timer_create(&args, &update_timer);
        }
        esp_timer_start_once(update_timer, 500000); //500ms
    }
}
