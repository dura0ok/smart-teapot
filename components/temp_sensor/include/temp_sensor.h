#pragma once

#include "esp_err.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handle for temperature sensor
 */
typedef struct temp_sensor_t *temp_sensor_handle_t;

/**
 * @brief Resolution of DS18B20 sensor
 */
typedef enum {
    TEMP_SENSOR_RESOLUTION_9BIT = 0,   ///< 9-bit resolution (0.5°C, 93.75ms)
    TEMP_SENSOR_RESOLUTION_10BIT = 1,  ///< 10-bit resolution (0.25°C, 187.5ms)
    TEMP_SENSOR_RESOLUTION_11BIT = 2,  ///< 11-bit resolution (0.125°C, 375ms)
    TEMP_SENSOR_RESOLUTION_12BIT = 3   ///< 12-bit resolution (0.0625°C, 750ms)
} temp_sensor_resolution_t;

/**
 * @brief Initialize temperature sensor
 * @param config Configuration containing GPIO pin for sensor
 * @param resolution Temperature sensor resolution
 * @param handle Output handle for the sensor
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t temp_sensor_init(const teapot_config_t *config, temp_sensor_resolution_t resolution, temp_sensor_handle_t *handle);

/**
 * @brief Deinitialize temperature sensor
 * @param handle Sensor handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t temp_sensor_deinit(temp_sensor_handle_t handle);

/**
 * @brief Trigger temperature conversion
 * @param handle Sensor handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t temp_sensor_trigger_conversion(temp_sensor_handle_t handle);

/**
 * @brief Read temperature from sensor
 * @param handle Sensor handle
 * @param temperature Output temperature in Celsius
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t temp_sensor_read(temp_sensor_handle_t handle, float *temperature);

/**
 * @brief Trigger conversion and read temperature (convenience function)
 * @param handle Sensor handle
 * @param temperature Output temperature in Celsius
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t temp_sensor_read_temperature(temp_sensor_handle_t handle, float *temperature);

#ifdef __cplusplus
}
#endif

