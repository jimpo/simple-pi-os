#include "libpi/rpi.h"

int _start() {
	unsigned led = GPIO_PIN20;

    gpio_set_output(led);
    for (int i = 0; i < 10; i++) {
        gpio_set_on(led);
        delay_cycles(5000000);
        gpio_set_off(led);
        delay_cycles(5000000);
    }
    return 0;
}
