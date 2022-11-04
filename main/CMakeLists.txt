#set(COMPONENT_SRCS "main.cpp" "handle_fs.cpp" "rest_server.c")
#set(COMPONENT_ADD_INCLUDEDIRS "")

set(COMPONENT_SRCS
					"main.c" "rest_server.c" "web_server.c" "led_strip.c" "config.c"
					"wifi_config.c" "timer_events.c" "process_events.c" "color.c" "create_events.c"
					"create_demo.c" "event_util.c" "global_data.c"
					"json_util.c" "create_config.c" "process_bmp.c" "process_objects.c"
					"create_objects.c" "decode_json.c" "create_tracks.c"
					#"location_based_events.c" "move_events.c"
					#"paint_pixel.c"
					)
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_EMBED_FILES "favicon.ico" "upload_script.html" "ledstrip.png")


register_component()
#"rest_server.c" "handle_fs.c"

#component_compile_options(-Wno-error=format= -Wno-format)
#component_compile_options(-Wno-error=format=)
component_compile_options(-Wno-format)
