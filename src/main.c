#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "components/led/led.h"

#define LED_GPIO 2

void blink_task(void *arg)
{
    while (1)
    {
        led_toggle();
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void)
{
    led_init(LED_GPIO);

    xTaskCreate(
        blink_task,
        "blink_task",
        1024,
        NULL,
        5,
        NULL);
}