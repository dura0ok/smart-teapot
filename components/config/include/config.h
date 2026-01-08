#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_WIFI_SSID_MAX_LEN 32
#define CONFIG_WIFI_PASSWORD_MAX_LEN 64
#define CONFIG_WIFI_PASSWORD_MIN_LEN 8
#define CONFIG_GPIO_MIN 0
#define CONFIG_GPIO_MAX 21
#define CONFIG_TEMP_MIN 0.0f
#define CONFIG_TEMP_MAX 100.0f

typedef struct {
    char ssid[CONFIG_WIFI_SSID_MAX_LEN + 1];
    char password[CONFIG_WIFI_PASSWORD_MAX_LEN + 1];
} teapot_wifi_config_t;

typedef struct {
    int relay_gpio;
    int temp_sensor_gpio;
} teapot_gpio_config_t;

typedef struct {
    teapot_wifi_config_t wifi;
    teapot_gpio_config_t gpio;
    float default_setpoint;
} teapot_config_t;

esp_err_t config_init_default(teapot_config_t *config);
esp_err_t config_init_from_generated(teapot_config_t *config);
esp_err_t config_validate(const teapot_config_t *config);
esp_err_t config_set_wifi_ssid(teapot_config_t *config, const char *ssid);
esp_err_t config_set_wifi_password(teapot_config_t *config, const char *password);
esp_err_t config_set_relay_gpio(teapot_config_t *config, int gpio);
esp_err_t config_set_temp_sensor_gpio(teapot_config_t *config, int gpio);
esp_err_t config_set_default_setpoint(teapot_config_t *config, float setpoint);

#ifdef __cplusplus
}
#endif