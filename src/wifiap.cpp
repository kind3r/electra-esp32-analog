#include "wifiap.h"
#include "dnsserver.h"

static const char *TAG = "WiFiAP";

httpd_handle_t WiFiAP::server = nullptr;
httpd_uri_t WiFiAP::home = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = handleHome,
    .user_ctx = NULL};
httpd_uri_t WiFiAP::configGet = {
    .uri = "/config",
    .method = HTTP_GET,
    .handler = handleConfigGet,
    .user_ctx = NULL};
httpd_uri_t WiFiAP::configSet = {
    .uri = "/config",
    .method = HTTP_POST,
    .handler = handleConfigSet,
    .user_ctx = NULL};
uint8_t *WiFiAP::buffer = new uint8_t[ELECTRA_ESP_CONFIG_BUFFER_SIZE];

void WiFiAP::init()
{
  mdns_start();
  wifi_init_softap();
  blinkPattern_t pattern = PATTERN_BLINK_SLOW();
  Led::setBlinkPattern(&pattern);
}

void WiFiAP::startWebServer()
{
  if (server == nullptr)
  {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 3;
    config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_start(&server, &config);
    httpd_register_uri_handler(server, &home);
    httpd_register_uri_handler(server, &configGet);
    httpd_register_uri_handler(server, &configSet);
  }
}

void WiFiAP::event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_wifi_connect();
  }
  else if (event_id == WIFI_EVENT_AP_STACONNECTED)
  {
    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    startWebServer();
  }
  else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
  {
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
  }
}

void WiFiAP::wifi_init_softap()
{
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &event_handler,
                                                      NULL,
                                                      NULL));

  wifi_config_t wifi_config = {};
  strcpy((char *)wifi_config.ap.ssid, ELECTRA_ESP_WIFI_CONFIG_SSID);
  wifi_config.ap.ssid_len = strlen(ELECTRA_ESP_WIFI_CONFIG_SSID);
  strcpy((char *)wifi_config.ap.password, ELECTRA_ESP_WIFI_CONFIG_PASS);
  wifi_config.ap.channel = ELECTRA_ESP_WIFI_CONFIG_CHANNEL;
  wifi_config.ap.max_connection = ELECTRA_ESP_WIFI_CONFIG_MAX_CONNECTIONS;
  wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
           ELECTRA_ESP_WIFI_CONFIG_SSID, ELECTRA_ESP_WIFI_CONFIG_PASS, ELECTRA_ESP_WIFI_CONFIG_CHANNEL);

  // Setup DNS server to respond to name.local
  char hostname[strlen(ELECTRA_ESP_WIFI_HOSTNAME) + 7];
  memcpy(hostname, ELECTRA_ESP_WIFI_HOSTNAME, strlen(ELECTRA_ESP_WIFI_HOSTNAME));
  memcpy(hostname + strlen(ELECTRA_ESP_WIFI_HOSTNAME), ".local", 6);
  hostname[strlen(ELECTRA_ESP_WIFI_HOSTNAME) + 7] = '\0';
  init_dns_server(hostname);
}

esp_err_t WiFiAP::handleHome(httpd_req_t *req)
{
  FILE *fd = nullptr;
  fd = fopen("/spiffs/index.html.gz", "r");
  if (!fd)
  {
    ESP_LOGE(TAG, "Failed to read index.html.gz from SPIFFS");
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
    return ESP_FAIL;
  }

  // set content type and encoding
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

  /* Retrieve the pointer to scratch buffer for temporary storage */
  size_t chunksize;
  do
  {
    /* Read file in chunks into the scratch buffer */
    chunksize = fread(buffer, 1, ELECTRA_ESP_CONFIG_BUFFER_SIZE, fd);

    if (chunksize > 0)
    {
      /* Send the buffer contents as HTTP response chunk */
      if (httpd_resp_send_chunk(req, (char *)buffer, chunksize) != ESP_OK)
      {
        fclose(fd);
        ESP_LOGE(TAG, "File sending failed!");
        /* Abort sending file */
        httpd_resp_sendstr_chunk(req, NULL);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
        return ESP_FAIL;
      }
    }

    /* Keep looping till the whole file is sent */
  } while (chunksize != 0);

  fclose(fd);

  return ESP_OK;
}

esp_err_t WiFiAP::handleConfigGet(httpd_req_t *req)
{
  httpd_resp_set_type(req, HTTPD_TYPE_JSON);

  Settings::getConfigFile(buffer);

  httpd_resp_sendstr(req, (char *)buffer);

  return ESP_OK;
}

esp_err_t WiFiAP::handleConfigSet(httpd_req_t *req)
{
  int total_len = req->content_len;
  int cur_len = 0;
  int received = 0;
  if (total_len >= ELECTRA_ESP_CONFIG_BUFFER_SIZE)
  {
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
    return ESP_FAIL;
  }
  while (cur_len < total_len)
  {
    received = httpd_req_recv(req, (char *)buffer + cur_len, total_len);
    if (received <= 0)
    {
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
      return ESP_FAIL;
    }
    cur_len += received;
  }
  buffer[total_len] = '\0';

  httpd_resp_set_type(req, "text/html");
  if (Settings::putConfigFile(buffer, total_len + 1) == ESP_OK)
  {
    httpd_resp_sendstr(req, "OK");
    // reboot
    xTaskCreate(rebootTask, "reboot_task", 2048, NULL, 10, NULL);
  }
  else
  {
    httpd_resp_sendstr(req, "FAIL");
  }

  return ESP_OK;
}

void WiFiAP::mdns_start()
{
  //initialize mDNS
  ESP_ERROR_CHECK(mdns_init());
  ESP_ERROR_CHECK(mdns_hostname_set(ELECTRA_ESP_WIFI_HOSTNAME));
  ESP_ERROR_CHECK(mdns_instance_name_set(ELECTRA_ESP_WIFI_HOSTNAME));

  //initialize web config service
  ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
}

void WiFiAP::rebootTask(void *arg)
{
  vTaskDelay(1000 / portTICK_RATE_MS);
  esp_restart();
  vTaskDelete(NULL);
}