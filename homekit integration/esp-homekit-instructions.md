### This assumes you are using [Achim Pieters' esp32-homekit](https://github.com/AchimPieters/esp32-homekit) library for HomeKit support

Below is a short walk‑through to add OTA support using the OTA API in this repository.

1. **Add the HomeKit component**
   Create a file named `idf_component.yml` next to your `main.c` and list the HomeKit library as a dependency:
```yaml
dependencies:
  idf: ">=5.0"
  achimpieters/esp32-homekit: "1.*"
```

2. **Copy the OTA API files**
   Place `ota-api.c` and `ota-api.h` from the `homekit integration` folder into the same directory as `main.c`.

3. **Register the files with CMake**
   Add them to the source list in your `CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "main.c" "ota-api.c" ...
    INCLUDE_DIRS "."
    REQUIRES esp32-homekit
)
```

4. **Update your `main.c`**
   Insert the code below after the `#include` section. These characteristics expose the OTA trigger and transfer device information to HomeKit.
```
// add this section to make your device OTA capable
// create the extra characteristic &ota_trigger, at the end of the primary service (before the NULL)
// it can be used in Eve, which will show it, where Home does not
// and apply the four other parameters in the accessories_information section

#include "ota-api.h"
homekit_characteristic_t ota_trigger  = API_OTA_TRIGGER;
homekit_characteristic_t manufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER,  "X");
homekit_characteristic_t serial       = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, "1");
homekit_characteristic_t model        = HOMEKIT_CHARACTERISTIC_(MODEL,         "Z");
homekit_characteristic_t revision     = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION,  "0.0.0");

// next use these two lines before calling homekit_server_init(&config);
//     int c_hash=ota_read_sysparam(&manufacturer.value.string_value,
//                                  &serial.value.string_value,
//                                  &model.value.string_value,
//                                  &revision.value.string_value);
//     config.accessories[0]->config_number=c_hash;
// end of OTA add-in instructions
```
###  for example it could end up like this:

##### !! Be aware that if in the future you change the id's of the characteristics (not only ota_trigger) your device might be rejected by the iPhone and it has proven impossible safe re-flash and re-pair to get the device back into action!! So, if after this you decide to add even more, add it behind what you already had.

```
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(
        .id=1,
        .category=homekit_accessory_category_window_covering,
        .services=(homekit_service_t*[]){
            HOMEKIT_SERVICE(ACCESSORY_INFORMATION,
                .characteristics=(homekit_characteristic_t*[]){
                    HOMEKIT_CHARACTERISTIC(NAME, "basic-curtain"),
                    &manufacturer,
                    &serial,
                    &model,
                    &revision,
                    HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
                    NULL
                }),
            HOMEKIT_SERVICE(WINDOW_COVERING, .primary=true,
                .characteristics=(homekit_characteristic_t*[]){
                    HOMEKIT_CHARACTERISTIC(NAME, "Curtain"),
                    &target,
                    &current,
                    &state,
                    &ota_trigger,
                    NULL
                }),
            NULL
        }),
    NULL
};

homekit_server_config_t config = {.accessories = accessories, .password = "111-11-111"};

void on_wifi_ready() {
    device_init();
    
    int c_hash=ota_read_sysparam(&manufacturer.value.string_value,&serial.value.string_value,
                                      &model.value.string_value,&revision.value.string_value);
    //c_hash=1; revision.value.string_value="0.0.1"; //cheat line
    config.accessories[0]->config_number=c_hash;
    
    homekit_server_init(&config);
}
```
5. **Build and flash**
   Compile the project and upload it to your board:
```bash
idf.py build flash
```

6. **Pair and update**
   Pair the device with the Home or Eve app. In the Eve app you will see a
   _FirmwareUpdate_ switch. Toggle it whenever a newer version exists in the
   configured repository.

The cheat line can be used in the initial stage before LCM is used at all since
without it HomeKit will not work.
