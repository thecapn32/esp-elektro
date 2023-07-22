#include <stdio.h>
#include <stdint.h>
#include <string.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"


#include <mcp4725.h>


#define TAG "ELEKTRO"


#define VDD 3.3


#define CTL1 GPIO_NUM_0
#define CTL2 GPIO_NUM_1

#define LED_PIN GPIO_NUM_5

// const uint32_t l_duration[18] = {30, 5, 30, 5, 240, 2, 180, 3, 180, 5, 120, 5, 360, 360, 90, 5, 90, 90};
// const uint32_t dacCur_r[18] = {20, 0, 20, 0, 20, 0, 100, 0, 20, 0, 80, 0, 50, 80, 50, 0, 50, 100};
const uint32_t freq[18] = {6000, 0, 5000, 0, 3000, 0, 200, 0, 100, 0, 1, 0, 100, 8, 3, 0, 1000, 50};



static i2c_dev_t dev;


// void rtos_timer_callback(TimerHandle_t xTimer)
// {
//     int flag = (int)pvTimerGetTimerID(xTimer);
//     ESP_LOGI(TAG,"Timer called with timer id %d", flag);
// }


// static int create_RTOS_timer(unsigned int ms)
// {
//     int flag = 1;
//     tmr = xTimerCreate("phasetimer", pdMS_TO_TICKS(ms), pdFALSE, (void *)flag, rtos_timer_callback);
//     xTimerStart(tmr, 0);
// }


static float I_to_V_conversion(uint32_t current)
{
    float r = 68;
    float u = 10000;
    return (r/u) * current;
}


static uint64_t hz_to_us(uint32_t fhz)
{
    if (fhz == 0) return 0;
    return 10000000/fhz;
}


static bool IRAM_ATTR timer_group_isr_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    static int i = 1;
    
    float *dacval = (float *) user_data;

    if (i)
    {
        gpio_set_level(LED_PIN, 0);
        i = 0;
        //mcp4725_set_voltage(&dev, 3.3, *dacval, false);
    } else 
    {
        gpio_set_level(LED_PIN, 1);
        i = 1;
        //mcp4725_set_voltage(&dev, 3.3, 0, false);
    }
    ESP_ERROR_CHECK(gptimer_set_raw_count(timer, 0));
    return false;
}

static void gptimer_init(uint64_t count)
{
    gptimer_handle_t gptimer = NULL;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_group_isr_callback,
    };

    float dac = 0.68;

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, &dac));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    gptimer_alarm_config_t alarm_config1 = {
        .reload_count = 0,
        .alarm_count = count, // period = 1s
        .flags.auto_reload_on_alarm = true,
    };

    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config1));
    
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}


/*
 * I = V_dac / R9. <R9=6.8K Ohm>
 * Thus, if you output 0.68V, the current is 100uA.
*/
static void i2c_DAC_setup()
{
    ESP_ERROR_CHECK(i2cdev_init());
}


void app_main()
{
    //i2c_DAC_setup();
    
    gpio_set_direction(CTL2, GPIO_MODE_OUTPUT);
    gpio_set_direction(CTL1, GPIO_MODE_OUTPUT);
    //gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(CTL1, 0);
    gpio_set_level(CTL2, 1);
    //gpio_set_level(LED_PIN, 1);
    //gptimer_init();

    for (int i = 0; i < 18; i++)
    {
        ESP_LOGI(TAG, "fq: %luHz, P:%lluus", freq[i], hz_to_us(freq[i])/2);
    }
    
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    

    //xTaskCreate(task, "test", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
}