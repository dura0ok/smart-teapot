#include "config.h"
#include "config_autogen.h"
#include <string.h>

esp_err_t config_init_default(teapot_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(config->wifi.ssid, "SmartTeapot", CONFIG_WIFI_SSID_MAX_LEN);
    config->wifi.ssid[CONFIG_WIFI_SSID_MAX_LEN] = '\0';
    strncpy(config->wifi.password, "", CONFIG_WIFI_PASSWORD_MAX_LEN);
    config->wifi.password[CONFIG_WIFI_PASSWORD_MAX_LEN] = '\0';

    config->gpio.relay_gpio = 4;
    config->gpio.temp_sensor_gpio = 5;

    config->default_setpoint = 85.0f;

    return ESP_OK;
}

esp_err_t config_init_from_generated(teapot_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = config_init_default(config);
    if (ret != ESP_OK) {
        return ret;
    }

    strncpy(config->wifi.ssid, WIFI_SSID, CONFIG_WIFI_SSID_MAX_LEN);
    config->wifi.ssid[CONFIG_WIFI_SSID_MAX_LEN] = '\0';

    strncpy(config->wifi.password, WIFI_PASSWORD, CONFIG_WIFI_PASSWORD_MAX_LEN);
    config->wifi.password[CONFIG_WIFI_PASSWORD_MAX_LEN] = '\0';

    config->gpio.relay_gpio = RELAY_GPIO;
    config->gpio.temp_sensor_gpio = TEMP_SENSOR_GPIO;
    config->default_setpoint = DEFAULT_SETPOINT;

    return ESP_OK;
}

esp_err_t config_validate(const teapot_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(config->wifi.ssid) == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (strlen(config->wifi.ssid) > CONFIG_WIFI_SSID_MAX_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (strlen(config->wifi.password) > CONFIG_WIFI_PASSWORD_MAX_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }
    // If password is provided, it must be at least 8 characters (WPA2 requirement)
    if (strlen(config->wifi.password) > 0 && strlen(config->wifi.password) < CONFIG_WIFI_PASSWORD_MIN_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (config->gpio.relay_gpio < CONFIG_GPIO_MIN || config->gpio.relay_gpio > CONFIG_GPIO_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->gpio.temp_sensor_gpio < CONFIG_GPIO_MIN || config->gpio.temp_sensor_gpio > CONFIG_GPIO_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->gpio.relay_gpio == config->gpio.temp_sensor_gpio) {
        return ESP_ERR_INVALID_STATE;
    }

    if (config->default_setpoint < CONFIG_TEMP_MIN || config->default_setpoint > CONFIG_TEMP_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

esp_err_t config_set_wifi_ssid(teapot_config_t *config, const char *ssid) {
    if (config == NULL || ssid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t len = strlen(ssid);
    if (len == 0 || len > CONFIG_WIFI_SSID_MAX_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }

    strncpy(config->wifi.ssid, ssid, CONFIG_WIFI_SSID_MAX_LEN);
    config->wifi.ssid[CONFIG_WIFI_SSID_MAX_LEN] = '\0';

    return ESP_OK;
}

esp_err_t config_set_wifi_password(teapot_config_t *config, const char *password) {
    if (config == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t len = strlen(password);
    if (len > CONFIG_WIFI_PASSWORD_MAX_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }

    strncpy(config->wifi.password, password, CONFIG_WIFI_PASSWORD_MAX_LEN);
    config->wifi.password[CONFIG_WIFI_PASSWORD_MAX_LEN] = '\0';

    return ESP_OK;
}

esp_err_t config_set_relay_gpio(teapot_config_t *config, int gpio) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (gpio < CONFIG_GPIO_MIN || gpio > CONFIG_GPIO_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (gpio == config->gpio.temp_sensor_gpio) {
        return ESP_ERR_INVALID_STATE;
    }

    config->gpio.relay_gpio = gpio;

    return ESP_OK;
}

esp_err_t config_set_temp_sensor_gpio(teapot_config_t *config, int gpio) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (gpio < CONFIG_GPIO_MIN || gpio > CONFIG_GPIO_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (gpio == config->gpio.relay_gpio) {
        return ESP_ERR_INVALID_STATE;
    }

    config->gpio.temp_sensor_gpio = gpio;

    return ESP_OK;
}

esp_err_t config_set_default_setpoint(teapot_config_t *config, float setpoint) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (setpoint < CONFIG_TEMP_MIN || setpoint > CONFIG_TEMP_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    config->default_setpoint = setpoint;

    return ESP_OK;
}