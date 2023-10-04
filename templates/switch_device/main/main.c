#include <stdio.h>
#include "wifi.c"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include <string.h>

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const char *TAG = "MAIN";
const char *AWS_TAG = "AWS IOT";

char HostAddress[255] = AWS_IOT_MQTT_HOST;
uint32_t port = AWS_IOT_MQTT_PORT;

char *thing_name;

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Certificates ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ROOT CA CERTIFICATE
extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");

// HTTPS CERTIFICATE
// extern const uint8_t https_certificate_pem_start[] asm("_binary_https_certificate_pem_start");
// extern const uint8_t https_certificate_pem_end[] asm("_binary_https_certificate_pem_end");

// DEVICE CERTIFICATE
extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_pem_crt_end[] asm("_binary_certificate_pem_crt_end");

// PRIVATE KEY CERTIFICATE
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[] asm("_binary_private_pem_key_end");

// DATA
extern const uint8_t data_start[] asm("_binary_data_json_start");
extern const uint8_t data_end[] asm("_binary_data_json_end");

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ AWS Disconnect Callback Handler ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data)
{
    ESP_LOGW(AWS_TAG, "MQTT Disconnect");
    IoT_Error_t rc = FAILURE;

    if (NULL == pClient)
    {
        return;
    }

    if (aws_iot_is_autoreconnect_enabled(pClient))
    {
        ESP_LOGI(AWS_TAG, "Auto Reconnect is enabled, Reconnecting attempt will start now");
    }
    else
    {
        ESP_LOGW(AWS_TAG, "Auto Reconnect not enabled. Starting manual reconnect...");
        rc = aws_iot_mqtt_attempt_reconnect(pClient);
        if (NETWORK_RECONNECTED == rc)
        {
            ESP_LOGW(AWS_TAG, "Manual Reconnect Successful");
        }
        else
        {
            ESP_LOGW(AWS_TAG, "Manual Reconnect Failed - %d", rc);
        }
    }
}

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Subscribe Callback Handler ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData)
{
    // Parse json Data : Receive one param per message
    cJSON *json = cJSON_Parse((char *)params->payload);
    cJSON *param = json->child;

    //* Change Values based on params
    if (strcmp(param->string, "Switch_1") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new Switch_1 Value : %d", param->valueint);
        // nodes.set_switch_function(nodes.getRelayGPIO(1), val.val.b);
        // esp_rmaker_param_update_and_report(param->string, val);
    }
    else if (strcmp(param->string, "Switch_2") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new Switch_2 Value : %d", param->valueint);
        // nodes.set_switch_function(nodes.getRelayGPIO(2), val.val.b);
        // esp_rmaker_param_update_and_report(param->string, val);
    }
    else if (strcmp(param->string, "Switch_3") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new Switch_3 Value : %d", param->valueint);
        // nodes.set_switch_function(nodes.getRelayGPIO(3), val.val.b);
        // esp_rmaker_param_update_and_report(param->string, val);
    }
    else if (strcmp(param->string, "Switch_4") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new Switch_4 Value : %d", param->valueint);
        // nodes.set_switch_function(nodes.getRelayGPIO(4), val.val.b);
        // esp_rmaker_param_update_and_report(param->string, val);
    }
    else if (strcmp(param->string, "Switch_5") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new Switch_5 Value : %d", param->valueint);
        // nodes.set_switch_function(nodes.getRelayGPIO(5), val.val.b);
        // esp_rmaker_param_update_and_report(param->string, val);
    }
    else if (strcmp(param->string, "Switch_6") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new Switch_6 Value : %d", param->valueint);
        // nodes.set_switch_function(nodes.getRelayGPIO(6), val.val.b);
        // esp_rmaker_param_update_and_report(param->string, val);
    }
    else if (strcmp(param->string, "Fan") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new Fan Speed Value : %d", param->valueint);
        // nodes.fan_speed = val.val.i;
        // nodes.set_fan(nodes.fanCloudState);
        // esp_rmaker_param_update_and_report(param->string, val);
    }
    else if (strcmp(param->string, "fan_switch") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new Fan State Value : %d", param->valueint);
        // nodes.fanCloudState = val.val.b;
        // nodes.set_fan(nodes.fanCloudState);
        // esp_rmaker_param_update_and_report(param->string, val);
    }
    // else if (strcmp(param->string, "All") == 0)
    // {
    //     ESP_LOGI(TAG, "Received value = %s for %s - %s",
    //              val.val.b ? "true" : "false", esp_rmaker_device_get_name(device),
    //              param->string);
    //     esp_rmaker_param_update_and_report(param->string, val);

    // cJSON_Delete(param);
    // cJSON_Delete(json);
    // }
    else if (strcmp(param->string, "Reboot") == 0)
    {
        if (param->valueint == 1)
        {
            ESP_LOGI(AWS_TAG, "Received Reboot Request.");
            // esp_rmaker_reboot(2);
        }
    }
    else if (strcmp(param->string, "Wifi-Reset") == 0)
    {
        if (param->valueint == 1)
        {
            ESP_LOGI(AWS_TAG, "Received Wifi-Reset Request.");
            // esp_rmaker_wifi_reset(2, 2);
        }
    }
    else if (strcmp(param->string, "Factory-Reset") == 0)
    {
        if (param->valueint == 1)
        {
            ESP_LOGI(AWS_TAG, "Received Factory-Reset Request.");
            // esp_rmaker_factory_reset(2, 2);
        }
    }
    cJSON_Delete(json);
}

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ MAIN AWS TASK ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const char *SUBSCRIBE_TOPIC = "MyESP32/sub";
const char *PUBLISH_TOPIC = "MyESP32/pub";

void aws_iot_task(void *param)
{
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);
    IoT_Error_t rc = FAILURE;

    AWS_IoT_Client client;
    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

    // IoT_Publish_Message_Params paramsQOS0;
    IoT_Publish_Message_Params params;

    ESP_LOGI(AWS_TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    //* Configuring MQTT Init Parameters
    mqttInitParams.enableAutoReconnect = false;
    mqttInitParams.pHostURL = HostAddress;
    mqttInitParams.port = port;

    mqttInitParams.pRootCALocation = (const char *)aws_root_ca_pem_start;
    mqttInitParams.pDeviceCertLocation = (const char *)certificate_pem_crt_start;
    mqttInitParams.pDevicePrivateKeyLocation = (const char *)private_pem_key_start;

    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnectCallbackHandler;
    mqttInitParams.disconnectHandlerData = NULL;

    rc = aws_iot_mqtt_init(&client, &mqttInitParams);
    if (SUCCESS != rc)
    {
        ESP_LOGE(AWS_TAG, "aws_iot_mqtt_init returned error : %d ", rc);
        abort();
    }

    //* Configuring Connect Parameters
    connectParams.keepAliveIntervalInSec = 10;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    /* Client ID is set in the menuconfig of the example */
    connectParams.pClientID = thing_name;
    connectParams.clientIDLen = (uint16_t)strlen(thing_name);
    connectParams.isWillMsgPresent = false;

    ESP_LOGI(AWS_TAG, "Connecting to AWS...");
    do
    {
        rc = aws_iot_mqtt_connect(&client, &connectParams);
        if (SUCCESS != rc)
        {
            ESP_LOGE(AWS_TAG, "Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    } while (SUCCESS != rc);

    /*
      Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
      #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
      #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
    */
    rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
    if (SUCCESS != rc)
    {
        ESP_LOGE(AWS_TAG, "Unable to set Auto Reconnect to true - %d", rc);
        abort();
    }

    // TODO: Have seperate Sub and Pub topic for device
    //* 1. Subscribe to topic to receive updates from cloud
    ESP_LOGI(AWS_TAG, "Subscribing to %s to receive updates.....", SUBSCRIBE_TOPIC);

    rc = aws_iot_mqtt_subscribe(&client, SUBSCRIBE_TOPIC, strlen(SUBSCRIBE_TOPIC), QOS0, iot_subscribe_callback_handler, NULL);
    if (SUCCESS != rc)
    {
        ESP_LOGE(AWS_TAG, "Error subscribing : %d ", rc);
        abort();
    }

    ESP_LOGI(AWS_TAG, "Subscribed");

    //* Publish Data Payload
    char cPayload[100];
    params.qos = QOS1;
    params.payload = (void *)cPayload;
    params.isRetained = 0;

    while ((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc))
    {

        // Max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(&client, 100);
        if (NETWORK_ATTEMPTING_RECONNECT == rc)
        {
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }

        ESP_LOGI(AWS_TAG, "Stack remaining for task '%s' is %d bytes", pcTaskGetName(NULL), uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // TODO : Message to publish
        sprintf(cPayload, "{\"power\" : \"1234\",\"timestamp\" : \"98953155\"}");
        params.payloadLen = strlen(cPayload);
        rc = aws_iot_mqtt_publish(&client, PUBLISH_TOPIC, strlen(PUBLISH_TOPIC), &params);
        if (rc == MQTT_REQUEST_TIMEOUT_ERROR)
        {
            ESP_LOGW(AWS_TAG, "QOS1 publish ack not received.");
            rc = SUCCESS;
        }
    }

    ESP_LOGE(AWS_TAG, "An error occurred in the main loop.");
    abort();
}

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Get ENV values ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void get_env_data()
{
    if (data_start != NULL)
    {
        cJSON *json = cJSON_Parse((char *)data_start);
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
        {
            cJSON *name = cJSON_GetObjectItem(json, "name");
            if (name->valuestring != NULL)
            {
                thing_name = (char *)malloc(strlen(name->valuestring));
                strcpy(thing_name, name->valuestring);
                printf("Name: %s\n", name->valuestring);
            }

            cJSON *model = cJSON_GetObjectItem(json, "model");
            if (model->valuestring != NULL)
            {
                printf("Model: %s\n", model->valuestring);
            }

            cJSON *switch_type = cJSON_GetObjectItem(json, "switch_type");
            if (switch_type->valuestring != NULL)
            {
                printf("Switch Type: %s\n", switch_type->valuestring);
            }

            cJSON *node_size = cJSON_GetObjectItem(json, "node_size");
            printf("Node Size : %d\n", node_size->valueint);

            cJSON *pins = cJSON_GetObjectItem(json, "pins");
            printf("Pins : | ");
            for (int i = 0; i < cJSON_GetArraySize(pins); i++)
            {
                cJSON *subitem = cJSON_GetArrayItem(pins, i);
                printf("%d | ", subitem->valueint);
            }

            cJSON *fan = cJSON_GetObjectItem(json, "fan");
            printf("\nIs Fan : %d\n", fan->valueint);

            cJSON_Delete(json);
        }
    }
}

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ APP MAIN ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void app_main(void)
{
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Get env values

    get_env_data();

    // Initialize Wifi and then run device
    initialise_wifi();
    xTaskCreatePinnedToCore(&aws_iot_task, "aws_iot_task", 9216, NULL, 5, NULL, 1);
}