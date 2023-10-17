#include <iostream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/gpio.h"
#include <string>
#include <vector>
#include <utility>
#include "include/publish.h"

using namespace std;

typedef struct switch_device_config
{
    string name;
    int size;
    bool fan = false;
    bool active = true;
    vector<gpio_num_t> relays;
    vector<gpio_num_t> switches;
    vector<gpio_num_t> fan_relays;
    gpio_num_t fan_switch;

} switch_device_config;

class Node
{
private:
    const char *TAG = "NODE";
    AWS_IoT_Client *client;
    bool fanAdded = false;
    pair<bool, bool> activeState{0, 1};
    bool SwitchState[6] = {true, true, true, true, true, true};

public:
    switch_device_config config;
    int fan_speed = 2;
    bool fanSwitchState = true;
    bool fanCloudState = true;

    //* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FAN ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    void addFan()
    {
        if (this->config.size <= 4)
        {
            for (gpio_num_t relay : config.fan_relays)
            {
                gpio_set_direction(relay, GPIO_MODE_OUTPUT);
            }

            gpio_set_direction(config.fan_switch, GPIO_MODE_INPUT); // gpio 34-39 pullup can't work
            gpio_set_pull_mode(config.fan_switch, GPIO_PULLUP_ONLY);
            fanAdded = true;
            ESP_LOGI(TAG, "Fan Added : Relay1{GPIO_NUM_%d}, Relay2{GPIO_NUM_%d}, Relay3{GPIO_NUM_%d}, Switch{GPIO_NUM_%d}.", this->config.fan_relays[0], this->config.fan_relays[1], this->config.fan_relays[2], this->config.fan_switch);
        }
        else
        {
            ESP_LOGE(TAG, "Max Pins used. Fan not created");
        }
    }

    void updateFanState(bool state)
    {
        if (fanAdded)
        {
            if (state)
            {
                for (gpio_num_t relay : config.fan_relays)
                {
                    gpio_set_level(relay, activeState.first);
                }
            }
            else
            {
                set_fan(true);
            }
            fanSwitchState = state;
            fanCloudState = !state;
            aws_publish_bool(client, "fan_switch", !state);
        }
        else
        {
            ESP_LOGE(TAG, "Fan not available");
        }
    }

    void set_fan(bool state = true)
    {
        if (fanAdded)
        {
            if (state)
            {
                switch (fan_speed)
                {
                case 1:
                    gpio_set_level(config.fan_relays[0], activeState.second);
                    gpio_set_level(config.fan_relays[1], activeState.first);
                    gpio_set_level(config.fan_relays[2], activeState.first);
                    break;
                case 2:
                    gpio_set_level(config.fan_relays[0], activeState.first);
                    gpio_set_level(config.fan_relays[1], activeState.second);
                    gpio_set_level(config.fan_relays[2], activeState.first);
                    break;
                case 3:
                    gpio_set_level(config.fan_relays[0], activeState.second);
                    gpio_set_level(config.fan_relays[1], activeState.second);
                    gpio_set_level(config.fan_relays[2], activeState.first);
                    break;
                case 4:
                    gpio_set_level(config.fan_relays[0], activeState.first);
                    gpio_set_level(config.fan_relays[1], activeState.first);
                    gpio_set_level(config.fan_relays[2], activeState.second);
                    break;
                default:
                    gpio_set_level(config.fan_relays[0], activeState.first);
                    gpio_set_level(config.fan_relays[1], activeState.first);
                    gpio_set_level(config.fan_relays[2], activeState.first);
                    break;
                }
            }
            else
            {
                for (gpio_num_t relay : this->config.fan_relays)
                {
                    gpio_set_level(relay, activeState.first);
                }
            }
        }
        else
        {
            ESP_LOGE(TAG, "Fan not available");
        }
    }

    //* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Nodes and Switches ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    void set_switch_function(gpio_num_t pin, bool state)
    {
        ESP_LOGD(TAG, "PIN NUMBER :~ %d \n PIN STATE :~ %d \n", pin, state);
        gpio_set_level(pin, state);
    }

    gpio_num_t getSwitchGPIO(int switchNumber)
    {
        if (switchNumber > config.size)
        {
            ESP_LOGE(TAG, "Switch %d doesn't exist.", switchNumber);
            abort();
        }
        return config.switches[switchNumber - 1];
    }

    gpio_num_t getRelayGPIO(int switchNumber)
    {
        if (switchNumber > config.size)
        {
            ESP_LOGE(TAG, "Switch %d doesn't exist.", switchNumber);
            abort();
        }
        return config.relays[switchNumber - 1];
    }

    bool getDeviceState(int switchNumber)
    {
        if (switchNumber > config.size)
        {
            ESP_LOGE(TAG, "Switch %d doesn't exist.", switchNumber);
            abort();
        }
        return SwitchState[switchNumber - 1];
    }

    void updateDeviceState(int switchNumber, bool touch = false, bool state = true)
    {
        if (switchNumber > config.size)
        {
            ESP_LOGE(TAG, "Switch %d doesn't exist.", switchNumber);
            abort();
        }
        if (touch)
        {
            SwitchState[switchNumber - 1] = !SwitchState[switchNumber - 1];
            ESP_LOGI(TAG, "Switch %d updated to %d.", switchNumber, !SwitchState[switchNumber - 1]);
            set_switch_function(config.relays[switchNumber - 1], SwitchState[switchNumber - 1]);
            char *switch_name = (char *)malloc(100);
            sprintf(switch_name, "switch_%d", switchNumber);
            aws_publish_bool(client, switch_name, SwitchState[switchNumber - 1]);
            free(switch_name);
        }
        else
        {
            SwitchState[switchNumber - 1] = state;
            ESP_LOGI(TAG, "Switch %d updated to %d.", switchNumber, !SwitchState[switchNumber - 1]);
            set_switch_function(config.relays[switchNumber - 1], SwitchState[switchNumber - 1]);
            char *switch_name = (char *)malloc(100);
            sprintf(switch_name, "switch_%d", switchNumber);
            aws_publish_bool(client, switch_name, SwitchState[switchNumber - 1]);
            free(switch_name);
        }
    }

    //* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Constructor ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Node() {}
    Node(AWS_IoT_Client *pClient, switch_device_config config)
    {
        this->client = pClient;
        this->config = config;
        if (config.size > 6)
        {
            this->config.size = 6;
            ESP_LOGE(TAG, "Node size cannot be greater than 6; Setting Node size to 6.");
        }
        for (int i = 0; i < this->config.size; i++)
        {
            ESP_LOGI(TAG, "Switch No. %d : Relay{GPIO_NUM_%d}, Switch{GPIO_NUM_%d}.", i + 1, this->config.relays[i], this->config.switches[i]);

            gpio_set_direction(this->config.relays[i], GPIO_MODE_OUTPUT);
            gpio_set_direction(this->config.switches[i], GPIO_MODE_INPUT);
            gpio_set_pull_mode(this->config.switches[i], GPIO_PULLUP_ONLY);
        }

        // if active low
        if (config.active == false)
        {
            activeState = {1, 0};
        }

        // Include Fan
        if (config.fan)
        {
            addFan();
        }
    }
};
