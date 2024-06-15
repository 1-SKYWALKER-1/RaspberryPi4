#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define BUTTON_GPIO "534"
#define LED1_GPIO "529"
#define LED2_GPIO "539"
#define LED3_GPIO "535"
#define BLINK_DELAY 3
#define nanosInSec 1000000000

#define OUTPUT "out"
#define INPUT "in"
#define LOW "0"
#define HIGH "1"
struct Array {
    int size;
    char **elements;
};
// Функція для налаштування режиму піна
void pinMode(const char pin[], const char mode[]) {
    FILE *sysfs_export;
    FILE *sysfs_direction;
    char path[50];

    // Експортуємо пін
    sysfs_export = fopen("/sys/class/gpio/export", "w");
    if (sysfs_export == NULL) {
        perror("Error opening export file");
        return;
    }
    fwrite(pin, 1, strlen(pin), sysfs_export);
    fclose(sysfs_export);

    // Встановлюємо напрямок піна
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/direction", pin);
    sysfs_direction = fopen(path, "w");
    if (sysfs_direction == NULL) {
        perror("Error opening direction file");
        return;
    }
    fwrite(mode, 1, strlen(mode), sysfs_direction);
    fclose(sysfs_direction);
}

// Функція для установки значення на піні
void digitalWrite(const char pin[], const char value[]) {
    char path[50];
    FILE *sysfs_value;

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/value", pin);
    sysfs_value = fopen(path, "w");
    if (sysfs_value == NULL) {
        perror("Error opening value file");
        return;
    }
    fwrite(value, 1, strlen(value), sysfs_value);
    fclose(sysfs_value);
}

// Функція для зчитування стану кнопки
int digitalRead(const char pin[]) {
    char path[50];
    char value_str[3];
    FILE *sysfs_value;

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/value", pin);
    sysfs_value = fopen(path, "r");
    if (sysfs_value == NULL) {
        perror("Error opening value file");
        return -1;
    }
    fread(value_str, 1, 3, sysfs_value);
    fclose(sysfs_value);

    return atoi(value_str);
}

// Функція для очищення (unexport) піна
void cleanUp(const char pin[]) {
    FILE *sysfs_unexport;

    sysfs_unexport = fopen("/sys/class/gpio/unexport", "w");
    if (sysfs_unexport == NULL) {
        perror("Error opening unexport file");
        return;
    }
    fwrite(pin, 1, strlen(pin), sysfs_unexport);
    fclose(sysfs_unexport);
}
void changeDiodesState(struct Array pins) {
    for (int i = 0; i < pins.size; i++) {
        char *pin = pins.elements[i];
        char currentValue[2];
            snprintf(currentValue, sizeof(currentValue), "%d", digitalRead(pin) == 0 ? 1 : 0);
            digitalWrite(pin, currentValue);
    }
}
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
    // Очищаем пины перед завершением программы
    cleanUp(LED1_GPIO);
    cleanUp(LED2_GPIO);
    cleanUp(LED3_GPIO);
    cleanUp(BUTTON_GPIO);

    // Настраиваем пины
    pinMode(LED1_GPIO, OUTPUT);
    pinMode(LED2_GPIO, OUTPUT);
    pinMode(LED3_GPIO, OUTPUT);
    pinMode(BUTTON_GPIO, INPUT);

    struct timespec lastToggleTime;
    clock_gettime(CLOCK_MONOTONIC, &lastToggleTime);
	char *oneDiode[] = {LED1_GPIO};
    char *twoDiodes[] = {LED1_GPIO, LED2_GPIO};
    char *threeDiodes[] = {LED1_GPIO, LED2_GPIO, LED3_GPIO};

    struct Array one = {1, oneDiode};
    struct Array two = {2, twoDiodes};
    struct Array three = {3, threeDiodes};
	_Bool turnOff = false;
    while (1) {
        // Считываем состояние кнопки
        _Bool buttonState = digitalRead(BUTTON_GPIO);

        // Обрабатываем нажатие кнопки
        if (previous_button_state == 1 && buttonState == 0) {
            button_pressed = (button_pressed < 4) ? (button_pressed + 1) : 0;
        }

        // Выполняем действия в зависимости от состояния кнопки
        switch (button_pressed) {
            case 0: {
                digitalWrite(LED1_GPIO, LOW);
                digitalWrite(LED2_GPIO, LOW);
                digitalWrite(LED3_GPIO, LOW);
                break;
            }
            case 1: {
                blinkLed(one, &lastToggleTime);
				turnOff = false;
                break;
            }
            case 2: {
				if(!turnOff){
					digitalWrite(LED1_GPIO, LOW);
					turnOff = true;
				}
                blinkLed(two, &lastToggleTime);
                
                break;
            }
            case 3: {
				if(turnOff){
					digitalWrite(LED1_GPIO, LOW);
					digitalWrite(LED2_GPIO, LOW);
					turnOff = false;
				}
                blinkLed(three, &lastToggleTime);

                break;
            }
            default: {
                digitalWrite(LED1_GPIO, HIGH);
                digitalWrite(LED2_GPIO, HIGH);
                digitalWrite(LED3_GPIO, HIGH);
                break;
            }
        }

        // Обновляем состояние предыдущей кнопки
        previous_button_state = buttonState;

        // Задержка для стабилизации цикла
        usleep(10000); // 10 миллисекунд
    }

    return 0;
}
