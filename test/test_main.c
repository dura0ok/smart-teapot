#include <unity.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

extern void run_config_tests(void);
extern void run_wifi_web_tests(void);

void setUp(void) {
    // Set httpd log level to WARN to suppress "error in listen" messages
    // This is expected when port is already in use (race condition in tests)
    esp_log_level_set("httpd", ESP_LOG_WARN);
}

void tearDown(void) {
}

static void run_all_tests(void) {
    UNITY_BEGIN();
    
    run_config_tests();
    run_wifi_web_tests();
    
    UNITY_END();
}

void app_main(void) {
    run_all_tests();
}
