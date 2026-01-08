#include "wifi_web.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/ip4_addr.h"
#include "config.h"
#include "temp_sensor.h"
#include "relay.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"
#include <string.h>
#include <dirent.h>
#include <assert.h>

static const char *TAG = "WIFI_WEB";
static const char *SPIFFS_BASE_PATH = "/spiffs";
static wifi_web_ctx_t *g_ctx = NULL;
static bool server_started_from_event = false;
static esp_event_handler_instance_t wifi_event_handler_instance = NULL;

static esp_err_t send_json_response(httpd_req_t *req, cJSON *json) {
    char *json_str = cJSON_Print(json);
    if (json_str == NULL) {
        return ESP_ERR_NO_MEM;
    }

    httpd_resp_set_type(req, "application/json");
    esp_err_t ret = httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);
    free(json_str);
    return ret;
}

static esp_err_t send_file_from_spiffs(httpd_req_t *req, const char *filepath, const char *content_type) {
    char filepath_full[256];
    snprintf(filepath_full, sizeof(filepath_full), "%s%s", SPIFFS_BASE_PATH, filepath);
    
    FILE *file = fopen(filepath_full, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath_full);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, content_type);
    
    char buffer[512];
    size_t read_bytes;
    esp_err_t ret = ESP_OK;
    
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, read_bytes) != ESP_OK) {
            ret = ESP_FAIL;
            break;
        }
    }
    
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ret;
}

static esp_err_t index_html_handler(httpd_req_t *req) {
    return send_file_from_spiffs(req, "/index.html", "text/html");
}

static esp_err_t styles_css_handler(httpd_req_t *req) {
    return send_file_from_spiffs(req, "/styles.css", "text/css");
}

static esp_err_t script_js_handler(httpd_req_t *req) {
    return send_file_from_spiffs(req, "/script.js", "application/javascript");
}

static esp_err_t api_state_get_handler(httpd_req_t *req) {
    wifi_web_ctx_t *ctx = (wifi_web_ctx_t *)req->user_ctx;
    bool relay_state = false;
    
    if (ctx->relay_handle != NULL) {
        relay_get_state((relay_handle_t)ctx->relay_handle, &relay_state);
    }
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "is_on", ctx->state.is_on);
    cJSON_AddBoolToObject(json, "relay_state", relay_state);
    cJSON_AddNumberToObject(json, "setpoint_temp", ctx->state.setpoint_temp);
    cJSON_AddNumberToObject(json, "current_temp", ctx->state.current_temp);
    
    esp_err_t ret = send_json_response(req, json);
    cJSON_Delete(json);
    return ret;
}

// Handler for POST /api/power
static esp_err_t api_power_post_handler(httpd_req_t *req) {
    wifi_web_ctx_t *ctx = (wifi_web_ctx_t *)req->user_ctx;
    
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Bad Request", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    
    cJSON *is_on_item = cJSON_GetObjectItem(json, "is_on");
    if (!cJSON_IsBool(is_on_item)) {
        cJSON_Delete(json);
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Bad Request", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    
    ctx->state.is_on = cJSON_IsTrue(is_on_item);
    ESP_LOGI(TAG, "Power set to: %s", ctx->state.is_on ? "ON" : "OFF");
    cJSON_Delete(json);
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    esp_err_t err = send_json_response(req, response);
    cJSON_Delete(response);
    return err;
}

// Handler for POST /api/setpoint
static esp_err_t api_setpoint_post_handler(httpd_req_t *req) {
    wifi_web_ctx_t *ctx = (wifi_web_ctx_t *)req->user_ctx;
    
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Bad Request", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    
    cJSON *temp_item = cJSON_GetObjectItem(json, "temperature");
    if (!cJSON_IsNumber(temp_item)) {
        cJSON_Delete(json);
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Bad Request", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    
    float temp = (float)cJSON_GetNumberValue(temp_item);
    if (temp < CONFIG_TEMP_MIN || temp > CONFIG_TEMP_MAX) {
        cJSON_Delete(json);
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Bad Request", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    
    ctx->state.setpoint_temp = temp;
    ESP_LOGI(TAG, "Setpoint set to: %.1f°C", temp);
    cJSON_Delete(json);
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    esp_err_t err = send_json_response(req, response);
    cJSON_Delete(response);
    return err;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "WiFi Access Point started");
        // Start web server immediately after AP starts (only once per init cycle)
        if (g_ctx != NULL && g_ctx->server == NULL && !server_started_from_event) {
            server_started_from_event = true;
            esp_err_t ret = wifi_web_start(g_ctx);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Web server started on http://192.168.4.1");
            } else {
                // Log as debug if server already running or port in use (not a real error)
                if (ret == ESP_ERR_INVALID_STATE || ret == ESP_FAIL) {
                    ESP_LOGD(TAG, "Web server start skipped (already running or port in use)");
                } else {
                    ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
                }
                server_started_from_event = false; // Reset on failure to allow retry
            }
        } else if (g_ctx != NULL && g_ctx->server != NULL) {
            ESP_LOGD(TAG, "Web server already running, skipping start");
        } else if (server_started_from_event) {
            ESP_LOGD(TAG, "Server start already attempted from event, skipping");
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station connected: %02x:%02x:%02x:%02x:%02x:%02x",
                 event->mac[0], event->mac[1], event->mac[2],
                 event->mac[3], event->mac[4], event->mac[5]);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station disconnected: %02x:%02x:%02x:%02x:%02x:%02x",
                 event->mac[0], event->mac[1], event->mac[2],
                 event->mac[3], event->mac[4], event->mac[5]);
    }
}

// Initialize context only (for unit tests)
esp_err_t wifi_web_init_ctx(wifi_web_ctx_t *ctx, teapot_config_t *config) {
    if (ctx == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate configuration
    esp_err_t ret = config_validate(config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Config validation failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(wifi_web_ctx_t));
    ctx->config = config;
    ctx->state.is_on = false;
    ctx->state.setpoint_temp = config->default_setpoint;
    ctx->state.current_temp = 0.0f;
    ctx->server = NULL;
    ctx->temp_sensor_handle = NULL;
    ctx->temp_task_handle = NULL;
    
    return ESP_OK;
}

// Initialize SPIFFS only (for unit tests)
esp_err_t wifi_web_init_spiffs(void) {
    // Unmount SPIFFS if already mounted
    esp_err_t ret = esp_vfs_spiffs_unregister("web_storage");
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "SPIFFS unmounted before remounting");
    } else if (ret != ESP_ERR_INVALID_STATE && ret != ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to unmount SPIFFS: %s", esp_err_to_name(ret));
    }
    
    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = SPIFFS_BASE_PATH,
        .partition_label = "web_storage",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    ret = esp_vfs_spiffs_register(&spiffs_conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info("web_storage", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS partition size: total: %d, used: %d", total, used);
    }
    
    // List files in SPIFFS for debugging
    DIR *dir = opendir(SPIFFS_BASE_PATH);
    if (dir != NULL) {
        ESP_LOGI(TAG, "Files in SPIFFS:");
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            ESP_LOGI(TAG, "  %s", entry->d_name);
        }
        closedir(dir);
    } else {
        ESP_LOGW(TAG, "Failed to open SPIFFS directory: %s", SPIFFS_BASE_PATH);
    }
    
    return ESP_OK;
}

// Initialize WiFi AP only
esp_err_t wifi_web_init_wifi(teapot_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create default event loop (if not already created)
    esp_err_t event_ret = esp_event_loop_create_default();
    if (event_ret != ESP_OK && event_ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(event_ret));
        return event_ret;
    }
    
    // Create default WiFi Access Point (only if not already created)
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap_netif == NULL) {
        ap_netif = esp_netif_create_default_wifi_ap();
        if (ap_netif == NULL) {
            ESP_LOGE(TAG, "Failed to create WiFi AP netif");
            return ESP_FAIL;
        }
    }
    
    // Configure AP IP address
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));
    
    // Initialize WiFi with default configuration (only if not already initialized)
    static bool wifi_initialized = false;
    if (!wifi_initialized) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
            return ret;
        }
        wifi_initialized = true;
    }
    
    // Register WiFi event handlers (only once)
    if (wifi_event_handler_instance == NULL) {
        ret = esp_event_handler_instance_register(WIFI_EVENT,
                                                  ESP_EVENT_ANY_ID,
                                                  &wifi_event_handler,
                                                  NULL,
                                                  &wifi_event_handler_instance);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
            return ret;
        }
    }
    
    // Configure WiFi Access Point
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(config->wifi.ssid),
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        },
    };
    strncpy((char*)wifi_config.ap.ssid, config->wifi.ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid[sizeof(wifi_config.ap.ssid) - 1] = '\0';
    
    // Set password if provided (validation already checked min length)
    bool has_password = strlen(config->wifi.password) > 0;
    if (has_password) {
        strncpy((char*)wifi_config.ap.password, config->wifi.password, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.password[sizeof(wifi_config.ap.password) - 1] = '\0';
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
        ESP_LOGI(TAG, "WiFi AP configured with password (WPA2_PSK)");
    } else {
        ESP_LOGI(TAG, "WiFi AP configured as open network (no password)");
    }
    
    // Stop WiFi if already started (for tests cleanup)
    wifi_mode_t current_mode;
    esp_err_t mode_ret = esp_wifi_get_mode(&current_mode);
    if (mode_ret == ESP_OK && current_mode != WIFI_MODE_NULL) {
        ESP_LOGD(TAG, "Stopping WiFi before reconfiguration");
        esp_wifi_stop();
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait for WiFi to stop
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi Access Point initialized. SSID: %s, IP: 192.168.4.1", config->wifi.ssid);
    
    return ESP_OK;
}

// Full initialization (for main.c)
esp_err_t wifi_web_init(wifi_web_ctx_t *ctx, teapot_config_t *config) {
    if (ctx == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (g_ctx != NULL && g_ctx->server != NULL) {
        wifi_web_stop(g_ctx);
    }
    
    server_started_from_event = false;
    
    esp_err_t ret = wifi_web_init_ctx(ctx, config);
    if (ret != ESP_OK) {
        return ret;
    }
    
    g_ctx = ctx;
    
    ret = wifi_web_init_spiffs();
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = wifi_web_init_wifi(config);
    if (ret != ESP_OK) {
        return ret;
    }
    
    relay_handle_t relay;
    ret = relay_init(config, &relay);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize relay: %s", esp_err_to_name(ret));
        return ret;
    }
    ctx->relay_handle = (void *)relay;
    ESP_LOGI(TAG, "Relay initialized");
    
    return ESP_OK;
}

esp_err_t wifi_web_start(wifi_web_ctx_t *ctx) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (ctx->server != NULL) {
        ESP_LOGW(TAG, "Server already started");
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_len = 512;
    config.lru_purge_enable = true;
    
    esp_err_t ret = httpd_start(&ctx->server, &config);
    if (ret != ESP_OK) {
        // If server already running or port in use, it's not an error (concurrent start from events)
        if (ret == ESP_ERR_INVALID_STATE || ret == ESP_FAIL) {
            ESP_LOGD(TAG, "HTTP server port in use (concurrent start attempt), continuing");
            // Server might already be running from concurrent event handler call
            // Just return OK - port is in use, which is expected with multiple events
            return ESP_OK;
        } else {
            ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
            server_started_from_event = false; // Reset flag on real error
            return ret;
        }
    }
    
    // Server started successfully - flag already set in event handler
    
    // Register URI handlers
    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_html_handler,
        .user_ctx = ctx
    };
    httpd_register_uri_handler(ctx->server, &index_uri);
    
    httpd_uri_t html_uri = {
        .uri = "/index.html",
        .method = HTTP_GET,
        .handler = index_html_handler,
        .user_ctx = ctx
    };
    httpd_register_uri_handler(ctx->server, &html_uri);
    
    httpd_uri_t css_uri = {
        .uri = "/styles.css",
        .method = HTTP_GET,
        .handler = styles_css_handler,
        .user_ctx = ctx
    };
    httpd_register_uri_handler(ctx->server, &css_uri);
    
    httpd_uri_t js_uri = {
        .uri = "/script.js",
        .method = HTTP_GET,
        .handler = script_js_handler,
        .user_ctx = ctx
    };
    httpd_register_uri_handler(ctx->server, &js_uri);
    
    httpd_uri_t state_get_uri = {
        .uri = "/api/state",
        .method = HTTP_GET,
        .handler = api_state_get_handler,
        .user_ctx = ctx
    };
    httpd_register_uri_handler(ctx->server, &state_get_uri);
    
    httpd_uri_t power_post_uri = {
        .uri = "/api/power",
        .method = HTTP_POST,
        .handler = api_power_post_handler,
        .user_ctx = ctx
    };
    httpd_register_uri_handler(ctx->server, &power_post_uri);
    
    httpd_uri_t setpoint_post_uri = {
        .uri = "/api/setpoint",
        .method = HTTP_POST,
        .handler = api_setpoint_post_handler,
        .user_ctx = ctx
    };
    httpd_register_uri_handler(ctx->server, &setpoint_post_uri);
    
    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}

esp_err_t wifi_web_stop(wifi_web_ctx_t *ctx) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    wifi_web_stop_temp_sensor(ctx);
    
    if (ctx->relay_handle != NULL) {
        relay_deinit((relay_handle_t)ctx->relay_handle);
        ctx->relay_handle = NULL;
        ESP_LOGI(TAG, "Relay deinitialized");
    }
    
    if (ctx->server != NULL) {
        esp_err_t ret = httpd_stop(ctx->server);
        if (ret != ESP_OK) {
            return ret;
        }
        ctx->server = NULL;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
    
    server_started_from_event = false;
    esp_vfs_spiffs_unregister("web_storage");
    ESP_LOGI(TAG, "SPIFFS unmounted");
    
    return ESP_OK;
}

esp_err_t wifi_web_set_power(wifi_web_ctx_t *ctx, bool is_on) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ctx->state.is_on = is_on;
    ESP_LOGI(TAG, "Power set to: %s", is_on ? "ON" : "OFF");
    return ESP_OK;
}

esp_err_t wifi_web_set_setpoint(wifi_web_ctx_t *ctx, float temperature) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (temperature < CONFIG_TEMP_MIN || temperature > CONFIG_TEMP_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ctx->state.setpoint_temp = temperature;
    ESP_LOGI(TAG, "Setpoint set to: %.1f°C", temperature);
    return ESP_OK;
}

esp_err_t wifi_web_get_state(wifi_web_ctx_t *ctx, teapot_state_t *state) {
    if (ctx == NULL || state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *state = ctx->state;
    return ESP_OK;
}

esp_err_t wifi_web_set_current_temp(wifi_web_ctx_t *ctx, float temperature) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ctx->state.current_temp = temperature;
    return ESP_OK;
}

static void temp_sensor_task(void *pvParameters) {
    wifi_web_ctx_t *ctx = (wifi_web_ctx_t *)pvParameters;
    temp_sensor_handle_t sensor = (temp_sensor_handle_t)ctx->temp_sensor_handle;
    relay_handle_t relay = (relay_handle_t)ctx->relay_handle;
    
    ESP_LOGI(TAG, "Temperature sensor task started");
    
    while (1) {
        float temperature;
        esp_err_t ret = temp_sensor_read_temperature(sensor, &temperature);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read temperature: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        
        if (temperature == 85.0f || temperature == -85.0f) {
            ESP_LOGW(TAG, "Temperature reading failed (default value: %.2f°C)", temperature);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        
        wifi_web_set_current_temp(ctx, temperature);
        ESP_LOGI(TAG, "Temperature: %.2f°C", temperature);
        
        if (relay == NULL) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        
        if (!ctx->state.is_on) {
            bool current_state;
            relay_get_state(relay, &current_state);
            if (current_state) {
                relay_set_state(relay, false);
                ESP_LOGI(TAG, "Power off: relay OFF");
            }
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        
        bool should_be_on = temperature < ctx->state.setpoint_temp;
        bool current_state;
        relay_get_state(relay, &current_state);
        
        if (should_be_on != current_state) {
            relay_set_state(relay, should_be_on);
            ESP_LOGI(TAG, "Auto control: relay %s (temp %.2f°C %s setpoint %.2f°C)", 
                    should_be_on ? "ON" : "OFF", temperature, 
                    should_be_on ? "<" : ">=", ctx->state.setpoint_temp);
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

esp_err_t wifi_web_start_temp_sensor(wifi_web_ctx_t *ctx) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (ctx->temp_sensor_handle != NULL) {
        ESP_LOGW(TAG, "Temperature sensor already initialized");
        return ESP_OK;
    }
    
    if (ctx->config == NULL) {
        ESP_LOGE(TAG, "Config is NULL, cannot initialize temperature sensor");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Initialize temperature sensor
    temp_sensor_handle_t sensor;
    esp_err_t ret = temp_sensor_init(ctx->config, TEMP_SENSOR_RESOLUTION_12BIT, &sensor);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize temperature sensor: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ctx->temp_sensor_handle = (void *)sensor;
    ESP_LOGI(TAG, "Temperature sensor initialized");
    
    // Create task for reading temperature
    BaseType_t task_ret = xTaskCreate(
        temp_sensor_task,
        "temp_sensor",
        4096,
        ctx,
        5,
        (TaskHandle_t *)&ctx->temp_task_handle
    );
    
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create temperature sensor task");
        temp_sensor_deinit(sensor);
        ctx->temp_sensor_handle = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Temperature sensor task created");
    return ESP_OK;
}

esp_err_t wifi_web_stop_temp_sensor(wifi_web_ctx_t *ctx) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Delete task if exists
    if (ctx->temp_task_handle != NULL) {
        vTaskDelete((TaskHandle_t)ctx->temp_task_handle);
        ctx->temp_task_handle = NULL;
        ESP_LOGI(TAG, "Temperature sensor task deleted");
    }
    
    // Deinitialize sensor if exists
    if (ctx->temp_sensor_handle != NULL) {
        temp_sensor_deinit((temp_sensor_handle_t)ctx->temp_sensor_handle);
        ctx->temp_sensor_handle = NULL;
        ESP_LOGI(TAG, "Temperature sensor deinitialized");
    }
    
    return ESP_OK;
}