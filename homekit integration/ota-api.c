#include <stdlib.h>  //for printf
#include <stdio.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs.h"
#include "ota.h"

// the first function is the ONLY thing needed for a repo to support ota after having started with ota-boot
// in ota-boot the user gets to set the wifi and the repository details and it then installs the ota-main binary

extern nvs_handle_t lcm_handle;

void ota_update(void *arg) {  //arg not used
    ota_temp_boot();
    esp_restart();
}

// this function is optional to couple Homekit parameters to the sysparam variables and github parameters
unsigned int  ota_read_sysparam(char **manufacturer,char **serial,char **model,char **revision) {
    esp_err_t err;
    size_t size;
    char *value;

    if (nvs_get_str(lcm_handle,"ota_repo", NULL, &size)==ESP_OK) {
        value = malloc(size);
        if (!value) return 0;
        nvs_get_str(lcm_handle,"ota_repo", value, &size);
        char *slash = strchr(value,'/');
        if (slash) *slash = '\0';
        *manufacturer = value;
        *model = slash ? slash + 1 : value + strlen(value);
    } else {
        *manufacturer = "manuf_unknown";
        *model = "model_unknown";
    }
    if (nvs_get_str(lcm_handle,"ota_version", NULL, &size)==ESP_OK) {
        value = malloc(size);
        if (!value) return 0;
        nvs_get_str(lcm_handle,"ota_version", value, &size);
        *revision = value;
    } else *revision = "0.0.0";

    uint8_t macaddr[6];
    esp_wifi_get_mac(WIFI_IF_STA, macaddr);
    *serial=malloc(18);
    sprintf(*serial,"%02X:%02X:%02X:%02X:%02X:%02X",macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);

    unsigned int c_hash=0;
    char version[16];
    char* rev=version;
    char* dot;
    strncpy(rev,*revision,16);
    if ((dot=strchr(rev,'.'))) {dot[0]=0; c_hash=            atoi(rev); rev=dot+1;}
    if ((dot=strchr(rev,'.'))) {dot[0]=0; c_hash=c_hash*1000+atoi(rev); rev=dot+1;}
                                          c_hash=c_hash*1000+atoi(rev);
                                            //c_hash=c_hash*10  +configuration_variant; //possible future extension
    printf("manuf=\'%s\' serial=\'%s\' model=\'%s\' revision=\'%s\' c#=%d\n",*manufacturer,*serial,*model,*revision,c_hash);
    return c_hash;
}




#include <homekit/characteristics.h>

static esp_timer_handle_t update_timer;
static void update_timer_cb(void* arg) {
    ota_update(arg);
}

void ota_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid ota-value format: %d\n", value.format);
        return;
    }
    if (value.bool_value) {
        //make a distinct light pattern or other feedback to the user = call identify routine
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
