#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "atuador.h"
#include "wifi.h"
#include "mqtt_app.h"


void app_main() 
{
 wifi_init_sta();

    vTaskDelay(pdMS_TO_TICKS(5000));
    mqtt_start();
 gpioInit();
    while(1)
    {
        mqtt_publish_status();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}