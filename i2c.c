#include "i2c.h"
#include "eeng1030_lib.h"

void initI2C(void)
{
    pinMode(GPIOB,3,1); // optional LED/debug pin
    pinMode(GPIOB,6,2); // PB6 = I2C1_SCL
    pinMode(GPIOB,7,2); // PB7 = I2C1_SDA

    selectAlternateFunction(GPIOB,6,4);
    selectAlternateFunction(GPIOB,7,4);

    RCC->APB1ENR1 |= (1 << 21); // enable I2C1

    I2C1->CR1 = 0;
    delay_ms(100);

    // 100 kHz approx for 80 MHz clock
    I2C1->TIMINGR = (7 << 28) + (4 << 20) + (2 << 16) + (49 << 8) + (49);
    I2C1->ICR |= 0xffff;
    I2C1->CR1 |= (1 << 0);
}

void ResetI2C(void)
{
    I2C1->CR1 &= ~(1 << 0);
    while(I2C1->CR1 & (1 << 0));
    I2C1->CR1 = (1 << 0);
}

void I2CStart(uint8_t address, int rw, int nbytes)
{
    unsigned Reg;

    Reg = I2C1->CR2;
    Reg &= ~(1 << 13);
    Reg &= ~(0x3ff);
    Reg |= ((address << 1) & 0x3ff);

    if (rw == READ)
        Reg |= (1 << 10);
    else
        Reg &= ~(1 << 10);

    Reg &= ~(0x00ff0000);
    Reg |= (nbytes << 16);
    Reg |= (1 << 13);

    I2C1->CR2 = Reg;

    while((I2C1->ISR & (1 << 0)) == 0);
}

void I2CReStart(uint8_t address, int rw, int nbytes)
{
    unsigned Reg;

    Reg = I2C1->CR2;
    Reg &= ~(1 << 13);
    Reg &= ~(0x3ff);
    Reg |= ((address << 1) & 0x3ff);

    if (rw == READ)
        Reg |= (1 << 10);
    else
        Reg &= ~(1 << 10);

    Reg &= ~(0x00ff0000);
    Reg |= (nbytes << 16);
    Reg &= ~(1 << 24);
    Reg |= (1 << 13);

    I2C1->CR2 = Reg;

    while((I2C1->ISR & (1 << 0)) == 0);
}

void I2CStop(void)
{
    I2C1->CR2 &= ~(1 << 24);
    delay_ms(10);
    I2C1->CR2 |= (1 << 14);
    while((I2C1->ISR & (1 << 0)) == 0);
}

void I2CWrite(uint8_t Data)
{
    while((I2C1->ISR & (1 << 0)) == 0);
    I2C1->TXDR = Data;
    while((I2C1->ISR & (1 << 0)) == 0);
}

uint8_t I2CRead(void)
{
    while((I2C1->ISR & (1 << 2)) == 0);
    return I2C1->RXDR;
}