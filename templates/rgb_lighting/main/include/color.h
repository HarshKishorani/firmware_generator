#include <iostream>
#include <string>
using namespace std;

class Color
{
public:
    string hexCode;
    int red;
    int green;
    int blue;

    Color()
    {
        hexCode = "ffffff";
        red = 0;
        blue = 0;
        green = 0;
    }

    Color(string hexCode)
    {
        this->hexCode = hexCode;
        toRGB();
    }

    Color(uint8_t r, uint8_t g, uint8_t b)
    {
        this->red = r;
        this->green = g;
        this->blue = b;
    }

    void toRGB()
    {
        long number = (long)strtol(&hexCode[0], NULL, 16);
        red = number >> 16;
        green = number >> 8 & 0xFF;
        blue = number & 0xFF;
    }
};