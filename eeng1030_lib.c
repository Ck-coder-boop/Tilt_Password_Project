#include <stdint.h>
#include <stm32l432xx.h>

volatile uint32_t milliseconds = 0;

void initClocks()
{
    // Initialize the clock system to 80 MHz using PLL from MSI
    RCC->CR &= ~(1 << 24); // Make sure PLL is off

    RCC->PLLCFGR = (1 << 25) + (1 << 24) + (1 << 22) + (1 << 21) +
                   (1 << 17) + (80 << 8) + (1 << 0);

    RCC->CR |= (1 << 24); // Turn PLL on
    while ((RCC->CR & (1 << 25)) == 0)
    {
        // Wait for PLL ready
    }

    // Configure flash for 4 wait states
    FLASH->ACR &= ~((1 << 2) + (1 << 1) + (1 << 0));
    FLASH->ACR |= (1 << 2);

    // Select PLL as system clock
    RCC->CFGR |= (1 << 1) + (1 << 0);
}

void initSysTick(void)
{
    milliseconds = 0;
    SysTick->LOAD = 80000 - 1;   // 80 MHz / 1000 = 80000 counts per 1 ms
    SysTick->VAL = 0;
    SysTick->CTRL = (1 << 2) | (1 << 1) | (1 << 0); // CLKSOURCE, TICKINT, ENABLE
}

void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
    Port->PUPDR = Port->PUPDR & ~(3u << (BitNumber * 2));
    Port->PUPDR = Port->PUPDR | (1u << (BitNumber * 2));
}

void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
    /*
        Modes : 00 = input
                01 = output
                10 = alternate function
                11 = analog mode
    */
    uint32_t mode_register_value = Port->MODER;

    Mode = Mode << (2 * BitNumber);
    mode_register_value = mode_register_value & ~(3u << (BitNumber * 2));
    mode_register_value = mode_register_value | Mode;
    Port->MODER = mode_register_value;
}

void selectAlternateFunction(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t AF)
{
    if (BitNumber < 8)
    {
        Port->AFR[0] &= ~(0x0f << (4 * BitNumber));
        Port->AFR[0] |= (AF << (4 * BitNumber));
    }
    else
    {
        BitNumber = BitNumber - 8;
        Port->AFR[1] &= ~(0x0f << (4 * BitNumber));
        Port->AFR[1] |= (AF << (4 * BitNumber));
    }
}

void delay_ms(volatile uint32_t delay_time_ms)
{
    uint32_t start_time_ms = milliseconds;

    while ((milliseconds - start_time_ms) < delay_time_ms)
    {
        asm("wfi");
    }
}

void SysTick_Handler(void)
{
    milliseconds++;
}