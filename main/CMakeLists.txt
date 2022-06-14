#set(COMPONENT_SRCS "main.cpp" "handle_fs.cpp" "rest_server.c")
#set(COMPONENT_ADD_INCLUDEDIRS "")

idf_component_register(SRCS "handle_fs.c" "main.c" "rest_server.c" "led_strip.c" "config.c"
                    INCLUDE_DIRS ".")

#register_component()
#"rest_server.c"