idf_component_register(
    SRCS "src/wifi_web.c"
    INCLUDE_DIRS "include"
    REQUIRES config nvs_flash esp_http_server esp_netif esp_wifi esp_event spiffs json temp_sensor relay
)

# Создаем SPIFFS образ с веб-файлами из каталога data (PlatformIO автоматически создаст образ)
# FLASH_IN_PROJECT гарантирует, что образ будет включен в прошивку (включая тесты)
# PlatformIO создаст образ из каталога data/ в корне проекта
# Для ESP-IDF используем путь к data относительно корня проекта
get_filename_component(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(PROJECT_DIR ${PROJECT_DIR} DIRECTORY)
spiffs_create_partition_image(web_storage ${PROJECT_DIR}/data FLASH_IN_PROJECT)
