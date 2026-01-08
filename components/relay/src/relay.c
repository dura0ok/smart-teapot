#include "relay.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"
#include <string.h>

static const char *TAG = "RELAY";

#define RELAY_ON  0
#define RELAY_OFF 1

struct relay_t {
    gpio_num_t gpio;
    bool current_state;
};

static struct relay_t g_relay = {0};

esp_err_t relay_init(const teapot_config_t *config, relay_handle_t *handle) {
    if (config == NULL || handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->gpio.relay_gpio < CONFIG_GPIO_MIN || config->gpio.relay_gpio > CONFIG_GPIO_MAX) {
        ESP_LOGE(TAG, "Invalid relay GPIO: %d", config->gpio.relay_gpio);
        return ESP_ERR_INVALID_ARG;
    }
    
    g_relay.gpio = (gpio_num_t)config->gpio.relay_gpio;
    g_relay.current_state = false;
    
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << g_relay.gpio,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %d: %s", g_relay.gpio, esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Relay initialized on GPIO %d", g_relay.gpio);
    *handle = &g_relay;
    return ESP_OK;
}

esp_err_t relay_deinit(relay_handle_t handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Relay deinitialized on GPIO %d", handle->gpio);
    memset(&g_relay, 0, sizeof(g_relay));
    return ESP_OK;
}

esp_err_t relay_on(relay_handle_t handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // gpio_set_level(handle->gpio, RELAY_ON);
    handle->current_state = true;
    ESP_LOGI(TAG, "Relay ON (GPIO %d)", handle->gpio);
    return ESP_OK;
}

esp_err_t relay_off(relay_handle_t handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // gpio_set_level(handle->gpio, RELAY_OFF);
    handle->current_state = false;
    ESP_LOGI(TAG, "Relay OFF (GPIO %d)", handle->gpio);
    return ESP_OK;
}

esp_err_t relay_set_state(relay_handle_t handle, bool is_on) {
    return is_on ? relay_on(handle) : relay_off(handle);
}

esp_err_t relay_get_state(relay_handle_t handle, bool *is_on) {
    if (handle == NULL || is_on == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *is_on = handle->current_state;
    return ESP_OK;
}
