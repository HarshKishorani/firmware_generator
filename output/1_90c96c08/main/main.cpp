#include <stdio.h>
#include "include/app_wifi.h"
#include "cJSON.h"
#include "include/node.h"
#include "esp_timer.h"
#include "include/utils.h"
using namespace std;

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define BOOT_PIN GPIO_NUM_0

const char *TAG = "MAIN";
const char *AWS_TAG = "AWS IOT";

AWS_IoT_Client client;
char HostAddress[255] = AWS_IOT_MQTT_HOST;
uint32_t port = AWS_IOT_MQTT_PORT;

char *thing_name;
string switchType;

switch_device_config device_config;
Node nodes;

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Boot Pin Interrupt task ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// TODO: Write reset functions

SemaphoreHandle_t xSemaphore = NULL;

// interrupt service routine, called when the button is pressed
void IRAM_ATTR gpio_interrupt_handler(void *arg)
{
    // notify the button task
    xSemaphoreGiveFromISR(xSemaphore, NULL);
}
// task that will react to button clicks
void BOOT_Button_Task(void *arg)
{
    while (true)
    {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
        {
            uint64_t begin_timer = esp_timer_get_time();
            uint64_t end_timer = esp_timer_get_time();
            ESP_LOGE(TAG, "Start Time is : %llu", begin_timer / 1000000);
            while (gpio_get_level(BOOT_PIN) != 1)
            {
                ESP_LOGE(TAG, "Current Time : %llu", esp_timer_get_time() - begin_timer / 1000000);
            }
            end_timer = esp_timer_get_time();
            ESP_LOGI(TAG, "Seconds = %llu", (end_timer - begin_timer) / 1000000);

            if ((end_timer - begin_timer) / 1000000 >= 10)
            {
                ESP_LOGE(TAG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FACTORY RESETTING ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
                app_factory_reset(0, 0);
            }
            else if ((end_timer - begin_timer) / 1000000 >= 3)
            {
                ESP_LOGE(TAG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ RESETTING WIFI ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
                app_wifi_reset(0, 0);
            }
            else if ((end_timer - begin_timer) / 1000000 >= 1)
            {
                ESP_LOGE(TAG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ POWER CHANGE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
                // scene.togglePower();
                // ESP_LOGI(TAG, "Power is : %d", scene.power);
            }
            // vTaskDelete(NULL);
        }
    }
}

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Load Certificates ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Get ENV values ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void get_env_data()
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

            device_config.name = name->valuestring;
        }

        cJSON *model = cJSON_GetObjectItem(json, "model");
        if (model->valuestring != NULL)
        {
            printf("Model: %s\n", model->valuestring);
        }

        cJSON *switch_type = cJSON_GetObjectItem(json, "switch_type");
        if (switch_type->valuestring != NULL)
        {
            switchType = switch_type->valuestring;
            printf("Switch Type: %s\n", switch_type->valuestring);
        }

        cJSON *node_size = cJSON_GetObjectItem(json, "node_size");
        device_config.size = node_size->valueint;

        cJSON *relays = cJSON_GetObjectItem(json, "relays");
        for (int i = 0; i < cJSON_GetArraySize(relays); i++)
        {
            cJSON *subitem = cJSON_GetArrayItem(relays, i);
            gpio_num_t pin = (gpio_num_t)subitem->valueint;
            device_config.relays.push_back(pin);
        }

        cJSON *switches = cJSON_GetObjectItem(json, "switches");
        for (int i = 0; i < cJSON_GetArraySize(switches); i++)
        {
            cJSON *subitem = cJSON_GetArrayItem(switches, i);
            gpio_num_t pin = (gpio_num_t)subitem->valueint;
            device_config.switches.push_back(pin);
        }

        cJSON *fan_relays = cJSON_GetObjectItem(json, "fan_relays");
        for (int i = 0; i < cJSON_GetArraySize(fan_relays); i++)
        {
            cJSON *subitem = cJSON_GetArrayItem(fan_relays, i);
            gpio_num_t pin = (gpio_num_t)subitem->valueint;
            device_config.fan_relays.push_back(pin);
        }

        cJSON *fan_switch = cJSON_GetObjectItem(json, "fan_switch");
        gpio_num_t pin = (gpio_num_t)fan_switch->valueint;
        device_config.fan_switch = pin;

        cJSON *fan = cJSON_GetObjectItem(json, "fan");
        device_config.fan = fan->valueint;

        cJSON_Delete(json);
    }
}

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

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ AWS Subscribe Callback Handler ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData)
{
    // Parse json Data : Receive one param per message
    cJSON *json = cJSON_Parse((char *)params->payload);
    cJSON *param = json->child;

    //* Change Values based on params
    if (strcmp(param->string, "switch_1") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new switch_1 Value : %d", param->valueint);
        nodes.set_switch_function(nodes.getRelayGPIO(1), param->valueint);
        aws_publish_bool(&client, param->string, param->valueint);
    }
    else if (strcmp(param->string, "switch_2") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new switch_2 Value : %d", param->valueint);
        nodes.set_switch_function(nodes.getRelayGPIO(2), param->valueint);
        aws_publish_bool(&client, param->string, param->valueint);
    }
    else if (strcmp(param->string, "switch_3") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new switch_3 Value : %d", param->valueint);
        nodes.set_switch_function(nodes.getRelayGPIO(3), param->valueint);
        aws_publish_bool(&client, param->string, param->valueint);
    }
    else if (strcmp(param->string, "switch_4") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new switch_4 Value : %d", param->valueint);
        nodes.set_switch_function(nodes.getRelayGPIO(4), param->valueint);
        aws_publish_bool(&client, param->string, param->valueint);
    }
    else if (strcmp(param->string, "switch_5") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new switch_5 Value : %d", param->valueint);
        nodes.set_switch_function(nodes.getRelayGPIO(5), param->valueint);
        aws_publish_bool(&client, param->string, param->valueint);
    }
    else if (strcmp(param->string, "switch_6") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new switch_6 Value : %d", param->valueint);
        nodes.set_switch_function(nodes.getRelayGPIO(6), param->valueint);
        aws_publish_bool(&client, param->string, param->valueint);
    }
    else if (strcmp(param->string, "fan_speed") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new Fan Speed Value : %d", param->valueint);
        nodes.fan_speed = param->valueint;
        nodes.set_fan(nodes.fanCloudState);
        aws_publish_int(&client, param->string, param->valueint);
    }
    else if (strcmp(param->string, "fan_switch") == 0)
    {
        ESP_LOGI(AWS_TAG, "Received new Fan State Value : %d", param->valueint);
        nodes.fanCloudState = param->valueint;
        nodes.set_fan(nodes.fanCloudState);
        aws_publish_bool(&client, param->string, param->valueint);
    }
    // else if (strcmp(param->string, "All") == 0)
    // {
    //     ESP_LOGI(TAG, "Received value = %s for %s - %s",
    //              val.val.b ? "true" : "false", app_device_get_name(device),
    //              param->string);
    //     aws_publish_bool(&client, param->string, param->valueint);
    // }
    else if (strcmp(param->string, "reboot") == 0)
    {
        aws_publish_bool(&client, param->string, param->valueint);
        if (param->valueint == 1)
        {
            ESP_LOGI(AWS_TAG, "Received Reboot Request.");
            app_reboot(2);
        }
    }
    else if (strcmp(param->string, "wifi_reset") == 0)
    {
        aws_publish_bool(&client, param->string, param->valueint);
        if (param->valueint == 1)
        {
            ESP_LOGI(AWS_TAG, "Received Wifi-Reset Request.");
            app_wifi_reset(2, 5);
        }
    }
    else if (strcmp(param->string, "factory_reset") == 0)
    {
        aws_publish_bool(&client, param->string, param->valueint);
        if (param->valueint == 1)
        {
            ESP_LOGI(AWS_TAG, "Received Factory-Reset Request.");
            app_factory_reset(2, 2);
        }
    }
    cJSON_Delete(json);
}

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ MAIN AWS TASK (Subscribe and endless loop) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void aws_iot_task(void *param)
{
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);
    IoT_Error_t rc = FAILURE;

    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

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

    //* Subscribe to topic to receive updates from cloud
    ESP_LOGI(AWS_TAG, "Subscribing to %s to receive updates.....", SUBSCRIBE_TOPIC);

    rc = aws_iot_mqtt_subscribe(&client, SUBSCRIBE_TOPIC, strlen(SUBSCRIBE_TOPIC), QOS0, iot_subscribe_callback_handler, NULL);
    if (SUCCESS != rc)
    {
        ESP_LOGE(AWS_TAG, "Error subscribing : %d ", rc);
        abort();
    }

    ESP_LOGI(AWS_TAG, "Subscribed");

    //* Main Loop
    if (switchType == "RETRO")
    {
        ESP_LOGE(TAG, "Using Retro Switches.......................");
        while ((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc))
        {
            // Max time the yield function will wait for read messages
            rc = aws_iot_mqtt_yield(&client, 250);
            if (NETWORK_ATTEMPTING_RECONNECT == rc)
            {
                // If the client is attempting to reconnect we will skip the rest of the loop.
                continue;
            }

            if (gpio_get_level(nodes.config.fan_switch) != nodes.fanSwitchState)
            {
                nodes.updateFanState(gpio_get_level(nodes.config.fan_switch));
            }
            for (int i = 1; i <= nodes.config.size; i++)
            {
                vTaskDelay(50 / portTICK_PERIOD_MS);
                if (gpio_get_level(nodes.getSwitchGPIO(i)) != nodes.getDeviceState(i))
                {
                    nodes.updateDeviceState(i, 0, gpio_get_level(nodes.getSwitchGPIO(i)));
                }
            }
        }
    }
    else
    {
        ESP_LOGE(TAG, "Using Touch Switches.......................");
        while ((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc))
        {
            // Max time the yield function will wait for read messages
            rc = aws_iot_mqtt_yield(&client, 250);
            if (NETWORK_ATTEMPTING_RECONNECT == rc)
            {
                // If the client is attempting to reconnect we will skip the rest of the loop.
                continue;
            }

            for (int i = 1; i <= nodes.config.size; i++)
            {
                if (gpio_get_level(nodes.getSwitchGPIO(i)))
                {
                    nodes.updateDeviceState(i, 1);
                    while (gpio_get_level(nodes.getSwitchGPIO(i)))
                    {
                        ESP_LOGE(TAG, "Switch %d : %d", i, gpio_get_level(nodes.getSwitchGPIO(i)));
                    }
                }
            }
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
    }
    ESP_LOGE(AWS_TAG, "An error occurred in the main loop.");
    abort();
}

//* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ APP MAIN ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern "C" void app_main(void)
{
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Get env values and initialize nodes
    get_env_data();
    nodes = Node(&client, device_config);
    // TODO: Get and update values from cloud

    // Factory & Wifi Reset using Boot Pin
    esp_rom_gpio_pad_select_gpio(BOOT_PIN);
    gpio_set_direction(BOOT_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(BOOT_PIN);
    gpio_pullup_dis(BOOT_PIN);
    gpio_set_intr_type(BOOT_PIN, GPIO_INTR_ANYEDGE);

    xSemaphore = xSemaphoreCreateBinary();
    xTaskCreate(BOOT_Button_Task, "BOOT_Button_Task", 5000, NULL, 1, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BOOT_PIN, gpio_interrupt_handler, (void *)BOOT_PIN);

    // Initialize wifi and start aws_iot_task
    initialise_wifi();
    xTaskCreatePinnedToCore(&aws_iot_task, "aws_iot_task", 9216, NULL, 5, NULL, 1);
}