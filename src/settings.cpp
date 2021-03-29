#include "settings.h"

static const char *TAG = "Settings";

bool Settings::ready = false;
uint32_t Settings::handle = 0;
char *Settings::haVersion = NULL;
char *Settings::ssid = NULL;
char *Settings::pass = NULL;
char *Settings::mqttUrl = NULL;
char *Settings::mqttUser = NULL;
char *Settings::mqttPass = NULL;
char *Settings::entity = NULL;

const char *nvs_errors[] = {"OTHER", "NOT_INITIALIZED", "NOT_FOUND", "TYPE_MISMATCH", "READ_ONLY", "NOT_ENOUGH_SPACE", "INVALID_NAME", "INVALID_HANDLE", "REMOVE_FAILED", "KEY_TOO_LONG", "PAGE_FULL", "INVALID_STATE", "INVALID_LENGTH"};
#define nvs_error(e) (((e) > ESP_ERR_NVS_BASE) ? nvs_errors[(e) & ~(ESP_ERR_NVS_BASE)] : nvs_errors[0])

esp_err_t Settings::init()
{
  if (!ready)
  {
    esp_err_t ret;

    // init NVS
    ret = nvs_open("ESP32GW", NVS_READWRITE, &handle);
    if (ret)
    {
      ESP_LOGE(TAG, "nvs_open failed: %s", nvs_error(ret));
      return ret;
    }
    if (!isKey("haVersion")) {
      haVersion = new char[8];
      memcpy(haVersion, "default", 7);
      haVersion[7] = '\0';
    } else {
      size_t verLen = getBytesLength("haVersion") + 1;
      haVersion = new char[verLen];
      getBytes("haVersion", haVersion, verLen - 1);
      haVersion[verLen - 1] = '\0';
    }
    
    // init SPIFFS
    ret = initSPIFFS();
    if (ret != ESP_OK)
    {
      return ret;
    }
    // read and parse config file
    uint8_t configBuffer[ELECTRA_ESP_CONFIG_BUFFER_SIZE] = {0};
    ret = readConfig(configBuffer, ELECTRA_ESP_CONFIG_BUFFER_SIZE);
    if (ret != ESP_OK)
    {
      return ret;
    }

    ready = true;
  }

  return ready ? ESP_OK : ESP_FAIL;
}

char *Settings::getHaVersion()
{
  return haVersion;
}

void Settings::setHaVersion(const char *newVersion) 
{
  size_t verLen = strlen(newVersion) + 1;
  putBytes("haVersion", newVersion, verLen - 1);
  delete[] haVersion;
  haVersion = new char[verLen];
  memcpy(haVersion, newVersion, verLen);
}

char *Settings::getSsid()
{
  return ssid;
}

char *Settings::getPass()
{
  return pass;
}

char *Settings::getMqttUrl()
{
  return mqttUrl;
}

char *Settings::getMqttUser()
{
  return mqttUser;
}

char *Settings::getMqttPass()
{
  return mqttPass;
}

char *Settings::getEntity()
{
  return entity;
}

esp_err_t Settings::initSPIFFS()
{
  ESP_LOGV(TAG, "Mounting SPIFFS");

  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5, // This decides the maximum number of files that can be created on the storage
      .format_if_mount_failed = true};

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS [%s]", esp_err_to_name(ret));
    }
    return ret;
  }

  ESP_LOGV(TAG, "SPIFFS mounted");

#if CONFIG_LOG_DEFAULT_LEVEL >= ESP_LOG_DEBUG
  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information [%s]", esp_err_to_name(ret));
    return false;
  }

  ESP_LOGD(TAG, "Partition size: total: %d, used: %d", total, used);
#endif

  return ESP_OK;
}

esp_err_t Settings::readConfig(uint8_t *buffer, size_t bufferLen)
{
  ESP_LOGV(TAG, "Opening settings file");

  FILE *fd = nullptr;
  fd = fopen(ELECTRA_ESP_SETTINGS_FILE, "r");
  if (!fd)
  {
    ESP_LOGE(TAG, "Failed to read [%s] from SPIFFS", ELECTRA_ESP_SETTINGS_FILE);
    return ESP_FAIL;
  }

  size_t fileLen = fread(buffer, 1, bufferLen, fd);
  fclose(fd);

  if (fileLen > 0)
  {
    ESP_LOGV(TAG, "Reading settings file");
    cJSON *root = cJSON_Parse((char *)buffer);
    // get wifi ssid
    if (getConfigVar(root, "ssid", ssid) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'ssid'");
      return ESP_FAIL;
    }
    // get wifi password
    if (getConfigVar(root, "pass", pass) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'pass'");
      return ESP_FAIL;
    }
    // get ha mqtt url
    if (getConfigVar(root, "mqttUrl", mqttUrl) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'mqttUrl'");
      return ESP_FAIL;
    }
    // get ha mqtt username
    if (getConfigVar(root, "mqttUser", mqttUser) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'mqttUser'");
      return ESP_FAIL;
    }
    // get ha mqtt password
    if (getConfigVar(root, "mqttPass", mqttPass) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'mqttPass'");
      return ESP_FAIL;
    }
    // get ha entity
    if (getConfigVar(root, "entity", entity) != ESP_OK)
    {
      ESP_LOGE(TAG, "Config file missing 'entity'");
      return ESP_FAIL;
    }

    ESP_LOGV(TAG, "Read settings file completed");

#if CONFIG_LOG_DEFAULT_LEVEL >= ESP_LOG_DEBUG
    ESP_LOGD(TAG, "Read settings:\nssid: [%s]\npass: [%s]\nmqtt URL: [%s]\nmqtt User: [%s]\nmqtt Password: [%s]",
             ssid, pass, mqttUrl, mqttUser, mqttPass);
#endif

    return ESP_OK;
  }
  else
  {
    ESP_LOGE(TAG, "Config file is empty");
    return ESP_FAIL;
  }
}

esp_err_t Settings::getConfigVar(cJSON *root, const char *name, char *&var)
{
  if (cJSON_HasObjectItem(root, name))
  {
    cJSON *objJSON = cJSON_GetObjectItem(root, name);
    size_t objLen = strlen(objJSON->valuestring);
    var = new char[objLen + 1];
    memcpy(var, objJSON->valuestring, objLen);
    var[objLen] = '\0';
    return ESP_OK;
  }
  else
  {
    return ESP_FAIL;
  }
}

PreferenceType Settings::getType(const char *key)
{
  if (!key || strlen(key) > 15)
  {
    return PT_INVALID;
  }
  int8_t mt1;
  uint8_t mt2;
  int16_t mt3;
  uint16_t mt4;
  int32_t mt5;
  uint32_t mt6;
  int64_t mt7;
  uint64_t mt8;
  size_t len = 0;
  if (nvs_get_i8(handle, key, &mt1) == ESP_OK)
    return PT_I8;
  if (nvs_get_u8(handle, key, &mt2) == ESP_OK)
    return PT_U8;
  if (nvs_get_i16(handle, key, &mt3) == ESP_OK)
    return PT_I16;
  if (nvs_get_u16(handle, key, &mt4) == ESP_OK)
    return PT_U16;
  if (nvs_get_i32(handle, key, &mt5) == ESP_OK)
    return PT_I32;
  if (nvs_get_u32(handle, key, &mt6) == ESP_OK)
    return PT_U32;
  if (nvs_get_i64(handle, key, &mt7) == ESP_OK)
    return PT_I64;
  if (nvs_get_u64(handle, key, &mt8) == ESP_OK)
    return PT_U64;
  if (nvs_get_str(handle, key, NULL, &len) == ESP_OK)
    return PT_STR;
  if (nvs_get_blob(handle, key, NULL, &len) == ESP_OK)
    return PT_BLOB;
  return PT_INVALID;
}

bool Settings::isKey(const char *key)
{
  return getType(key) != PT_INVALID;
}

size_t Settings::putBytes(const char *key, const void *value, size_t len)
{
  if (!key || !value || !len)
  {
    return 0;
  }
  esp_err_t err = nvs_set_blob(handle, key, value, len);
  if (err)
  {
    ESP_LOGE(TAG, "nvs_set_blob fail: %s %s", key, nvs_error(err));
    return 0;
  }
  err = nvs_commit(handle);
  if (err)
  {
    ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
    return 0;
  }
  return len;
}

size_t Settings::getBytesLength(const char *key)
{
  size_t len = 0;
  if (!key)
  {
    return 0;
  }
  esp_err_t err = nvs_get_blob(handle, key, NULL, &len);
  if (err)
  {
    ESP_LOGE(TAG, "nvs_get_blob len fail: %s %s", key, nvs_error(err));
    return 0;
  }
  return len;
}

size_t Settings::getBytes(const char *key, void *buf, size_t maxLen)
{
  size_t len = getBytesLength(key);
  if (!len || !buf || !maxLen)
  {
    return len;
  }
  if (len > maxLen)
  {
    ESP_LOGE(TAG, "not enough space in buffer: %u < %u", maxLen, len);
    return 0;
  }
  esp_err_t err = nvs_get_blob(handle, key, buf, &len);
  if (err)
  {
    ESP_LOGE(TAG, "nvs_get_blob fail: %s %s", key, nvs_error(err));
    return 0;
  }
  return len;
}

size_t Settings::getConfigFile(uint8_t *buffer) {
  FILE *fd = nullptr;
  fd = fopen(ELECTRA_ESP_SETTINGS_FILE, "r");
  if (!fd)
  {
    ESP_LOGI(TAG, "Providing default config file");
    size_t fileLen = strlen(ELECTRA_ESP_DEFAULT_CONFIG);
    memcpy(buffer, ELECTRA_ESP_DEFAULT_CONFIG, fileLen);
    buffer[fileLen] = '\0';
    return fileLen;
  } else {
    size_t fileLen = fread(buffer, 1, ELECTRA_ESP_CONFIG_BUFFER_SIZE, fd);
    fclose(fd);
    return fileLen;
  }
}

esp_err_t Settings::putConfigFile(uint8_t *buffer, size_t bufferLen) {
  FILE *fd = nullptr;
  fd = fopen(ELECTRA_ESP_SETTINGS_FILE, "w");
  if (!fd)
  {
    return ESP_FAIL;
  } else {
    fwrite(buffer, 1, bufferLen, fd);
    fclose(fd);
    return ESP_OK;
  }
  return ESP_OK;
}