idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES freertos config wifi_web
)