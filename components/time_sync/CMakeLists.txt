idf_component_register(SRCS "time_sync.c"
                    PRIV_REQUIRES "esp_wifi" "esp_timer" "nvs_flash"
                    INCLUDE_DIRS ".")
component_compile_options(-Wno-error=format= -Wno-format)