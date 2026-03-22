#include "spi.h"
#include "eeng1030_lib.h"

void initSPI(SPI_TypeDef *spi)
{
    int drain;

    RCC->APB2ENR |= (1 << 12); // SPI1 clock enable

    pinMode(GPIOA, 7, 2);
    pinMode(GPIOA, 1, 2);
    pinMode(GPIOA, 11, 2);

    selectAlternateFunction(GPIOA, 7, 5);
    selectAlternateFunction(GPIOA, 1, 5);
    selectAlternateFunction(GPIOA, 11, 5);

    drain = spi->SR;

    spi->CR1 = (1 << 9) + (1 << 8) + (1 << 6) + (1 << 3) +
               (1 << 2) + (1 << 1) + (1 << 0);

    spi->CR2 = (1 << 10) + (1 << 9) + (1 << 8);
}

uint8_t transferSPI8(SPI_TypeDef *spi, uint8_t data)
{
    uint8_t ReturnValue;
    volatile uint8_t *preg = (volatile uint8_t *)&spi->DR;

    while ((spi->SR & (1 << 7)) != 0)
    {
    }

    *preg = data;

    while ((spi->SR & (1 << 7)) != 0)
    {
    }

    ReturnValue = *preg;
    return ReturnValue;
}

uint16_t transferSPI16(SPI_TypeDef *spi, uint16_t data)
{
    uint32_t ReturnValue;

    while ((spi->SR & (1 << 7)) != 0)
    {
    }

    spi->DR = data;

    while ((spi->SR & (1 << 7)) != 0)
    {
    }

    ReturnValue = spi->DR;
    return (uint16_t)ReturnValue;
}

uint8_t spi_exchange(SPI_TypeDef *spi, uint8_t d_out[], uint32_t d_out_len, uint8_t d_in[], uint32_t d_in_len)
{
    unsigned index = 0;
    uint8_t ReturnValue = 0;

    while (d_out_len--)
    {
        transferSPI8(spi, d_out[index]);
        index++;
    }

    index = 0;

    while (d_in_len--)
    {
        d_in[index] = transferSPI8(spi, 0xFF);
        index++;
    }

    return ReturnValue;
}