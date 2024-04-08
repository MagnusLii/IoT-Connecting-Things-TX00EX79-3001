#include "pico/stdlib.h"
//#include "hardware/pwm.h"
//#include <stdio.h>

int main() {
    gpio_init(21);
    gpio_set_dir(21, GPIO_OUT);

    while (1) {
        gpio_put(21, 1);
        sleep_ms(500);

        gpio_put(21, 0);
        sleep_ms(500);
    }
    return 0;
}