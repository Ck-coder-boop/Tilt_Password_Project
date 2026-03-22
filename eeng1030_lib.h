#include <stdint.h>
#include <stm32l432xx.h>

void initClocks(void);
void initSysTick(void);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void selectAlternateFunction(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t AF);
void delay_ms(volatile uint32_t delay_time_ms);