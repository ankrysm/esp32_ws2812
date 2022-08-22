#set(COMPONENT_SRCS "main.cpp" "handle_fs.cpp" "rest_server.c")
#set(COMPONENT_ADD_INCLUDEDIRS "")

idf_component_register(SRCS
					"main.c" "rest_server.c" "led_strip.c" "config.c"
					"wifi_config.c" "timer_events.c" "process_events.c" "color.c" "create_events.c"
					"create_demo.c" "event_util.c"
					#"location_based_events.c" "move_events.c"
					#"paint_pixel.c"
                    INCLUDE_DIRS ".")

#register_component()
#"rest_server.c" "handle_fs.c"

component_compile_options(-Wno-error=format= -Wno-format)
