
set(COMPONENT_SRCS
					"main.c" "rest_server.c" "web_server.c" "led_strip.c" "config.c"
					"wifi_config.c" "timer_events.c" "process_events.c" "color.c" "create_events.c"
					"event_util.c" "global_data.c"
					"json_util.c" "create_config.c" "process_bmp.c" "process_objects.c"
					"create_objects.c" "decode_json.c" "create_tracks.c" "logger.c"
					)
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_EMBED_FILES "webcontent/favicon.ico" "webcontent/upload_script.html" "webcontent/ledstrip.jpg"
		"webcontent/example.json" "webcontent/example_config.json" "webcontent/stylesheet.css"
		"webcontent/main_page.html" "webcontent/main_content.html" "webcontent/settings_content.html"
		"webcontent/files_content.html" "webcontent/help_content.html" )


register_component()

#component_compile_options(-Wno-error=format= -Wno-format)
#component_compile_options(-Wno-error=format=)
component_compile_options(-Wno-format)
