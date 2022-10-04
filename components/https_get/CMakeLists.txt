idf_component_register(SRCS "https_get.c"
                    PRIV_REQUIRES "esp_wifi" "esp_timer" "nvs_flash" "esp-tls" "esp_http_client"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES  server_root_cert.pem 
                    )
component_compile_options(-Wno-error=format= -Wno-format)