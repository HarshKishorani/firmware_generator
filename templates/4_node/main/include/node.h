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
    char *SUBSCRIBE_TOPIC;
    char *PUBLISH_TOPIC;

} switch_device_config;

switch_device_config device_config;

class Node
{
private:
    const char *TAG = "NODE";
    AWS_IoT_Client *client;
    bool fanAdded = false;
    // low, high
    pair<bool, bool> activeState{0, 1};
    bool LastSwitchState[6] = {false, false, false, false, false, false};
    bool CurrentDeviceState[6] = {false, false, false, false, false, false};

public:
    switch_device_config config;
    int fan_speed = 2;
    bool fanSwitchState = true;
    bool fanCloudState = true;

    //* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FAN ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // TODO : Update fan functions accoring to active High
    void addFan()
    {
        if (this->config.size <= 4)
        {
            for (gpio_num_t relay : config.fan_relays)
            {
                gpio_set_direction(relay, GPIO_MODE_OUTPUT);
            }

            gpio_set_direction(config.fan_switch, GPIO_MODE_INPUT); // gpio 34-39 pullup can't work
            // gpio_set_pull_mode(config.fan_switch, GPIO_PULLUP_ONLY);
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
            aws_publish_bool(client, device_config.PUBLISH_TOPIC, "fan_switch", !state);
        }
        else
        {
            ESP_LOGE(TAG, "Fan not available");
        }
    }

    bool getFanCloudState()
    {
        return fanCloudState;
    }

    int getFanSpeed()
    {
        return fan_speed;
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
    
    /// @brief Get Switch GPIO. (Use 1 based indexing)
    /// @param switchNumber 
    /// @return 
    gpio_num_t getSwitchGPIO(int switchNumber)
    {
        if (switchNumber > config.size)
        {
            ESP_LOGE(TAG, "Switch %d doesn't exist.", switchNumber);
            abort();
        }
        return config.switches[switchNumber - 1];
    }

    /// @brief Get Relay GPIO. (Use 1 based indexing)
    /// @param switchNumber 
    /// @return 
    gpio_num_t getRelayGPIO(int switchNumber)
    {
        if (switchNumber > config.size)
        {
            ESP_LOGE(TAG, "Switch %d doesn't exist.", switchNumber);
            abort();
        }
        return config.relays[switchNumber - 1];
    }

    /// @brief Get Last Switch State. (Use 1 based indexing)
    /// @param switchNumber 
    /// @return 
    bool getLastSwitchState(int switchNumber)
    {
        if (switchNumber > config.size)
        {
            ESP_LOGE(TAG, "Switch %d doesn't exist.", switchNumber);
            abort();
        }
        return LastSwitchState[switchNumber - 1];
    }

    /// @brief Update using Cloud and local control. Sets gpio on or off.
    /// @param switchNumber
    /// @param state
    void setDeviceState(int switchNumber, bool state)
    {
        CurrentDeviceState[switchNumber - 1] = state;
        int deviceActiveState = state == 0 ? activeState.first : activeState.second;
        ESP_LOGI(TAG, "SET SWITCH STATE :~ %d PIN STATE :~ %d ACTIVE :~ %d\n", switchNumber, state,deviceActiveState);
        gpio_set_level(getRelayGPIO(switchNumber), deviceActiveState);
    }

    /// @brief Get current GPIO value of switch.
    /// @param switchNumber
    int getDeviceState(int switchNumber)
    {
        int state = CurrentDeviceState[switchNumber - 1];
        ESP_LOGD(TAG, "GET SWITCH STATE :~ %d PIN STATE :~ %d \n", switchNumber, state);
        return state;
    }

    /// @brief Update Last Switch State
    /// @param switchNumber 
    /// @param touch 
    /// @param state 
    void updateSwitchState(int switchNumber, bool touch = false, bool state = true)
    {
        if (switchNumber > config.size)
        {
            ESP_LOGE(TAG, "Switch %d doesn't exist.", switchNumber);
            abort();
        }
        if (touch)
        {
            LastSwitchState[switchNumber - 1] = !LastSwitchState[switchNumber - 1];
            ESP_LOGI(TAG, "Switch %d updated to %d.", switchNumber, !LastSwitchState[switchNumber - 1]);
            setDeviceState(switchNumber, LastSwitchState[switchNumber - 1]);
            char *switch_name = (char *)malloc(100);
            sprintf(switch_name, "switch_%d", switchNumber);
            aws_publish_bool(client, device_config.PUBLISH_TOPIC, switch_name, LastSwitchState[switchNumber - 1]);
            free(switch_name);
        }
        else
        {
            LastSwitchState[switchNumber - 1] = state;
            ESP_LOGI(TAG, "Switch State of %d updated to %d.", switchNumber, LastSwitchState[switchNumber - 1]);
            setDeviceState(switchNumber, LastSwitchState[switchNumber - 1]);
            char *switch_name = (char *)malloc(100);
            sprintf(switch_name, "switch_%d", switchNumber);
            aws_publish_bool(client, device_config.PUBLISH_TOPIC, switch_name, LastSwitchState[switchNumber - 1]);
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
            // gpio_set_pull_mode(this->config.switches[i], GPIO_PULLUP_ONLY);

            // Get SwitchStates for LastSwitchStates
            LastSwitchState[i] = gpio_get_level(getSwitchGPIO(i + 1));
        }

        // if active low
        if (config.active == false)
        {
            activeState = {1, 0};
        }

        // TODO : Include Fan
        // if (config.fan)
        // {
        //     addFan();
        // }
    }
};
