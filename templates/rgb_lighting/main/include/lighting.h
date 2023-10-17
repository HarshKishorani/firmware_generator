#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <vector>
#include <utility>
#include "include/publish.h"
#include "include/color.h"
#include <cmath>

using namespace std;

typedef struct lighting_device_config
{
    string name;
    gpio_num_t led_red_gpio;
    gpio_num_t led_green_gpio;
    gpio_num_t led_blue_gpio;

} lighting_device_config;

class Lighting
{
private:
    const char *TAG = "LIGHTING";
    AWS_IoT_Client *client;

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK};

    ledc_channel_config_t ledc_channel[3];

    vector<Color> sceneColors;
    void processScene(string &text)
    {
        sceneColors.clear();
        int index = 0;
        while (text.length())
        {
            string code = text.substr(0, 6);
            Color color(code);
            sceneColors.push_back(color);
            text.erase(0, 6);
            sceneColors[index].toRGB();
            index++;
        }
    }

    void off()
    {
        uint32_t red_s = 0;
        uint32_t green_s = 0;
        uint32_t blue_s = 0;
        ledc_fade_func_install(ESP_INTR_FLAG_LEVEL1);
        ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, red_s, 1000));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, green_s, 1000));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
        ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, blue_s, 1000));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));

        ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
        ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, LEDC_FADE_NO_WAIT);
        ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, LEDC_FADE_NO_WAIT);
    }

    int mapSpeed(int input, int input_min, int input_max, int output_min, int output_max)
    {
        int slope = (output_max - output_min) / (input_max - input_min);
        return output_min + round(slope * (input - input_min));
    }

public:
    lighting_device_config config;
    bool power = true;
    bool modeChange = false;
    string text;
    int mode = 2;
    int speed = 450;

    //* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Controls ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    void setState(bool state)
    {
        this->power = state;
    }

    void init()
    {
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
        ledc_channel[0].channel = LEDC_CHANNEL_0;
        ledc_channel[0].gpio_num = config.led_red_gpio;
        ledc_channel[1].channel = LEDC_CHANNEL_1;
        ledc_channel[1].gpio_num = config.led_green_gpio;
        ledc_channel[2].channel = LEDC_CHANNEL_2;
        ledc_channel[2].gpio_num = config.led_blue_gpio;
        for (int i = 0; i < 3; i++)
        {
            ledc_channel[i].speed_mode = LEDC_LOW_SPEED_MODE;
            ledc_channel[i].timer_sel = LEDC_TIMER_0;
            ledc_channel[i].intr_type = LEDC_INTR_DISABLE;
            ledc_channel[i].duty = 0;
            ledc_channel[i].hpoint = 0;

            ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[i]));
        }
    }

    void update(string colors)
    {
        this->text = colors;
        processScene(colors);
    }

    void run()
    {
        if (power)
        {

            switch (mode)
            {
            case 0:
                steady();
                break;
            case 1:
                flash();
                break;
            case 2:
                breath();
                break;
            case 3:
                fade();
                break;
            default:
                steady();
                break;
            }
        }
        else
        {
            cout << "Power is off......." << endl;
            off();
        }
    }

    void updateMode(int mode)
    {
        this->mode = mode;
        this->modeChange = true;
    }
    void updateSpeed(int speed)
    {
        this->speed = mapSpeed(speed, 1, 100, 10000, 1000);
        cout << "speed updated : " << this->speed << endl;
    }

    void togglePower()
    {
        this->power = !this->power;
    }

    //* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Constructor ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Lighting() {}

    Lighting(AWS_IoT_Client *pClient, lighting_device_config config, string colors)
    {
        this->client = pClient;
        this->config = config;

        this->text = colors;
        processScene(colors);
    }

    //* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Modes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void steady()
    {
        if (power && !modeChange)
        {
            cout << "Color "
                 << " : ";
            cout << "Red = " << sceneColors[0].red << " || green = " << sceneColors[0].green << " || blue = " << sceneColors[0].blue << endl;
            uint32_t red_duty = 8191 * sceneColors[0].red / 255;
            uint32_t green_duty = 8191 * sceneColors[0].green / 255;
            uint32_t blue_duty = 8191 * sceneColors[0].blue / 255;
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, red_duty));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, green_duty));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, blue_duty));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));
            vTaskDelay(pdMS_TO_TICKS(speed / 2));
        }
        else
        {
            modeChange = false;
            off();
        }
    }

    void flash()
    {
        for (int index = 0; index < sceneColors.size(); index++)
        {
            if (power && !modeChange)
            {

                cout << "Color " << index << " : ";
                cout << "Red = " << sceneColors[index].red << " || green = " << sceneColors[index].green << " || blue = " << sceneColors[index].blue << endl;
                uint32_t red_duty = 8191 * sceneColors[index].red / 255;
                uint32_t green_duty = 8191 * sceneColors[index].green / 255;
                uint32_t blue_duty = 8191 * sceneColors[index].blue / 255;
                ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, red_duty));
                ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
                ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, green_duty));
                ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
                ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, blue_duty));
                ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));
                vTaskDelay(pdMS_TO_TICKS(speed / 2));
            }
            else
            {
                modeChange = false;
                off();
                break;
            }
        }
    }

    void breath()
    {
        for (int index = 0; index < sceneColors.size(); index++)
        {
            if (power && !modeChange)
            {
                cout << "Color " << index << " : ";
                cout << "Red = " << sceneColors[index].red << " || green = " << sceneColors[index].green << " || blue = " << sceneColors[index].blue << endl;
                uint32_t red_ = 8191 * sceneColors[index].red / 255;
                uint32_t green_ = 8191 * sceneColors[index].green / 255;
                uint32_t blue_ = 8191 * sceneColors[index].blue / 255;
                _set_color(red_, green_, blue_);
            }
            else
            {
                modeChange = false;
                off();
                break;
            }
        }
    }

    void fade()
    {
        for (int index = 0; index < sceneColors.size(); index++)
        {
            if (power && !modeChange)
            {
                cout << "Color " << index << " : ";
                cout << "Red = " << sceneColors[index].red << " || green = " << sceneColors[index].green << " || blue = " << sceneColors[index].blue << endl;
                uint32_t red_ = 8191 * sceneColors[index].red / 255;
                uint32_t green_ = 8191 * sceneColors[index].green / 255;
                uint32_t blue_ = 8191 * sceneColors[index].blue / 255;
                _set_color(red_, green_, blue_);
                _set_color(0, 0, 0);
            }
            else
            {
                modeChange = false;
                off();
                break;
            }
        }
    }

    void _set_color(uint32_t red_s, uint32_t green_s, uint32_t blue_s)
    {
        ledc_fade_func_install(ESP_INTR_FLAG_LEVEL1);
        ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, red_s, speed));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, green_s, speed));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
        ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, blue_s, speed));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));

        ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
        ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, LEDC_FADE_NO_WAIT);
        ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, LEDC_FADE_NO_WAIT);
    }
};