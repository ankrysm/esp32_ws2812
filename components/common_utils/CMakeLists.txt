idf_component_register(SRCS "common_util.c"
					REQUIRES "app_update"
                    INCLUDE_DIRS ".")
component_compile_options(-Wno-error=format= -Wno-format)