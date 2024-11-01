#ifndef __DEMO_PWM__
#define __DEMO_PWM__

#ifdef __cplusplus
extern "C" {
#endif

#define INIT_COLOR_T 160                   // 254  // Color Temperature
#define COLOR_TEMPERATURE_MIN 1            // Color Kelvins.(15.32---1,000,000)  Kelvins = 1,000,000 / Temperature
#define COLOR_TEMPERATURE_MAX 65279        // Color Temperature.(1-------65279)
#define COLOR_TEMPERATURE_APPLE_MIN 20     // Color Temperature.20 = Kelvines.50000
#define COLOR_TEMPERATURE_APPLE_MIDDLE 150 // Color Temperature.150 = Kelvines.6666
#define COLOR_TEMPERATURE_APPLE_MAX 1000   // Color Temperature.1000 = Kelvins.1000
typedef struct RgbColor_s
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RgbColor_t;
typedef struct CtColor_s
{
    uint16_t ctMireds;
} CtColor_t;

void set_rgb(RgbColor_t rgb);
void set_r_g_b(uint8_t red, uint8_t green, uint8_t blue);
void set_temperature(uint16_t temperature);
void set_level_temperature(uint8_t level, uint16_t temperature);
RgbColor_t CTToRgb(CtColor_t ct);

void demo_pwm_init(void);
void demo_pwm_start(void);
void set_color_red(uint8_t currLevel);
void set_color_green(uint8_t currLevel);
void set_color_yellow(uint8_t currLevel);
void set_color(uint8_t currLevel, uint8_t currHue, uint8_t currSat);

void set_level(uint8_t currLevel);

#ifdef __cplusplus
}
#endif

#endif // __DEMO_PWM__
