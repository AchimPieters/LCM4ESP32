### Using OTA with ESP32-HomeKit
This guide shows how to integrate the OTA API from this repository with the [Achim Pieters' esp32-homekit](https://github.com/AchimPieters/esp32-homekit) library.

1. **Add dependencies**
   Create an `idf_component.yml` next to `main.c`:
```yaml
dependencies:
  idf: ">=5.3"
  achimpieters/esp32-homekit: "~1.4.1"
```

2. **Copy OTA sources**
   Place `ota-api.c` and `ota-api.h` from the `homekit integration` folder next to `main.c`.

3. **Register sources**
   Add them to `CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "main.c" "ota-api.c"
    INCLUDE_DIRS "."
    REQUIRES esp32-homekit
)
```

4. **Update `main.c`**
   Include the header and declare the characteristics:
```c
#include "ota-api.h"

homekit_characteristic_t ota_trigger  = API_OTA_TRIGGER;
homekit_characteristic_t manufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER, "X");
homekit_characteristic_t serial       = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, "1");
homekit_characteristic_t model        = HOMEKIT_CHARACTERISTIC_(MODEL, "Z");
homekit_characteristic_t revision     = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION, "0.0.0");
```
   Before starting the HomeKit server:
```c
int c_hash = ota_read_sysparam(&manufacturer.value.string_value,
                               &serial.value.string_value,
                               &model.value.string_value,
                               &revision.value.string_value);
config.accessories[0]->config_number = c_hash;
```

5. **Build and flash**
   Run `idf.py build flash` and pair the device. In the Eve app, toggle the _FirmwareUpdate_ switch whenever a newer version is available.


