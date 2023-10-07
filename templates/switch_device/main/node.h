#include <iostream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "driver/gpio.h"
#include <string>
#include <vector>

#include <utility>

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
            // TODO : Add Fan to cloud
            // fan_switch = esp_rmaker_param_create("fan_switch", NULL, esp_rmaker_bool(0), PROP_FLAG_READ | PROP_FLAG_WRITE | PROP_FLAG_PERSIST);
            // esp_rmaker_param_add_ui_type(fan_switch, ESP_RMAKER_UI_TOGGLE);
            // esp_rmaker_device_add_param(this->device, fan_switch);

            // fan = esp_rmaker_param_create("Fan", NULL, esp_rmaker_int(1), PROP_FLAG_READ | PROP_FLAG_WRITE | PROP_FLAG_PERSIST);
            // esp_rmaker_param_add_ui_type(fan, ESP_RMAKER_UI_SLIDER);
            // esp_rmaker_param_add_bounds(fan, esp_rmaker_int(1), esp_rmaker_int(4), esp_rmaker_int(1));
            // esp_rmaker_device_add_param(this->device, fan);

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
            // esp_rmaker_param_update_and_notify(fan_switch, esp_rmaker_bool(!state));
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
            // esp_rmaker_param_update_and_notify(parameters[switchNumber - 1], esp_rmaker_bool(SwitchState[switchNumber - 1]));
        }
        else
        {
            SwitchState[switchNumber - 1] = state;
            ESP_LOGI(TAG, "Switch %d updated to %d.", switchNumber, !SwitchState[switchNumber - 1]);
            set_switch_function(config.relays[switchNumber - 1], SwitchState[switchNumber - 1]);
            // esp_rmaker_param_update_and_notify(parameters[switchNumber - 1], esp_rmaker_bool(!SwitchState[switchNumber - 1]));
        }
    }

    //* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Constructor ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Node() {}
    Node(switch_device_config config)
    {
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

        // TODO: Cloud Functions
        // ESP_LOGE(TAG, "Name : %s", config.name);
        /*ESP_LOGE(TAG, "Size : %d", config.size);
        ESP_LOGE(TAG, "Fan : %s", config.fan ? "True" : "False");
        ESP_LOGE(TAG, "Active : %s", config.active ? "High" : "Low");

        ESP_LOGI(TAG, "Relay GPIOs : ");
        for (gpio_num_t relay_gpio : config.relays)
        {
            ESP_LOGE(TAG, "GPIO_NUM_%d", relay_gpio);
        }

        ESP_LOGI(TAG, "Switch GPIOs : ");
        for (gpio_num_t switch_gpio : config.switches)
        {
            ESP_LOGE(TAG, "GPIO_NUM_%d", switch_gpio);
        }

        ESP_LOGI(TAG, "Fan Relay GPIOs : ");
        for (gpio_num_t fan_relay_gpio : config.fan_relays)
        {
            ESP_LOGE(TAG, "GPIO_NUM_%d", fan_relay_gpio);
        }

        ESP_LOGE(TAG, "Fan Switch GPIO: GPIO_NUM_%d", config.fan_switch);*/
    }
};
