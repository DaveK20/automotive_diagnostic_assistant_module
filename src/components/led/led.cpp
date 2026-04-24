#include "led.h"
#include "driver/gpio.h"

static gpio_num_t led_gpio;

void led_init(gpio_num_t gpio)
{
    led_gpio = gpio;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << led_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};

    gpio_config(&io_conf);
}

void led_on(void)
{
    gpio_set_level(led_gpio, 1);
}

void led_off(void)
{
    gpio_set_level(led_gpio, 0);
}

void led_toggle(void)
{
    static uint8_t state = 0;
    state = !state;
    gpio_set_level(led_gpio, state);
}