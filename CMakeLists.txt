# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include(FetchContent)

FetchContent_Declare(
	platform
	#SOURCE_DIR ../../esp-protocols
	GIT_REPOSITORY https://github.com/espressif/esp-protocols.git
	GIT_TAG origin/master
)

FetchContent_GetProperties(platform)
if (NOT platform_POPULATED)
	FetchContent_Populate(platform)
endif()

# This example uses an extra component for common functions such as Wi-Fi and Ethernet connection.
#set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common
	#$ENV{IDF_PATH}/examples/common_components/led_strip)
	# $ENV{IDF_PATH}/examples/components/esp_http_server
#)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_ws2812-Application)
