#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lamps.h"
#include "traffic.h"
#include "button.h"

void app_main(void)
{
    lamps_init();
    traffic_init();
    button_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));

        switch (button_poll()) {
        case BUTTON_EVENT_SHORT: traffic_next(); break;
        case BUTTON_EVENT_LONG:  traffic_reset(); break;
        default: break;
        }
    }
}
