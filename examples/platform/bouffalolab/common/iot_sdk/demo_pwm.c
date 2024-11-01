/**
 * Copyright (c) 2016-2021 Bouffalolab Co., Ltd.
 *
 * Contact information:
 * web site:    https://www.bouffalolab.com/
 */
#include <stdio.h>

#include <hosal_pwm.h>

#include "demo_pwm.h"
#include "mboard.h"
#include <math.h>

#define clamp(a, min, max) ((a) < (min) ? (min) : ((a) > (max) ? (max) : (a)))
#define PWM_FREQ 1000
#define PWM_DUTY_CYCLE 10000

hosal_pwm_dev_t rgb_pwm[] = {

#if MAX_PWM_CHANNEL == 3
    {
        .port = LED_B_PIN_PORT,
        /* pwm config */
        .config.pin        = LED_B_PIN,
        .config.duty_cycle = 0,        // duty_cycle range is 0~10000 correspond to 0~100%
        .config.freq       = PWM_FREQ, // freq range is between 0~40MHZ
    },
    {
        .port = LED_R_PIN_PORT,
        /* pwm config */
        .config.pin        = LED_R_PIN,
        .config.duty_cycle = 0,        // duty_cycle range is 0~10000 correspond to 0~100%
        .config.freq       = PWM_FREQ, // freq range is between 0~40MHZ
    },
    {
        .port = LED_G_PIN_PORT,
        /* pwm config */
        .config.pin        = LED_G_PIN,
        .config.duty_cycle = 0,        // duty_cycle range is 0~10000 correspond to 0~100%
        .config.freq       = PWM_FREQ, // freq range is between 0~40MHZ
    }
#else
    {
        .port = LED_PIN_PORT,
        /* pwm config */
        .config.pin        = LED_PIN,
        .config.duty_cycle = 0,        // duty_cycle range is 0~10000 correspond to 0~100%
        .config.freq       = PWM_FREQ, // freq range is between 0~40MHZ
    },
#endif
};

void demo_pwm_init(void)
{
    /* init pwm with given settings */
    for (uint32_t i = 0; i < MAX_PWM_CHANNEL; i++)
    {
        hosal_pwm_init(rgb_pwm + i);
    }
}

void demo_pwm_start(void)
{
    /* start pwm */
    for (uint32_t i = 0; i < MAX_PWM_CHANNEL; i++)
    {
        hosal_pwm_start(rgb_pwm + i);
    }
}

void demo_pwm_change_param(hosal_pwm_config_t * para)
{
    /* change pwm param */
    for (uint32_t i = 0; i < MAX_PWM_CHANNEL; i++)
    {
        if (para[i].duty_cycle > PWM_DUTY_CYCLE)
        {
            para[i].duty_cycle = PWM_DUTY_CYCLE;
        }
        hosal_pwm_para_chg(rgb_pwm + i, para[i]);
    }
}

void demo_pwm_stop(void)
{
    for (uint32_t i = 0; i < MAX_PWM_CHANNEL; i++)
    {
        hosal_pwm_stop(rgb_pwm + i);
        hosal_pwm_finalize(rgb_pwm + i);
    }
}

void set_level(uint8_t currLevel)
{
    hosal_pwm_config_t para;

    if (currLevel <= 5 && currLevel >= 1)
    {
        currLevel = 5; // avoid demo off
    }

    para.duty_cycle = currLevel * PWM_DUTY_CYCLE / 254;
    para.freq       = PWM_FREQ;

    demo_pwm_change_param(&para);
}

void set_color_red(uint8_t currLevel)
{
    set_color(currLevel, 0, 254);
}

void set_color_green(uint8_t currLevel)
{
    set_color(currLevel, 84, 254);
}

void set_color_yellow(uint8_t currLevel)
{
    set_color(currLevel, 42, 254);
}

void set_color(uint8_t currLevel, uint8_t currHue, uint8_t currSat)
{
#if MAX_PWM_CHANNEL == 3
    uint16_t hue = (uint16_t) currHue * 360 / 254;
    uint8_t sat  = (uint16_t) currSat * 100 / 254;

    if (currLevel <= 5 && currLevel >= 1)
    {
        currLevel = 5; // avoid demo off
    }

    if (sat > 100)
    {
        sat = 100;
    }

    uint16_t i       = hue / 60;
    uint16_t rgb_max = currLevel;
    uint16_t rgb_min = rgb_max * (100 - sat) / 100;
    uint16_t diff    = hue % 60;
    uint16_t rgb_adj = (rgb_max - rgb_min) * diff / 60;
    uint32_t red, green, blue;

    switch (i)
    {
    case 0:
        red   = rgb_max;
        green = rgb_min + rgb_adj;
        blue  = rgb_min;
        break;
    case 1:
        red   = rgb_max - rgb_adj;
        green = rgb_max;
        blue  = rgb_min;
        break;
    case 2:
        red   = rgb_min;
        green = rgb_max;
        blue  = rgb_min + rgb_adj;
        break;
    case 3:
        red   = rgb_min;
        green = rgb_max - rgb_adj;
        blue  = rgb_max;
        break;
    case 4:
        red   = rgb_min + rgb_adj;
        green = rgb_min;
        blue  = rgb_max;
        break;
    default:
        red   = rgb_max;
        green = rgb_min;
        blue  = rgb_max - rgb_adj;
        break;
    }

    // change level to pwm duty_cycle
    // 0-254 to 0-10000
    hosal_pwm_config_t para[3];
    para[0].duty_cycle = blue * PWM_DUTY_CYCLE / 254;
    para[0].freq       = PWM_FREQ;
    para[1].duty_cycle = red * PWM_DUTY_CYCLE / 254;
    para[1].freq       = PWM_FREQ;
    para[2].duty_cycle = green * PWM_DUTY_CYCLE / 254;
    para[2].freq       = PWM_FREQ;

    demo_pwm_change_param(para);
#else
    set_level(currLevel);
#endif
}

void set_level_temperature(uint8_t level, uint16_t temperature)
{
    CtColor_t ct;
    temperature = clamp(temperature, COLOR_TEMPERATURE_MIN, COLOR_TEMPERATURE_MAX);
    ct.ctMireds = temperature;
    RgbColor_t rgb;
    rgb          = CTToRgb(ct);
    uint16_t tmp = 0;
    tmp          = rgb.r * level / 254;
    rgb.r        = (uint8_t) (tmp & 0xFF);
    tmp          = rgb.g * level / 254;
    rgb.g        = (uint8_t) (tmp & 0xFF);
    tmp          = rgb.b * level / 254;
    rgb.b        = (uint8_t) (tmp & 0xFF);
    set_rgb(rgb);
}
void set_temperature(uint16_t temperature)
{
    CtColor_t ct;
    temperature = clamp(temperature, COLOR_TEMPERATURE_MIN, COLOR_TEMPERATURE_MAX);
    ct.ctMireds = temperature;
    RgbColor_t rgb;
    rgb = CTToRgb(ct);
    set_rgb(rgb);
}
void set_rgb(RgbColor_t rgb)
{
    set_r_g_b(rgb.r, rgb.g, rgb.b);
}
void set_r_g_b(uint8_t red, uint8_t green, uint8_t blue)
{
    // ChipLogProgress(NotSpecified, "3R: set rgb r=%d g=%d b=%d", red, green, blue);
    // change level to pwm duty_cycle
    // 0-254 to 0-10000
    hosal_pwm_config_t para[3];
    para[0].duty_cycle = blue * 10000 / 254;  // blue * PWM_DUTY_CYCLE / 254;
    para[0].freq       = 1000;                // PWM_FREQ;
    para[1].duty_cycle = red * 10000 / 254;   // red * PWM_DUTY_CYCLE / 254;
    para[1].freq       = 1000;                // PWM_FREQ;
    para[2].duty_cycle = green * 10000 / 254; // green * PWM_DUTY_CYCLE / 254;
    para[2].freq       = 1000;                // PWM_FREQ;
    demo_pwm_change_param(para);
}
RgbColor_t CTToRgb(CtColor_t ct)
{
    RgbColor_t rgb;
    float r, g, b;
    // Algorithm credits to Tanner Helland: https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html
    // Convert Mireds to centiKelvins. k = 1,000,000/mired
    float ctCentiKelvin = 1000000 / (float) (ct.ctMireds);
    // Apple range (left.1000---right.25) map to (1000---40000)
    // 1000000/1000 = 1000      // 1000000/150 = 6666       // 1000000/25 = 40000
    if (ctCentiKelvin < 6666)
    {
        // 1000000/50000 = 20       // 1000000/30000 = 33.33    // 1000000/5000 = 200
        ctCentiKelvin = ctCentiKelvin / 100 * 0.232143 + 17.678571; // 50000(20)
        // 1000000/40000 = 25       // 1000000/30000 = 33.33    // 1000000/5000 = 200
        // ctCentiKelvin = ctCentiKelvin / 100 * 0.142857 + 23.571428; // 40000(25)
    }
    else
    {
        ctCentiKelvin = ctCentiKelvin / 200;
    }
    // Red
    if (ctCentiKelvin <= 66)
    {
        r = 255;
    }
    else
    {
        r = 329.698727446f * pow(ctCentiKelvin - 60, -0.1332047592f);
    }
    // Green
    if (ctCentiKelvin <= 66)
    {
        g = 99.4708025861f * log(ctCentiKelvin) - 161.1195681661f;
    }
    else
    {
        g = 288.1221695283f * pow(ctCentiKelvin - 60, -0.0755148492f);
    }
    // Blue
    if (ctCentiKelvin >= 66)
    {
        b = 255;
    }
    else
    {
        if (ctCentiKelvin <= 19)
        {
            b = 0;
        }
        else
        {
            b = 138.5177312231f * log(ctCentiKelvin - 10) - 305.0447927307f;
        }
    }
    // ChipLogProgress(NotSpecified, "3R: set rgb r=%f g=%f b=%f", r, g, b);
    rgb.r = (uint8_t) clamp(r, 0, 255);
    rgb.g = (uint8_t) clamp(g, 0, 255);
    rgb.b = (uint8_t) clamp(b, 0, 255);
    return rgb;
}
