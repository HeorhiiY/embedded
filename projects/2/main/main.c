#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lamps.h"
#include "traffic.h"

void app_main(void)
{
    lamps_init();
    traffic_init();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        traffic_next();
    }
}
