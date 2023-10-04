#include <stdio.h>
#include "esp_spiffs.h"
#include "esp_log.h"
#include "cJSON.h";

#define TAG "spiffs"

void app_main(void)
{
    esp_vfs_spiffs_conf_t config = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 1,
        .format_if_mount_failed = true,
    };
    esp_vfs_spiffs_register(&config);

    char *buffer = 0;
    long length;
    FILE *file = fopen("/spiffs/data.json", "r");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "File does not exist!");
    }
    else
    {
        ESP_LOGI(TAG, "GOT DATA : ");

        // Get length of file
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Allocate memory
        buffer = malloc(length + 1);

        // Read file
        if (buffer)
        {
            fread(buffer, 1, length, file);
        }
        fclose(file);
        buffer[length] = '\0';
        if (buffer)
        {
            // parse the JSON data
            cJSON *json = cJSON_Parse(buffer);
            if (json == NULL)
            {
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr != NULL)
                {
                    ESP_LOGE(TAG, "Error: %s\n", error_ptr);
                }
                cJSON_Delete(json);
            }
            else
            { // access the JSON data
                cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
                if (cJSON_IsString(name) && (name->valuestring != NULL))
                {
                    ESP_LOGI(TAG, "Name: %s\n", name->valuestring);
                }
                cJSON *age = cJSON_GetObjectItem(json, "age");
                ESP_LOGI(TAG, "Age: %d\n", age->valueint);

                // delete the JSON object
                cJSON_Delete(json);
            }
        }
    }
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(TAG, "DONE.");
}