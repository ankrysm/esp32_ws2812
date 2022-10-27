idf_component_register(SRCS "bmp.c"
                    PRIV_REQUIRES "https_get" "common_utils"
                    INCLUDE_DIRS ".")
component_compile_options(-Wno-error=format= -Wno-format)