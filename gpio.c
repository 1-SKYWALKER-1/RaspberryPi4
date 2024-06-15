#include <stdio.h>
#include <gpiod.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#define LED_PIN_1 17  // GPIO pin for the first LED
#define LED_PIN_2 27  // GPIO pin for the second LED
#define LED_PIN_3 23  // GPIO pin for the third LED
#define BUTTON_PIN 22 // GPIO pin for the button

#define BLINK_DELAY 3

#define CONSUMER "Consumer"

struct Array {
    int size;
    struct gpiod_line **elements;
};


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
            gpiod_line_release(line);
            return NULL;
        }
    } else {
        if (gpiod_line_request_output(line, CONSUMER, value) < 0) {
            perror("Error setting line as output");
            gpiod_line_release(line);
            return NULL;
        }
    }

    return line;
}

void changeDiodesState(struct Array lines) {
    for (int i = 0; i < lines.size; i++) {
        struct gpiod_line *line = lines.elements[i];
        gpiod_line_set_value(line, !gpiod_line_get_value(line));
    }
}

// Function to blink an LED connected to a GPIO line
void blinkLed(struct Array lines, struct timespec *lastToggleTime) {
    struct timespec currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    int periodNs = 1000000000 / BLINK_DELAY;
    long long elapsedNs = (currentTime.tv_sec - lastToggleTime->tv_sec) * 1000000000 +
                          (currentTime.tv_nsec - lastToggleTime->tv_nsec);

    if (elapsedNs >= periodNs) {
        changeDiodesState(lines);
        clock_gettime(CLOCK_MONOTONIC, lastToggleTime);
    }
}

_Bool previous_button_state = false;
uint8_t button_pressed = 0;

int main() {
    struct gpiod_chip *chip;
    struct gpiod_line *led_line_1, *led_line_2, *led_line_3, *button_line;
    struct timespec lastToggleTime;
    clock_gettime(CLOCK_MONOTONIC, &lastToggleTime);
	_Bool turnOff = false;
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


    struct gpiod_line *oneDiode[] = {led_line_1};
    struct gpiod_line *twoDiodes[] = {led_line_1, led_line_2};
    struct gpiod_line *threeDiodes[] = {led_line_1, led_line_2, led_line_3};

    struct Array one = {1, oneDiode};
    struct Array two = {2, twoDiodes};
    struct Array three = {3, threeDiodes};

    while (1) {
        _Bool buttonState = gpiod_line_get_value(button_line);

        if (previous_button_state == true && buttonState == false) {
            if (button_pressed < 4)
                button_pressed++;
            else
                button_pressed = 0;
        }

        switch (button_pressed) {
            case 0:
                gpiod_line_set_value(led_line_1, 0);
                gpiod_line_set_value(led_line_2, 0);
                gpiod_line_set_value(led_line_3, 0);
                //printf("zero case switch");
				turnOff = false;
                break;
            case 1:
                blinkLed(one, &lastToggleTime);
                //printf("first case switch");
				turnOff = false;
                break;
				
            case 2:
				if(!turnOff){
					gpiod_line_set_value(led_line_1, 0);
					turnOff = true;
				}
                blinkLed(two, &lastToggleTime);
                //printf("second case switch");
                break;
            case 3:
				if(turnOff){
					gpiod_line_set_value(led_line_1, 0);
					gpiod_line_set_value(led_line_2, 0);
					turnOff = false;
				}
                blinkLed(three, &lastToggleTime);
                //printf("third case switch");
                break;
            default:
                gpiod_line_set_value(led_line_1, 1);
                gpiod_line_set_value(led_line_2, 1);
                gpiod_line_set_value(led_line_3, 1);
                //printf("default case switch");
                break;
        }

        previous_button_state = buttonState;
        struct timespec delay = {0, 10000000L}; // 10 milliseconds
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
