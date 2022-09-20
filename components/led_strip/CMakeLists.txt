idf_component_register(SRCS "led_strip_util.c" "led_strip_encoder.c"
                    PRIV_REQUIRES "driver" "common_utils"
                    INCLUDE_DIRS ".")
component_compile_options(-Wno-error=format= -Wno-format)