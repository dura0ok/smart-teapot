#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "config.h"
#include "wifi_web.h"

static const char *TAG = "MAIN";
static wifi_web_ctx_t wifi_web_ctx;

void app_main(void) {
    ESP_LOGI(TAG, "=== Smart Teapot Project ===");
    
    teapot_config_t config;
    esp_err_t ret = config_init_from_generated(&config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load from generated config, using defaults");
        config_init_default(&config);
    }
    
    ESP_LOGI(TAG, "Configuration:");
    ESP_LOGI(TAG, "  Temp sensor GPIO: %d", config.gpio.temp_sensor_gpio);
    ESP_LOGI(TAG, "  Relay GPIO: %d", config.gpio.relay_gpio);
    ESP_LOGI(TAG, "  Default setpoint: %.1f°C", config.default_setpoint);
    ESP_LOGI(TAG, "  WiFi SSID: %s", config.wifi.ssid);
    ESP_LOGI(TAG, "  WiFi Password: %s", strlen(config.wifi.password) > 0 ? "***" : "(empty)");
    
    ESP_ERROR_CHECK(wifi_web_init(&wifi_web_ctx, &config));
    
    ESP_LOGI(TAG, "Smart Teapot initialized successfully");
    
    vTaskSuspend(NULL);
}