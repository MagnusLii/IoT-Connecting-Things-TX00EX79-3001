#include "pico/time.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "display/ssd1306.h"
#include "Untitled.h"
#include <stdlib.h>
#include <stdio.h>

#define LED1 20
#define LED2 21
#define LED3 22

bool led1_toggled = false;
bool led2_toggled = false;
bool led3_toggled = false;

int led1_state = 0;
int led2_state = 0;
int led3_state = 0;

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    printf("Alarm fired\n");
    int value = *(int*)user_data;
    if (value == 1) {
        led1_state = !led1_state;
        gpio_put(LED1, led1_state);
        led1_toggled = true;
    } else if (value == 2) {
        led2_state = !led2_state;
        gpio_put(LED2, led2_state);
        led2_toggled = true;
    } else if (value == 3) {
        led3_state = !led3_state;
        gpio_put(LED3, led3_state);
        led3_toggled = true;
    }
    return 0;
}


int main() {
    stdio_init_all();
    gpio_init(LED1);
    gpio_set_dir(LED1, GPIO_OUT);

    gpio_init(LED2);
    gpio_set_dir(LED2, GPIO_OUT);

    gpio_init(LED3);
    gpio_set_dir(LED3, GPIO_OUT);

    i2c_init(i2c1, 400000);

    alarm_pool_t *pool = alarm_pool_get_default();
    gpio_set_function(14, GPIO_FUNC_I2C); // SDA
    gpio_set_function(15, GPIO_FUNC_I2C); // SCL

    uint64_t led1_delay = 125*1000;
    uint64_t led2_delay = 250*1000;
    uint64_t led3_delay = 500*1000;

    int *led1 = new int;
    *led1 = 1;
    int *led2 = new int;
    *led2 = 2;
    int *led3 = new int;
    *led3 = 3;

    alarm_id_t alarm_id1 = alarm_pool_add_alarm_in_us(pool, led1_delay, alarm_callback, led1, false);
    alarm_id_t alarm_id2 = alarm_pool_add_alarm_in_us(pool, led2_delay, alarm_callback, led2, false);
    alarm_id_t alarm_id3 = alarm_pool_add_alarm_in_us(pool, led3_delay, alarm_callback, led3, false);
    
    ssd1306 display(i2c1);
    mono_vlsb picbuf(binary_data, 39, 39);

    char since_boot_str[11];

    display.show();
    

    while (1) {
        if (led1_toggled){
            alarm_id_t alarm_id1 = alarm_pool_add_alarm_in_us(pool, led1_delay, alarm_callback, led1, false);
            led1_toggled = false;
        }
        if (led2_toggled){
            alarm_id_t alarm_id2 = alarm_pool_add_alarm_in_us(pool, led2_delay, alarm_callback, led2, false);
            led2_toggled = false;
        }
        if (led3_toggled){
            alarm_id_t alarm_id3 = alarm_pool_add_alarm_in_us(pool, led3_delay, alarm_callback, led3, false);
            led3_toggled = false;
        }
    uint32_t since_boot = (to_ms_since_boot(get_absolute_time())) / 1000;

    sprintf(since_boot_str, "%lu", since_boot);
    display.fill(0);
    display.text(since_boot_str, 0, 0);
    display.blit(picbuf, 0, 10);

    display.show();

    }

    free(led1);
    free(led2);
    free(led3);

    return 0;
}