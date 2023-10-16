#include <iostream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <string>
#include <vector>
#include <utility>
#include "include/publish.h"

using namespace std;

typedef struct lighting_device_config
{
    string name;
    gpio_num_t brightness_gpio;
    gpio_num_t color_temp_gpio;

} lighting_device_config;

class Lighting
{
private:
    const char *TAG = "LIGHTING";
    AWS_IoT_Client *client;

    uint8_t brightness;
    uint8_t color_temp;
    bool status = true;

public:
    lighting_device_config config;

    //* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Controls ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    void updateStatus(bool status)
    {
        this->status = status;
    }

    void updateBrightness(uint8_t brightness)
    {
        this->brightness = brightness;
    }

    void updateColorTemp(uint8_t color_temp)
    {
        this->color_temp = color_temp;
    }

    void run()
    {
        ESP_LOGE(TAG, "Status : %d, Brightness : %d, Temprature : %d", status, brightness, color_temp);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, brightness);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, color_temp);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    }

    //* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Constructor ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Lighting() {}
    Lighting(AWS_IoT_Client *pClient, lighting_device_config config)
    {
        this->client = pClient;
        this->config = config;

        ledc_timer_config_t timer_conf = {
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_8_BIT,
            .timer_num = LEDC_TIMER_0,
            .freq_hz = 1000, // Set PWM frequency to 1 kHz
            .clk_cfg = LEDC_AUTO_CLK
        };
        ledc_timer_config(&timer_conf);

        ledc_channel_config_t channel_conf = {
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint = 0};

        channel_conf.timer_sel = LEDC_TIMER_0;
        channel_conf.channel = LEDC_CHANNEL_0;
        channel_conf.gpio_num = config.brightness_gpio;
        ledc_channel_config(&channel_conf);

        channel_conf.channel = LEDC_CHANNEL_1;
        channel_conf.gpio_num = config.color_temp_gpio;
        ledc_channel_config(&channel_conf);
    }
};