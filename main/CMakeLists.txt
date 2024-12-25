idf_component_register(SRCS
    "models/settings.cpp"
    "models/info.cpp"
    "version.cpp"
    "miscellaneous.cpp"
    "udp_server.cpp"
    "main.cpp"
    "wifi.cpp"
    "udp_server.cpp"
    INCLUDE_DIRS "." "${CMAKE_CURRENT_BINARY_DIR}/")

find_package(Git REQUIRED)
execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD OUTPUT_VARIABLE COMMIT_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
string(TIMESTAMP TIMESTAMP_NOW "%Y-%m-%d %H:%M:%S")

configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h @ONLY)