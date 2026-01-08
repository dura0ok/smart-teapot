#include "temp_sensor.h"
#include "onewire_bus.h"
#include "onewire_bus_impl_rmt.h"
#include "onewire_device.h"
#include "ds18b20.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "TEMP_SENSOR";

struct temp_sensor_t {
    onewire_bus_handle_t bus;
    onewire_device_t device;
    ds18b20_handle_t ds;
    bool initialized;
};

static struct temp_sensor_t g_sensor = {0};

esp_err_t temp_sensor_init(const teapot_config_t *config, temp_sensor_resolution_t resolution, temp_sensor_handle_t *handle) {
    if (config == NULL || handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (resolution > TEMP_SENSOR_RESOLUTION_12BIT) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->gpio.temp_sensor_gpio < CONFIG_GPIO_MIN || config->gpio.temp_sensor_gpio > CONFIG_GPIO_MAX) {
        ESP_LOGE(TAG, "Invalid GPIO: %d", config->gpio.temp_sensor_gpio);
        return ESP_ERR_INVALID_ARG;
    }
    
    g_sensor.initialized = false;

    const onewire_bus_config_t owb_cfg = {
        .bus_gpio_num = config->gpio.temp_sensor_gpio,
    };

    const onewire_bus_rmt_config_t rmt_cfg = {
        .max_rx_bytes = 10,
    };
    
    esp_err_t ret = onewire_new_bus_rmt(&owb_cfg, &rmt_cfg, &g_sensor.bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize 1-Wire bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "1-Wire bus initialized on GPIO %d", config->gpio.temp_sensor_gpio);
    
    onewire_device_iter_handle_t iter;
    ret = onewire_new_device_iter(g_sensor.bus, &iter);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create device iterator: %s", esp_err_to_name(ret));
        onewire_bus_del(g_sensor.bus);
        return ret;
    }
    
    ret = onewire_device_iter_get_next(iter, &g_sensor.device);
    onewire_del_device_iter(iter);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "No DS18B20 found on bus");
        onewire_bus_del(g_sensor.bus);
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Found DS18B20 device: %016llX", g_sensor.device.address);

    const ds18b20_config_t ds_cfg = {
        .resolution = (ds18b20_resolutions_t)resolution,
        .trigger_enabled = false,
        .trigger_high = 0,
        .trigger_low = 0
    };
    
    ret = ds18b20_init(&g_sensor.device, &ds_cfg, &g_sensor.ds);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize DS18B20: %s", esp_err_to_name(ret));
        onewire_bus_del(g_sensor.bus);
        return ret;
    }
    
    ret = ds18b20_set_resolution(g_sensor.ds, (ds18b20_resolutions_t)resolution);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set resolution: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Resolution set to %d-bit", 9 + (int)resolution);
    }
    
    g_sensor.initialized = true;
    *handle = &g_sensor;
    
    ESP_LOGI(TAG, "Temperature sensor initialized");
    return ESP_OK;
}

esp_err_t temp_sensor_deinit(temp_sensor_handle_t handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (g_sensor.ds != NULL) {
        ds18b20_delete(g_sensor.ds);
    }
    
    if (g_sensor.bus != NULL) {
        onewire_bus_del(g_sensor.bus);
    }
    
    memset(&g_sensor, 0, sizeof(g_sensor));
    return ESP_OK;
}

esp_err_t temp_sensor_trigger_conversion(temp_sensor_handle_t handle) {
    if (handle == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return ds18b20_trigger_temperature_conversion(handle->ds);
}

esp_err_t temp_sensor_read(temp_sensor_handle_t handle, float *temperature) {
    if (handle == NULL || temperature == NULL || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return ds18b20_get_measurement(handle->ds, temperature);
}

esp_err_t temp_sensor_read_temperature(temp_sensor_handle_t handle, float *temperature) {
    if (handle == NULL || temperature == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = temp_sensor_trigger_conversion(handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    return temp_sensor_read(handle, temperature);
}