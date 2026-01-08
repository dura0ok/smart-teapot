#include <unity.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include <string.h>

static void test_init_default(void) {
    teapot_config_t config;
    esp_err_t ret = config_init_default(&config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("SmartTeapot", config.wifi.ssid);
    TEST_ASSERT_EQUAL_STRING("", config.wifi.password);
    TEST_ASSERT_EQUAL(4, config.gpio.relay_gpio);
    TEST_ASSERT_EQUAL(5, config.gpio.temp_sensor_gpio);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 85.0f, config.default_setpoint);
}

static void test_init_default_null(void) {
    esp_err_t ret = config_init_default(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_validate_success(void) {
    teapot_config_t config;
    config_init_default(&config);
    esp_err_t ret = config_validate(&config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

static void test_validate_null(void) {
    esp_err_t ret = config_validate(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_validate_empty_ssid(void) {
    teapot_config_t config;
    config_init_default(&config);
    config.wifi.ssid[0] = '\0';
    esp_err_t ret = config_validate(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

static void test_validate_invalid_relay_gpio_negative(void) {
    teapot_config_t config;
    config_init_default(&config);
    config.gpio.relay_gpio = -1;
    esp_err_t ret = config_validate(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_validate_invalid_relay_gpio_too_large(void) {
    teapot_config_t config;
    config_init_default(&config);
    config.gpio.relay_gpio = 49;
    esp_err_t ret = config_validate(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_validate_invalid_temp_gpio(void) {
    teapot_config_t config;
    config_init_default(&config);
    config.gpio.temp_sensor_gpio = 50;
    esp_err_t ret = config_validate(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_validate_duplicate_gpio(void) {
    teapot_config_t config;
    config_init_default(&config);
    config.gpio.relay_gpio = 4;
    config.gpio.temp_sensor_gpio = 4;
    esp_err_t ret = config_validate(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

static void test_validate_temp_too_low(void) {
    teapot_config_t config;
    config_init_default(&config);
    config.default_setpoint = -56.0f;
    esp_err_t ret = config_validate(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_validate_temp_too_high(void) {
    teapot_config_t config;
    config_init_default(&config);
    config.default_setpoint = 126.0f;
    esp_err_t ret = config_validate(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_set_wifi_ssid(void) {
    teapot_config_t config;
    config_init_default(&config);
    const char *new_ssid = "MyNetwork";
    esp_err_t ret = config_set_wifi_ssid(&config, new_ssid);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING(new_ssid, config.wifi.ssid);
}

static void test_set_wifi_ssid_null(void) {
    teapot_config_t config;
    config_init_default(&config);
    esp_err_t ret = config_set_wifi_ssid(NULL, "Test");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    ret = config_set_wifi_ssid(&config, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_set_wifi_ssid_empty(void) {
    teapot_config_t config;
    config_init_default(&config);
    esp_err_t ret = config_set_wifi_ssid(&config, "");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, ret);
}

static void test_set_wifi_password(void) {
    teapot_config_t config;
    config_init_default(&config);
    const char *password = "MyPassword123";
    esp_err_t ret = config_set_wifi_password(&config, password);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING(password, config.wifi.password);
}

static void test_set_wifi_password_empty(void) {
    teapot_config_t config;
    config_init_default(&config);
    esp_err_t ret = config_set_wifi_password(&config, "");
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("", config.wifi.password);
}

static void test_validate_short_password(void) {
    teapot_config_t config;
    config_init_default(&config);
    // Set a short password (less than 8 chars)
    strncpy(config.wifi.password, "1234567", sizeof(config.wifi.password) - 1);
    config.wifi.password[sizeof(config.wifi.password) - 1] = '\0';
    
    esp_err_t ret = config_validate(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, ret);
}

static void test_validate_empty_password_ok(void) {
    teapot_config_t config;
    config_init_default(&config);
    // Empty password is OK (OPEN network)
    config.wifi.password[0] = '\0';
    
    esp_err_t ret = config_validate(&config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

static void test_set_relay_gpio(void) {
    teapot_config_t config;
    config_init_default(&config);
    int new_gpio = 10;
    esp_err_t ret = config_set_relay_gpio(&config, new_gpio);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(new_gpio, config.gpio.relay_gpio);
}

static void test_set_relay_gpio_invalid(void) {
    teapot_config_t config;
    config_init_default(&config);
    esp_err_t ret = config_set_relay_gpio(&config, -1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    ret = config_set_relay_gpio(&config, 49);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    config.gpio.temp_sensor_gpio = 10;
    ret = config_set_relay_gpio(&config, 10);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

static void test_set_temp_sensor_gpio(void) {
    teapot_config_t config;
    config_init_default(&config);
    int new_gpio = 12;
    esp_err_t ret = config_set_temp_sensor_gpio(&config, new_gpio);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(new_gpio, config.gpio.temp_sensor_gpio);
}

static void test_set_default_setpoint(void) {
    teapot_config_t config;
    config_init_default(&config);
    float new_setpoint = 90.0f;
    esp_err_t ret = config_set_default_setpoint(&config, new_setpoint);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, new_setpoint, config.default_setpoint);
}

static void test_set_default_setpoint_invalid(void) {
    teapot_config_t config;
    config_init_default(&config);
    esp_err_t ret = config_set_default_setpoint(&config, -56.0f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    ret = config_set_default_setpoint(&config, 126.0f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_setpoint_boundaries(void) {
    teapot_config_t config;
    config_init_default(&config);
    esp_err_t ret = config_set_default_setpoint(&config, CONFIG_TEMP_MIN);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    ret = config_set_default_setpoint(&config, CONFIG_TEMP_MAX);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

static void test_gpio_boundaries(void) {
    teapot_config_t config;
    config_init_default(&config);
    esp_err_t ret = config_set_relay_gpio(&config, CONFIG_GPIO_MIN);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    ret = config_set_temp_sensor_gpio(&config, CONFIG_GPIO_MAX);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

static void test_config_module_loaded(void) {
    TEST_ASSERT_TRUE(1);
}

void run_config_tests(void) {
    RUN_TEST(test_config_module_loaded);
    RUN_TEST(test_init_default);
    RUN_TEST(test_init_default_null);
    RUN_TEST(test_validate_success);
    RUN_TEST(test_validate_null);
    RUN_TEST(test_validate_empty_ssid);
    RUN_TEST(test_validate_invalid_relay_gpio_negative);
    RUN_TEST(test_validate_invalid_relay_gpio_too_large);
    RUN_TEST(test_validate_invalid_temp_gpio);
    RUN_TEST(test_validate_duplicate_gpio);
    RUN_TEST(test_validate_temp_too_low);
    RUN_TEST(test_validate_temp_too_high);
    RUN_TEST(test_set_wifi_ssid);
    RUN_TEST(test_set_wifi_ssid_null);
    RUN_TEST(test_set_wifi_ssid_empty);
    RUN_TEST(test_set_wifi_password);
    RUN_TEST(test_set_wifi_password_empty);
    RUN_TEST(test_validate_short_password);
    RUN_TEST(test_validate_empty_password_ok);
    RUN_TEST(test_set_relay_gpio);
    RUN_TEST(test_set_relay_gpio_invalid);
    RUN_TEST(test_set_temp_sensor_gpio);
    RUN_TEST(test_set_default_setpoint);
    RUN_TEST(test_set_default_setpoint_invalid);
    RUN_TEST(test_setpoint_boundaries);
    RUN_TEST(test_gpio_boundaries);
}