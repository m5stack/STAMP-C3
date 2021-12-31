#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "soc/rtc_cntl_reg.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include <time.h>
#include "esp_system.h"

#define STAMP_C3
// #define STAMP_C3U

#define RMT_TX_CHANNEL RMT_CHANNEL_0
led_strip_t *strip;

#ifdef STAMP_C3
uint8_t PINS[] = {4, 5, 6, 7, 8, 10, 1, 0, 21, 20, 9, 18, 19};
#define BTN_PIN 3
#endif

#ifdef STAMP_C3U
uint8_t PINS[] = {4, 5, 6, 7, 8, 10, 1, 0, 21, 20, 3, 18, 19};
#define BTN_PIN 9
#endif

#define RMT_TX_GPIO 2

void update_led(uint8_t r, uint8_t g, uint8_t b)
{
    ESP_ERROR_CHECK(strip->set_pixel(strip, 0, r, g, b));
    ESP_ERROR_CHECK(strip->refresh(strip, 0));
}

uint8_t state = 1;

void io_reverse()
{
    state = !state;
    ESP_LOGI("IO REVERSE", "IO STATE: %d", state);
    for (uint8_t index = 0; index < sizeof(PINS); index++)
    {
        gpio_set_level(PINS[index], state);
    }
}

uint8_t base_mac_addr[6] = {0};

void app_main(void)
{

    //Init RMT
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(RMT_TX_GPIO, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(1, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip)
    {
        ESP_LOGE("Main", "Install WS2812 driver failed");
    }

    //Get base MAC address from EFUSE BLK0(default option)
    esp_efuse_mac_get_default(base_mac_addr);

    ESP_LOGI("Main", "MAC: %X%X%X%X%X%X",
             base_mac_addr[0], base_mac_addr[1], base_mac_addr[2], base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]);

    //INIT IO MODE
    for (uint8_t index = 0; index < sizeof(PINS); index++)
    {
        gpio_reset_pin(PINS[index]);
        gpio_set_direction(PINS[index], GPIO_MODE_OUTPUT);
        gpio_set_pull_mode(PINS[index], GPIO_PULLUP_ONLY);
    }

    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);

    update_led(0, 255, 200);

    while (true)
    {
        if (gpio_get_level(BTN_PIN) == 0)
        {
            vTaskDelay(5 / portTICK_RATE_MS);
            if (gpio_get_level(BTN_PIN) == 0)
            {
                int r = rand() % 256;
                int g = rand() % 256;
                int b = rand() % 256;
                update_led(r, g, b);
                io_reverse();
                while (gpio_get_level(BTN_PIN) == 0)
                {
                    srand((unsigned)b);
                }
                ESP_LOGI("Main", "MAC: %X%X%X%X%X%X",
                         base_mac_addr[0], base_mac_addr[1], base_mac_addr[2], base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]);
            }
        }
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}
