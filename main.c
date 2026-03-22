#include "eeng1030_lib.h"
#include "display.h"
#include "i2c.h"
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <sys/unistd.h>
#include <stm32l432xx.h>

#define ACCEL_ADDR 0x69

#define TILT_LEFT     0
#define TILT_RIGHT    1
#define TILT_FORWARD  2
#define TILT_BACK     3
#define TILT_NONE     255

#define PASSWORD_LENGTH 4
#define PASSWORD_TIMEOUT_MS 5000

#define X_THRESHOLD 900
#define Y_FORWARD_THRESHOLD 800
#define Y_BACK_THRESHOLD   -900

#define X_NEUTRAL_HIGH  300
#define X_NEUTRAL_LOW  -300
#define Y_NEUTRAL_HIGH  200
#define Y_NEUTRAL_LOW  -200

#define LED1_PORT GPIOB
#define LED1_PIN  5
#define LED2_PORT GPIOB
#define LED2_PIN  4
#define LED3_PORT GPIOB
#define LED3_PIN  0
#define LED4_PORT GPIOA
#define LED4_PIN 10

#define BUTTON_PORT GPIOA
#define BUTTON_PIN 8

#define STATUS_LED_PORT GPIOA
#define STATUS_LED_PIN 9

extern volatile uint32_t milliseconds;

// ===== Helper functions =====
static const char* tiltToString(uint8_t tilt)
{
    if (tilt == TILT_LEFT) return "LEFT";
    if (tilt == TILT_RIGHT) return "RIGHT";
    if (tilt == TILT_FORWARD) return "FORWARD";
    if (tilt == TILT_BACK) return "BACK";
    return "NONE";
}

static void drawLockedScreen(void)
{
    uint16_t black = RGBToWord(0,0,0);
    uint16_t white = RGBToWord(255,255,255);

    fillRectangle(0,0,160,80,black);
    printTextX2("LOCKED", 28, 20, white, black);
}

static void drawUnlockedScreen(uint32_t time_ms)
{
    uint16_t black = RGBToWord(0,0,0);
    uint16_t green = RGBToWord(0,255,0);
    uint16_t white = RGBToWord(255,255,255);

    char buffer[10];

    uint32_t seconds = time_ms / 1000;
    uint32_t ms = (time_ms % 1000) / 10;

    buffer[0] = '0' + (seconds % 10);
    buffer[1] = '.';
    buffer[2] = '0' + (ms / 10);
    buffer[3] = '0' + (ms % 10);
    buffer[4] = 's';
    buffer[5] = 0;

    fillRectangle(0,0,160,80,black);

    printTextX2("UNLOCKED", 10, 5, green, black);
    printText("TIME:", 20, 45, white, black);
    printText(buffer, 70, 45, white, black);
}

// ===== Forward declarations =====
static void setup(void);
static void accel_init(void);
static int16_t accel_read_axis(uint8_t reg);
static uint8_t detect_tilt(int32_t x_g100, int32_t y_g100);
static void leds_red(void);
static void led_green(GPIO_TypeDef *port, uint32_t pin);
static void show_progress(uint8_t progress);
static void initSerial(uint32_t baudrate);
static void eputc(char c);

// ===== Password =====
static const uint8_t password[PASSWORD_LENGTH] =
{
    TILT_LEFT,
    TILT_RIGHT,
    TILT_FORWARD,
    TILT_BACK
};

int main(void)
{
    int16_t x_raw, y_raw;
    int32_t x_g100, y_g100;

    uint8_t progress = 0;
    uint8_t ready = 1;
    uint8_t tilt;

    uint32_t last_correct_input_time = 0;
    uint32_t start_time = 0;
    uint32_t solve_time = 0;

    uint8_t system_enabled = 0;
    uint8_t last_button_state = 1;
    uint8_t button_state;

    setup();
    initSerial(9600);
    accel_init();

    pinMode(LED1_PORT, LED1_PIN, 1);
    pinMode(LED2_PORT, LED2_PIN, 1);
    pinMode(LED3_PORT, LED3_PIN, 1);
    pinMode(LED4_PORT, LED4_PIN, 1);

    leds_red();
    drawLockedScreen();

    printf("Tilt lock system ready\r\n");

    while (1)
    {
        button_state = (BUTTON_PORT->IDR & (1 << BUTTON_PIN)) ? 1 : 0;

        if ((last_button_state == 1) && (button_state == 0))
        {
            system_enabled ^= 1;

            progress = 0;
            ready = 1;
            last_correct_input_time = 0;
            leds_red();

            if (system_enabled)
            {
                STATUS_LED_PORT->ODR |= (1 << STATUS_LED_PIN);
                start_time = milliseconds;
                drawLockedScreen();
                printf("System ENABLED\r\n");
            }
            else
            {
                STATUS_LED_PORT->ODR &= ~(1 << STATUS_LED_PIN);
                drawLockedScreen();
                printf("System DISABLED\r\n");
            }

            delay_ms(200);
        }

        last_button_state = button_state;

        if (system_enabled)
        {
            x_raw = accel_read_axis(0x12);
            y_raw = accel_read_axis(0x14);

            x_g100 = ((int32_t)x_raw * 981) / 16384;
            y_g100 = ((int32_t)y_raw * 981) / 16384;

            // ===== SERIAL LIVE VALUES =====
            printf("X=%ld  Y=%ld\r\n", x_g100, y_g100);

            if ((x_g100 < X_NEUTRAL_HIGH && x_g100 > X_NEUTRAL_LOW) &&
                (y_g100 < Y_NEUTRAL_HIGH && y_g100 > Y_NEUTRAL_LOW))
            {
                ready = 1;
            }

            if ((progress > 0) && ((milliseconds - last_correct_input_time) >= PASSWORD_TIMEOUT_MS))
            {
                printf("Timeout reset\r\n");
                progress = 0;
                ready = 1;
                last_correct_input_time = 0;
                leds_red();
                drawLockedScreen();
            }

            tilt = detect_tilt(x_g100, y_g100);

            if (tilt != TILT_NONE)
            {
                printf("Tilt detected: %s\r\n", tiltToString(tilt));
            }

            if (ready && (tilt != TILT_NONE))
            {
                ready = 0;

                if (tilt == password[progress])
                {
                    printf("Correct! Step %d/%d\r\n", progress+1, PASSWORD_LENGTH);

                    progress++;
                    last_correct_input_time = milliseconds;

                    show_progress(progress);

                    if (progress >= PASSWORD_LENGTH)
                    {
                        solve_time = milliseconds - start_time;

                        printf("UNLOCKED in %lu ms\r\n", solve_time);

                        show_progress(PASSWORD_LENGTH);
                        drawUnlockedScreen(solve_time);

                        while (system_enabled)
                        {
                            button_state = (BUTTON_PORT->IDR & (1 << BUTTON_PIN)) ? 1 : 0;

                            if ((last_button_state == 1) && (button_state == 0))
                            {
                                system_enabled = 0;
                                progress = 0;
                                ready = 1;
                                last_correct_input_time = 0;
                                leds_red();
                                STATUS_LED_PORT->ODR &= ~(1 << STATUS_LED_PIN);
                                drawLockedScreen();
                                printf("System DISABLED\r\n");
                                delay_ms(200);
                            }

                            last_button_state = button_state;
                            delay_ms(50);
                        }
                    }
                }
                else
                {
                    printf("Wrong tilt: %s (ignored)\r\n", tiltToString(tilt));
                }
            }
        }

        delay_ms(150); // slower for readable serial
    }
}

// ===== Remaining functions unchanged =====

static uint8_t detect_tilt(int32_t x_g100, int32_t y_g100)
{
    if ((y_g100 < 200) && (y_g100 > -200))
    {
        if (x_g100 > X_THRESHOLD) return TILT_RIGHT;
        if (x_g100 < -X_THRESHOLD) return TILT_LEFT;
    }

    if ((x_g100 < 300) && (x_g100 > -300))
    {
        if (y_g100 > Y_FORWARD_THRESHOLD) return TILT_FORWARD;
        if (y_g100 < Y_BACK_THRESHOLD) return TILT_BACK;
    }

    return TILT_NONE;
}

static void leds_red(void)
{
    LED1_PORT->ODR |= (1 << LED1_PIN);
    LED2_PORT->ODR |= (1 << LED2_PIN);
    LED3_PORT->ODR |= (1 << LED3_PIN);
    LED4_PORT->ODR |= (1 << LED4_PIN);
}

static void led_green(GPIO_TypeDef *port, uint32_t pin)
{
    port->ODR &= ~(1 << pin);
}

static void show_progress(uint8_t progress)
{
    leds_red();

    if (progress >= 1) led_green(LED1_PORT, LED1_PIN);
    if (progress >= 2) led_green(LED2_PORT, LED2_PIN);
    if (progress >= 3) led_green(LED3_PORT, LED3_PIN);
    if (progress >= 4) led_green(LED4_PORT, LED4_PIN);
}

static void setup(void)
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 1);

    initClocks();
    initSysTick();
    init_display();
    initI2C();

    pinMode(BUTTON_PORT, BUTTON_PIN, 0);
    enablePullUp(BUTTON_PORT, BUTTON_PIN);

    pinMode(STATUS_LED_PORT, STATUS_LED_PIN, 1);
    STATUS_LED_PORT->ODR &= ~(1 << STATUS_LED_PIN);
}

static void accel_init(void)
{
    I2CStart(ACCEL_ADDR, WRITE, 2);
    I2CWrite(0x7E);
    I2CWrite(0x11);
    I2CStop();
    delay_ms(100);
}

static int16_t accel_read_axis(uint8_t reg)
{
    uint16_t low, high;

    I2CStart(ACCEL_ADDR, WRITE, 1);
    I2CWrite(reg);
    I2CReStart(ACCEL_ADDR, READ, 2);

    low = I2CRead();
    high = I2CRead();

    I2CStop();

    return (int16_t)(low | (high << 8));
}

static void initSerial(uint32_t baudrate)
{
    RCC->AHB2ENR |= (1 << 0);
    RCC->APB1ENR1 |= (1 << 17);

    pinMode(GPIOA, 2, 2);
    selectAlternateFunction(GPIOA, 2, 7);

    uint32_t divisor = 80000000 / baudrate;

    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = (1 << 12);
    USART2->BRR = divisor;

    USART2->CR1 |= (1 << 3) | (1 << 2) | (1 << 0);
}

int _write(int file, char *data, int len)
{
    while (len--) eputc(*data++);
    return 0;
}

static void eputc(char c)
{
    while ((USART2->ISR & (1 << 6)) == 0);
    USART2->TDR = c;
}