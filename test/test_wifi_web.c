#include <unity.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_web.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

static wifi_web_ctx_t ctx;
static teapot_config_t config;

static void init_test_config(void) {
    config_init_default(&config);
    memset(&ctx, 0, sizeof(ctx));
}

static void test_wifi_web_init_ctx_success(void) {
    init_test_config();
    esp_err_t ret = wifi_web_init_ctx(&ctx, &config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(&config, ctx.config);
    TEST_ASSERT_FALSE(ctx.state.is_on);
    TEST_ASSERT_EQUAL_FLOAT(config.default_setpoint, ctx.state.setpoint_temp);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ctx.state.current_temp);
    TEST_ASSERT_NULL(ctx.server);
}

static void test_wifi_web_init_ctx_null_ctx(void) {
    init_test_config();
    esp_err_t ret = wifi_web_init_ctx(NULL, &config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_wifi_web_init_ctx_null_config(void) {
    init_test_config();
    esp_err_t ret = wifi_web_init_ctx(&ctx, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_wifi_web_init_ctx_validates_config(void) {
    init_test_config();
    config.wifi.ssid[0] = '\0';
    
    esp_err_t ret = wifi_web_init_ctx(&ctx, &config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

static void test_wifi_web_set_power_on(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    
    esp_err_t ret = wifi_web_set_power(&ctx, true);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(ctx.state.is_on);
}

static void test_wifi_web_set_power_off(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    ctx.state.is_on = true;
    
    esp_err_t ret = wifi_web_set_power(&ctx, false);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_FALSE(ctx.state.is_on);
}

static void test_wifi_web_set_power_null_ctx(void) {
    esp_err_t ret = wifi_web_set_power(NULL, true);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_wifi_web_set_setpoint_valid(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    
    float test_temp = 75.5f;
    esp_err_t ret = wifi_web_set_setpoint(&ctx, test_temp);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_FLOAT(test_temp, ctx.state.setpoint_temp);
}

static void test_wifi_web_set_setpoint_min(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    
    esp_err_t ret = wifi_web_set_setpoint(&ctx, CONFIG_TEMP_MIN);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_FLOAT(CONFIG_TEMP_MIN, ctx.state.setpoint_temp);
}

static void test_wifi_web_set_setpoint_max(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    
    esp_err_t ret = wifi_web_set_setpoint(&ctx, CONFIG_TEMP_MAX);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_FLOAT(CONFIG_TEMP_MAX, ctx.state.setpoint_temp);
}

static void test_wifi_web_set_setpoint_below_min(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    
    float original_temp = ctx.state.setpoint_temp;
    esp_err_t ret = wifi_web_set_setpoint(&ctx, CONFIG_TEMP_MIN - 1.0f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    TEST_ASSERT_EQUAL_FLOAT(original_temp, ctx.state.setpoint_temp);
}

static void test_wifi_web_set_setpoint_above_max(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    
    float original_temp = ctx.state.setpoint_temp;
    esp_err_t ret = wifi_web_set_setpoint(&ctx, CONFIG_TEMP_MAX + 1.0f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    TEST_ASSERT_EQUAL_FLOAT(original_temp, ctx.state.setpoint_temp);
}

static void test_wifi_web_set_setpoint_null_ctx(void) {
    esp_err_t ret = wifi_web_set_setpoint(NULL, 75.0f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_wifi_web_get_state(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    ctx.state.is_on = true;
    ctx.state.setpoint_temp = 90.0f;
    ctx.state.current_temp = 85.5f;
    
    teapot_state_t state;
    esp_err_t ret = wifi_web_get_state(&ctx, &state);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(ctx.state.is_on, state.is_on);
    TEST_ASSERT_EQUAL_FLOAT(ctx.state.setpoint_temp, state.setpoint_temp);
    TEST_ASSERT_EQUAL_FLOAT(ctx.state.current_temp, state.current_temp);
}

static void test_wifi_web_get_state_null_ctx(void) {
    teapot_state_t state;
    esp_err_t ret = wifi_web_get_state(NULL, &state);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_wifi_web_get_state_null_state(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    esp_err_t ret = wifi_web_get_state(&ctx, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_wifi_web_set_current_temp(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    
    float test_temp = 42.3f;
    esp_err_t ret = wifi_web_set_current_temp(&ctx, test_temp);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_FLOAT(test_temp, ctx.state.current_temp);
}

static void test_wifi_web_set_current_temp_negative(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    
    float test_temp = -5.0f;
    esp_err_t ret = wifi_web_set_current_temp(&ctx, test_temp);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_FLOAT(test_temp, ctx.state.current_temp);
}

static void test_wifi_web_set_current_temp_null_ctx(void) {
    esp_err_t ret = wifi_web_set_current_temp(NULL, 25.0f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

static void test_wifi_web_state_initialization(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    
    TEST_ASSERT_FALSE(ctx.state.is_on);
    TEST_ASSERT_EQUAL_FLOAT(config.default_setpoint, ctx.state.setpoint_temp);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ctx.state.current_temp);
}

static void test_wifi_web_multiple_setpoint_changes(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    
    float temps[] = {50.0f, 60.5f, 75.0f, 90.0f};
    for (int i = 0; i < 4; i++) {
        esp_err_t ret = wifi_web_set_setpoint(&ctx, temps[i]);
        TEST_ASSERT_EQUAL(ESP_OK, ret);
        TEST_ASSERT_EQUAL_FLOAT(temps[i], ctx.state.setpoint_temp);
    }
}

static void test_wifi_web_power_toggle(void) {
    init_test_config();
    wifi_web_init_ctx(&ctx, &config);
    
    TEST_ASSERT_EQUAL(ESP_OK, wifi_web_set_power(&ctx, true));
    TEST_ASSERT_TRUE(ctx.state.is_on);
    
    TEST_ASSERT_EQUAL(ESP_OK, wifi_web_set_power(&ctx, false));
    TEST_ASSERT_FALSE(ctx.state.is_on);
    
    TEST_ASSERT_EQUAL(ESP_OK, wifi_web_set_power(&ctx, true));
    TEST_ASSERT_TRUE(ctx.state.is_on);
}


static void test_wifi_web_spiffs_init(void) {
    esp_err_t ret = wifi_web_init_spiffs();
    if (ret == ESP_ERR_NOT_FOUND) {
        TEST_IGNORE_MESSAGE("SPIFFS partition not found in test environment");
        return;
    }
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

static void test_wifi_web_spiffs_file_exists(void) {
    esp_err_t ret = wifi_web_init_spiffs();
    if (ret == ESP_ERR_NOT_FOUND) {
        TEST_IGNORE_MESSAGE("SPIFFS partition not found in test environment");
        return;
    }
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    FILE *file = fopen("/spiffs/index.html", "r");
    if (file == NULL) {
        TEST_IGNORE_MESSAGE("SPIFFS image not flashed, skipping HTML file test");
        return;
    }
    fclose(file);
    TEST_PASS();
}

static void test_wifi_web_spiffs_file_content(void) {
    esp_err_t ret = wifi_web_init_spiffs();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    FILE *file = fopen("/spiffs/index.html", "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(file, "index.html file not found in SPIFFS - SPIFFS image should be loaded from data/ folder during build");
    
    char buffer[512];
    size_t read_bytes = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[read_bytes] = '\0';
    fclose(file);
    
    TEST_ASSERT_GREATER_THAN(0, read_bytes);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "<!DOCTYPE html>"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "Умный Чайник"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "<html"));
}

void run_wifi_web_tests(void) {
    RUN_TEST(test_wifi_web_init_ctx_success);
    RUN_TEST(test_wifi_web_init_ctx_null_ctx);
    RUN_TEST(test_wifi_web_init_ctx_null_config);
    RUN_TEST(test_wifi_web_set_power_on);
    RUN_TEST(test_wifi_web_set_power_off);
    RUN_TEST(test_wifi_web_set_power_null_ctx);
    RUN_TEST(test_wifi_web_set_setpoint_valid);
    RUN_TEST(test_wifi_web_set_setpoint_min);
    RUN_TEST(test_wifi_web_set_setpoint_max);
    RUN_TEST(test_wifi_web_set_setpoint_below_min);
    RUN_TEST(test_wifi_web_set_setpoint_above_max);
    RUN_TEST(test_wifi_web_set_setpoint_null_ctx);
    RUN_TEST(test_wifi_web_get_state);
    RUN_TEST(test_wifi_web_get_state_null_ctx);
    RUN_TEST(test_wifi_web_get_state_null_state);
    RUN_TEST(test_wifi_web_set_current_temp);
    RUN_TEST(test_wifi_web_set_current_temp_negative);
    RUN_TEST(test_wifi_web_set_current_temp_null_ctx);
    RUN_TEST(test_wifi_web_state_initialization);
    RUN_TEST(test_wifi_web_multiple_setpoint_changes);
    RUN_TEST(test_wifi_web_power_toggle);
    RUN_TEST(test_wifi_web_init_ctx_validates_config);
    RUN_TEST(test_wifi_web_spiffs_init);
    RUN_TEST(test_wifi_web_spiffs_file_exists);
    RUN_TEST(test_wifi_web_spiffs_file_content);
}