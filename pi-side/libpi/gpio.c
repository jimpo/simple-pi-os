#include "rpi.h"
#include "gpio.h"

// GPIO Register Map offset (BCM2835 Data Sheet p90).
// Instead of using 0x7E215000 as written, which is the bus address, the value
// is translated to the physical address beginning with 0x20
// (BCM2835 Data Sheet p6).
#define GPIO_BASE 0x20200000
unsigned gpio_fsel[] = {
    GPIO_BASE + 0x00,
    GPIO_BASE + 0x04,
    GPIO_BASE + 0x08,
    GPIO_BASE + 0x0C,
    GPIO_BASE + 0x10,
    GPIO_BASE + 0x14,
};
unsigned gpio_set[] = {
    GPIO_BASE + 0x1C,
    GPIO_BASE + 0x20
};
unsigned gpio_clr[] = {
    GPIO_BASE + 0x28,
    GPIO_BASE + 0x2C,
};
unsigned gpio_lvl[] = {
    GPIO_BASE + 0x34,
    GPIO_BASE + 0x38,
};
unsigned gpio_pud = GPIO_BASE + 0x94;
unsigned gpio_pudclk[] = {
    GPIO_BASE + 0x98,
    GPIO_BASE + 0x9C,
};

#define GPIO_FUNC_BITS 3
#define GPIO_FUNC_MASK ((1 << GPIO_FUNC_BITS) - 1)

void gpio_init() {
}

int gpio_set_function(unsigned pin, unsigned function) {
    // If `pin` or `function` is invalid, does nothing.
    if (pin > GPIO_PIN_LAST || function > GPIO_FUNC_MASK) {
        return 0;
    }

    // BCM2835 Data Sheet p91-94.
    unsigned gpio = gpio_fsel[pin / 10];
    unsigned value = GET32(gpio);
    value &= ~(GPIO_FUNC_MASK << (GPIO_FUNC_BITS * (pin % 10)));
    value |= (function << (GPIO_FUNC_BITS * (pin % 10)));
    PUT32(gpio, value);
    return 1;
}

unsigned gpio_get_function(unsigned pin) {
    // If `pin` is invalid, returns GPIO_INVALID_REQUEST.
    if (pin > GPIO_PIN_LAST) {
        return GPIO_INVALID_REQUEST;
    }

    // BCM2835 Data Sheet p91-94.
    unsigned gpio = gpio_fsel[pin / 10];
    unsigned value = GET32(gpio);
    return (value >> GPIO_FUNC_BITS * (pin % 10)) & GPIO_FUNC_MASK;
}

int gpio_set_input(unsigned pin) {
    gpio_set_function(pin, GPIO_FUNC_INPUT);
    return 1;
}

int gpio_set_output(unsigned pin) {
    gpio_set_function(pin, GPIO_FUNC_OUTPUT);
    return 1;
}

int gpio_write(unsigned pin, unsigned val) {
    // If `pin` is invalid, does nothing.
    if (pin > GPIO_PIN_LAST) {
        return GPIO_INVALID_REQUEST;
    }

    // BCM2835 Data Sheet p95.
    if (val) {
        PUT32(gpio_set[pin / 32], 1 << (pin % 32));
    } else {
        PUT32(gpio_clr[pin / 32], 1 << (pin % 32));
    }
    return 1;
}

unsigned gpio_read(unsigned pin) {
    // If `pin` is invalid, returns GPIO_INVALID_REQUEST.
    if (pin > GPIO_PIN_LAST) {
        return GPIO_INVALID_REQUEST;
    }

    // BCM2835 Data Sheet p96.
    unsigned value = GET32(gpio_lvl[pin / 32]);
    return (value >> pin % 32) & 1;
}

int gpio_set_pud(unsigned pin, unsigned pud) {
    // If `pin` is invalid, returns GPIO_INVALID_REQUEST.
    if (pin > GPIO_PIN_LAST) {
        return GPIO_INVALID_REQUEST;
    }

    // The GPIO Pull-up/down Clock Registers control the actuation of internal pull-downs on
    // the respective GPIO pins. These registers must be used in conjunction with the GPPUD
    // register to effect GPIO Pull-up/down changes. The following sequence of events is
    // required:

    // 1. Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
    //   to remove the current Pull-up/down)
    PUT32(gpio_pud, pud);

    // 2. Wait 150 cycles -- this provides the required set-up time for the control signal
    delay(150);

    // 3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
    // modify -- NOTE only the pads which receive a clock will be modified, all others will
    // retain their previous state.
    PUT32(gpio_pudclk[pin / 32], 1 << (pin % 32));

    // 4. Wait 150 cycles -- this provides the required hold time for the control signal
    delay(150);

    // 5. Write to GPPUD to remove the control signal
    PUT32(gpio_pud, GPIO_PUD_DISABLE);

    // 6. Write to GPPUDCLK0/1 to remove the clock
    PUT32(gpio_pudclk[pin / 32], 0);

    return 1;
}

int gpio_set_pullup(unsigned pin) {
    return gpio_set_pud(pin, GPIO_PUD_PULLUP);
}

int gpio_set_pulldown(unsigned pin) {
    return gpio_set_pud(pin, GPIO_PUD_PULLDOWN);
}

int gpio_set_on(unsigned pin) {
    return gpio_write(pin, 1);
}

int gpio_set_off(unsigned pin) {
    return gpio_write(pin, 0);
}
