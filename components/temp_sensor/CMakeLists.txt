idf_component_register(
    SRCS "src/temp_sensor.c"
    INCLUDE_DIRS "include"
    REQUIRES config onewire_bus driver
    PRIV_REQUIRES esp_ds18b20
)

