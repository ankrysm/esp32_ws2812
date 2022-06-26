#set(COMPONENT_SRCS "main.cpp" "handle_fs.cpp" "rest_server.c")
#set(COMPONENT_ADD_INCLUDEDIRS "")

idf_component_register(SRCS
					"main.c" "rest_server.c" "led_strip.c" "config.c"
					"wifi_config.c" "timer_events.c" "color.c" "create_events.c"
					"create_demo.c"
                    INCLUDE_DIRS ".")

#register_component()
#"rest_server.c" "handle_fs.c"