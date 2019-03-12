#include "libpi/rpi.h"

int notmain() {
	unsigned led = GPIO_PIN20;

    gpio_set_output(led);
    while(1) {
        gpio_set_on(led);
        delay_cycles(5000000);
        gpio_set_off(led);
        delay_cycles(5000000);
    }
    return 0;
}
