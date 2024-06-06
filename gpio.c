#include <stdio.h>
#include <gpiod.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define LED_PIN_1 17  // GPIO pin for the first LED
#define LED_PIN_2 27  // GPIO pin for the second LED
#define LED_PIN_3 23  // GPIO pin for the second LED
#define BUTTON_PIN 22 // GPIO pin for the button

#define BLINK_DELAY 3

#define CONSUMER "Consumer"

// Function to unexport GPIO pins


// Function to set up a GPIO line
struct gpiod_line *setup_gpio_line(struct gpiod_chip *chip, unsigned int line_num, int direction, int value) {
    struct gpiod_line *line;

    line = gpiod_chip_get_line(chip, line_num);
    if (!line) {
        perror("Error getting GPIO line");
        return NULL;
    }

    if (direction == GPIOD_LINE_REQUEST_DIRECTION_INPUT) {
        if (gpiod_line_request_input(line, CONSUMER) < 0) {
            perror("Error setting line as input");
            return NULL;
        }
    } else {
        if (gpiod_line_request_output(line, CONSUMER, value) < 0) {
            perror("Error setting line as output");
            return NULL;
        }
    }

    return line;
}

void blinkLed(struct gpiod_line *line, struct timespec *lastToggleTime) {
    struct timespec currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    int periodNs = nanosInSec / BLINK_DELAY;
    long long elapsedNs = (currentTime.tv_sec - lastToggleTime->tv_sec) * nanosInSec +
                          (currentTime.tv_nsec - lastToggleTime->tv_nsec);

    if (elapsedNs >= periodNs) {

        gpiod_line_set_value(line, !gpiod_line_get_value(line));
        clock_gettime(CLOCK_MONOTONIC, lastToggleTime);
    }
}

_Bool previous_button_state = false;
uint8_t button_presed = 0;

int main() {
    struct gpiod_chip *chip;
    struct gpiod_line *led_line_1, *led_line_2, *led_line_3, *button_line;

    // Open the GPIO chip (chip 0 is usually correct for Raspberry Pi)
    chip = gpiod_chip_open_by_number(0);
    if (!chip) {
        perror("Error opening GPIO chip");
        return 1;
    }

    // Setup GPIO lines
    led_line_1 = setup_gpio_line(chip, LED_PIN_1, GPIOD_LINE_REQUEST_DIRECTION_OUTPUT, 0);
    if (!led_line_1) return 1;

    led_line_2 = setup_gpio_line(chip, LED_PIN_2, GPIOD_LINE_REQUEST_DIRECTION_OUTPUT, 0);
    if (!led_line_2) return 1;

    led_line_3 = setup_gpio_line(chip, LED_PIN_3, GPIOD_LINE_REQUEST_DIRECTION_OUTPUT, 0);
    if (!led_line_3) return 1;

    button_line = setup_gpio_line(chip, BUTTON_PIN, GPIOD_LINE_REQUEST_DIRECTION_INPUT, 0);
    if (!button_line) return 1;

    while (1) {

        _Bool buttonState = gpiod_line_get_value(button_line);

        if (previous_button_state == 1 && buttonState == 0) {
            (button_presed < 5) ? (button_presed++) : (button_presed = 0);
        }

        switch (button_presed) {
            case 0: {
                gpiod_line_set_value(led_line_1, 0);
                gpiod_line_set_value(led_line_2, 0);
                gpiod_line_set_value(led_line_3, 0);
                break;
            }
            case 1: {
                blinkLed(led_line_1, &lastToggleTime);
                break;
            }
            case 2: {
                blinkLed(led_line_1, &lastToggleTime);
                blinkLed(led_line_2, &lastToggleTime);
                break;
            }
            case 3: {
                blinkLed(led_line_1, &lastToggleTime);
                blinkLed(led_line_2, &lastToggleTime);
                blinkLed(led_line_3, &lastToggleTime);
                break;
            }
            default: {
                gpiod_line_set_value(led_line_1, 1);
                gpiod_line_set_value(led_line_2, 1);
                gpiod_line_set_value(led_line_3, 1);
                break;
            }
        }
        previous_button_state = buttonState;
        struct timespec delay = {0, 10000000L};
        nanosleep(&delay, NULL);
    }

    // Release GPIO lines
    gpiod_line_release(led_line_1);
    gpiod_line_release(led_line_2);
    gpiod_line_release(led_line_3);
    gpiod_line_release(button_line);

    // Close the GPIO chip
    gpiod_chip_close(chip);

    return 0;
}

