# Embed the server root certificate into the final binary
idf_build_get_property(project_dir PROJECT_DIR)
idf_component_register(
    SRCS "main.c" "ota.c" "wifi_config.c" "form_urlencoded.c" "udplogger.c" "../homekit integration/ota-api.c"
    INCLUDE_DIRS "." "../homekit integration"
    REQUIRES homekit esp_timer app_update esp_app_format nvs_flash
    PRIV_REQUIRES esp_wifi esp_netif esp_https_server esp_http_server esp_http_client esp_driver_gpio
)
