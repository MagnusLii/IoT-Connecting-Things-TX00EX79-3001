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

bool alarm_callback(repeating_timer_t *rt) {
    printf("Alarm fired\n");
    int value = *(int*)rt->user_data;
    printf("Value: %d\n", value);
    if (value == LED1) {
        led1_state = !led1_state;
        gpio_put(LED1, led1_state);
    } else if (value == LED2) {
        led2_state = !led2_state;
        gpio_put(LED2, led2_state);
    } else if (value == LED3) {
        led3_state = !led3_state;
        gpio_put(LED3, led3_state);
    }
    return 1;
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

    //alarm_pool_t *pool = alarm_pool_get_default();
    gpio_set_function(14, GPIO_FUNC_I2C); // SDA
    gpio_set_function(15, GPIO_FUNC_I2C); // SCL

    uint64_t led1_delay = -125*1000;
    uint64_t led2_delay = -250*1000;
    uint64_t led3_delay = -500*1000;

    /*
    alarm_id_t alarm_id1 = alarm_pool_add_alarm_in_us(pool, led1_delay, alarm_callback, led1, false);
    alarm_id_t alarm_id2 = alarm_pool_add_alarm_in_us(pool, led2_delay, alarm_callback, led2, false);
    alarm_id_t alarm_id3 = alarm_pool_add_alarm_in_us(pool, led3_delay, alarm_callback, led3, false);
    */

    //struct repeating_timer timer;

    repeating_timer led_timer1;
    repeating_timer led_timer2;
    repeating_timer led_timer3;

    int led1 = LED1;
    int led2 = LED2;
    int led3 = LED3;

    add_repeating_timer_us(led1_delay, alarm_callback, &led1, &led_timer1);
    add_repeating_timer_us(led2_delay, alarm_callback, &led2, &led_timer2);
    add_repeating_timer_us(led3_delay, alarm_callback, &led3, &led_timer3);

    ssd1306 display(i2c1);
    mono_vlsb picbuf(binary_data, 39, 39);

    char since_boot_str[11];

    display.show();
    

    while (1) {
    /*
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
    */

    uint32_t since_boot = (to_ms_since_boot(get_absolute_time())) / 1000;

    sprintf(since_boot_str, "%lu", since_boot);
    display.fill(0);
    display.text(since_boot_str, 0, 0);
    display.blit(picbuf, 0, 10);

    display.show();

    }

    return 0;
}