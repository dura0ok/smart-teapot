#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
#include "config.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Структура состояния чайника
 */
typedef struct {
    bool is_on;           ///< Включен ли чайник принудительно
    float setpoint_temp;  ///< Температура поддержания (°C)
    float current_temp;   ///< Текущая температура (°C)
} teapot_state_t;

/**
 * @brief Контекст WiFi веб-сервера
 */
typedef struct {
    httpd_handle_t server;
    teapot_state_t state;
    teapot_config_t *config;
} wifi_web_ctx_t;

/**
 * @brief Инициализация контекста веб-сервера (валидация и инициализация состояния)
 * @param ctx Контекст веб-сервера
 * @param config Конфигурация чайника (будет валидирована внутри)
 * @return ESP_OK в случае успеха, иначе код ошибки
 */
esp_err_t wifi_web_init_ctx(wifi_web_ctx_t *ctx, teapot_config_t *config);

/**
 * @brief Инициализация SPIFFS файловой системы
 * @return ESP_OK в случае успеха, иначе код ошибки
 */
esp_err_t wifi_web_init_spiffs(void);

/**
 * @brief Инициализация WiFi Access Point
 * @param config Конфигурация чайника
 * @return ESP_OK в случае успеха, иначе код ошибки
 */
esp_err_t wifi_web_init_wifi(teapot_config_t *config);

/**
 * @brief Полная инициализация WiFi веб-сервера (включает WiFi AP и SPIFFS)
 * @param ctx Контекст веб-сервера
 * @param config Конфигурация чайника (будет валидирована внутри)
 * @return ESP_OK в случае успеха, иначе код ошибки
 */
esp_err_t wifi_web_init(wifi_web_ctx_t *ctx, teapot_config_t *config);

/**
 * @brief Запуск WiFi веб-сервера
 * @param ctx Контекст веб-сервера
 * @return ESP_OK в случае успеха, иначе код ошибки
 */
esp_err_t wifi_web_start(wifi_web_ctx_t *ctx);

/**
 * @brief Остановка WiFi веб-сервера
 * @param ctx Контекст веб-сервера
 * @return ESP_OK в случае успеха, иначе код ошибки
 */
esp_err_t wifi_web_stop(wifi_web_ctx_t *ctx);

/**
 * @brief Включить/выключить чайник принудительно
 * @param ctx Контекст веб-сервера
 * @param is_on true для включения, false для выключения
 * @return ESP_OK в случае успеха, иначе код ошибки
 */
esp_err_t wifi_web_set_power(wifi_web_ctx_t *ctx, bool is_on);

/**
 * @brief Установить температуру поддержания
 * @param ctx Контекст веб-сервера
 * @param temperature Температура в градусах Цельсия
 * @return ESP_OK в случае успеха, иначе код ошибки
 */
esp_err_t wifi_web_set_setpoint(wifi_web_ctx_t *ctx, float temperature);

/**
 * @brief Получить текущее состояние чайника
 * @param ctx Контекст веб-сервера
 * @param state Указатель на структуру состояния (будет заполнена)
 * @return ESP_OK в случае успеха, иначе код ошибки
 */
esp_err_t wifi_web_get_state(wifi_web_ctx_t *ctx, teapot_state_t *state);

/**
 * @brief Установить текущую температуру (для обновления с датчика)
 * @param ctx Контекст веб-сервера
 * @param temperature Текущая температура в градусах Цельсия
 * @return ESP_OK в случае успеха, иначе код ошибки
 */
esp_err_t wifi_web_set_current_temp(wifi_web_ctx_t *ctx, float temperature);

#ifdef __cplusplus
}
#endif

