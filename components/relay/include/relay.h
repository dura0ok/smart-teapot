#pragma once

#include "esp_err.h"
#include "config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handle for relay
 */
typedef struct relay_t *relay_handle_t;

/**
 * @brief Initialize relay
 * @param config Configuration containing GPIO pin for relay
 * @param handle Output handle for the relay
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t relay_init(const teapot_config_t *config, relay_handle_t *handle);

/**
 * @brief Deinitialize relay
 * @param handle Relay handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t relay_deinit(relay_handle_t handle);

/**
 * @brief Turn relay ON
 * @param handle Relay handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t relay_on(relay_handle_t handle);

/**
 * @brief Turn relay OFF
 * @param handle Relay handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t relay_off(relay_handle_t handle);

/**
 * @brief Set relay state
 * @param handle Relay handle
 * @param is_on true to turn ON, false to turn OFF
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t relay_set_state(relay_handle_t handle, bool is_on);

/**
 * @brief Get current relay state
 * @param handle Relay handle
 * @param is_on Output parameter: true if ON, false if OFF
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t relay_get_state(relay_handle_t handle, bool *is_on);

#ifdef __cplusplus
}
#endif

