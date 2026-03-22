#include <display.h>
#include <stdint.h>
#include <stm32l432xx.h>
#include "eeng1030_lib.h"
#include "font5x7.h"
#include "spi.h"

#define SCREEN_HEIGHT 80
#define SCREEN_WIDTH 160

static void CSLow(void);
static void CSHigh(void);
static void DCLow(void);
static void DCHigh(void);
static void ResetHigh(void);
static void ResetLow(void);
static void command(uint8_t cmd_value);
static void data(uint8_t data_value);
static void data16(uint16_t data_value);
void clear(void);
static uint32_t mystrlen(const char *s);
static void drawLineLowSlope(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t Colour);
static void drawLineHighSlope(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t Colour);
static int iabs(int x);
static void openAperture(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

void putPixel(uint16_t x, uint16_t y, uint16_t colour);
void putImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *Image, int hOrientation, int vOrientation);
void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t Colour);
void fillRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour);

void init_display()
{
    pinMode(GPIOA, 6, 1);
    pinMode(GPIOA, 5, 1);
    pinMode(GPIOA, 4, 1);

    initSPI(SPI1);

    ResetHigh();
    delay_ms(10);
    ResetLow();
    delay_ms(200);
    ResetHigh();

    CSHigh();
    delay_ms(200);

    CSLow();
    delay_ms(20);

    command(0x01); // software reset
    delay_ms(100);

    CSHigh();
    delay_ms(1);
    CSLow();
    delay_ms(1);

    command(0x11); // exit sleep
    delay_ms(120);

    CSHigh();
    delay_ms(1);
    CSLow();
    delay_ms(1);

    command(0xB1);
    data(0x05);
    data(0x3C);
    data(0x3C);

    CSHigh();
    delay_ms(1);
    CSLow();
    delay_ms(1);

    command(0xB2);
    data(0x05);
    data(0x3C);
    data(0x3C);

    CSHigh();
    delay_ms(1);
    CSLow();
    delay_ms(1);

    command(0xB3);
    data(0x05);
    data(0x3C);
    data(0x3C);
    data(0x05);
    data(0x3C);
    data(0x3C);

    CSHigh();
    delay_ms(1);
    CSLow();
    delay_ms(1);

    command(0xB4); // dot invert
    data(0x03);

    CSHigh();
    delay_ms(1);
    CSLow();
    delay_ms(1);

    command(0x36); // memory access control
    data(0x60);

    CSHigh();
    delay_ms(1);
    CSLow();
    delay_ms(1);

    command(0x3A); // colour mode
    data(0x05);    // 16-bit

    CSHigh();
    delay_ms(1);
    CSLow();
    delay_ms(1);

    command(0x29); // display on
    delay_ms(100);

    CSHigh();
    delay_ms(1);
    CSLow();
    delay_ms(1);

    command(0x21);
    delay_ms(1);
    command(0x2C);

    fillRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x0000);
}

static void CSHigh(void)
{
    GPIOA->ODR |= (1 << 4);
}

static void CSLow(void)
{
    GPIOA->ODR &= ~(1 << 4);
}

static void DCHigh(void)
{
    GPIOA->ODR |= (1 << 5);
}

static void DCLow(void)
{
    GPIOA->ODR &= ~(1 << 5);
}

static void ResetHigh(void)
{
    GPIOA->ODR |= (1 << 6);
}

static void ResetLow(void)
{
    GPIOA->ODR &= ~(1 << 6);
}

static void command(uint8_t cmd_value)
{
    DCLow();
    transferSPI8(SPI1, cmd_value);
}

static void data(uint8_t data_value)
{
    DCHigh();
    transferSPI8(SPI1, data_value);
}

static void data16(uint16_t data_value)
{
    DCHigh();
    transferSPI8(SPI1, (uint8_t)(data_value >> 8));
    transferSPI8(SPI1, (uint8_t)(data_value & 0xFF));
}

void openAperture(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    x1 = x1 + 1;
    x2 = x2 + 1;
    y1 = y1 + 26;
    y2 = y2 + 26;

    command(0x2A);
    data(x1 >> 8);
    data(x1 & 0xFF);
    data(x2 >> 8);
    data(x2 & 0xFF);

    command(0x2B);
    data(y1 >> 8);
    data(y1 & 0xFF);
    data(y2 >> 8);
    data(y2 & 0xFF);

    command(0x2C);
}

void fillRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour)
{
    uint32_t pixelcount = (uint32_t)height * (uint32_t)width;

    openAperture(x, y, x + width - 1, y + height - 1);

    while (pixelcount--)
    {
        data16(colour);
    }
}

void putPixel(uint16_t x, uint16_t y, uint16_t colour)
{
    openAperture(x, y, x, y);
    data16(colour);
}

void putImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *Image, int hOrientation, int vOrientation)
{
    uint16_t Colour;
    uint32_t offset = 0;
    uint16_t row_index;
    uint16_t col_index;

    openAperture(x, y, x + width - 1, y + height - 1);

    if (hOrientation == 0)
    {
        if (vOrientation == 0)
        {
            for (row_index = 0; row_index < height; row_index++)
            {
                offset = row_index * width;
                for (col_index = 0; col_index < width; col_index++)
                {
                    Colour = Image[offset + col_index];
                    data16(Colour);
                }
            }
        }
        else
        {
            for (row_index = 0; row_index < height; row_index++)
            {
                offset = (height - (row_index + 1)) * width;
                for (col_index = 0; col_index < width; col_index++)
                {
                    Colour = Image[offset + col_index];
                    data16(Colour);
                }
            }
        }
    }
    else
    {
        if (vOrientation == 0)
        {
            for (row_index = 0; row_index < height; row_index++)
            {
                offset = row_index * width;
                for (col_index = 0; col_index < width; col_index++)
                {
                    Colour = Image[offset + (width - col_index - 1)];
                    data16(Colour);
                }
            }
        }
        else
        {
            for (row_index = 0; row_index < height; row_index++)
            {
                offset = (height - (row_index + 1)) * width;
                for (col_index = 0; col_index < width; col_index++)
                {
                    Colour = Image[offset + (width - col_index - 1)];
                    data16(Colour);
                }
            }
        }
    }
}

void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t Colour)
{
    if (iabs(y1 - y0) < iabs(x1 - x0))
    {
        if (x0 > x1)
        {
            drawLineLowSlope(x1, y1, x0, y0, Colour);
        }
        else
        {
            drawLineLowSlope(x0, y0, x1, y1, Colour);
        }
    }
    else
    {
        if (y0 > y1)
        {
            drawLineHighSlope(x1, y1, x0, y0, Colour);
        }
        else
        {
            drawLineHighSlope(x0, y0, x1, y1, Colour);
        }
    }
}

int iabs(int x)
{
    if (x < 0)
    {
        x = -x;
    }
    return x;
}

void drawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t Colour)
{
    drawLine(x, y, x + w, y, Colour);
    drawLine(x, y, x, y + h, Colour);
    drawLine(x + w, y, x + w, y + h, Colour);
    drawLine(x, y + h, x + w, y + h, Colour);
}

void drawCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t Colour)
{
    uint16_t x = radius - 1;
    uint16_t y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);

    if (radius > x0) return;
    if (radius > y0) return;
    if ((x0 + radius) > SCREEN_WIDTH) return;
    if ((y0 + radius) > SCREEN_HEIGHT) return;

    while (x >= y)
    {
        putPixel(x0 + x, y0 + y, Colour);
        putPixel(x0 + y, y0 + x, Colour);
        putPixel(x0 - y, y0 + x, Colour);
        putPixel(x0 - x, y0 + y, Colour);
        putPixel(x0 - x, y0 - y, Colour);
        putPixel(x0 - y, y0 - x, Colour);
        putPixel(x0 + y, y0 - x, Colour);
        putPixel(x0 + x, y0 - y, Colour);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }

        if (err > 0)
        {
            x--;
            dx += 2;
            err += dx - (radius << 1);
        }
    }
}

void fillCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t Colour)
{
    uint16_t x = radius - 1;
    uint16_t y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);

    if (radius > x0) return;
    if (radius > y0) return;
    if ((x0 + radius) > SCREEN_WIDTH) return;
    if ((y0 + radius) > SCREEN_HEIGHT) return;

    while (x >= y)
    {
        drawLine(x0 - x, y0 + y, x0 + x, y0 + y, Colour);
        drawLine(x0 - y, y0 + x, x0 + y, y0 + x, Colour);
        drawLine(x0 - x, y0 - y, x0 + x, y0 - y, Colour);
        drawLine(x0 - y, y0 - x, x0 + y, y0 - x, Colour);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }

        if (err > 0)
        {
            x--;
            dx += 2;
            err += dx - (radius << 1);
        }
    }
}

void printText(const char *Text, uint16_t x, uint16_t y, uint16_t ForeColour, uint16_t BackColour)
{
    uint8_t Index = 0;
    uint8_t Row, Col;
    const uint8_t *CharacterCode = 0;
    uint16_t TextBox[FONT_WIDTH * FONT_HEIGHT];
    uint16_t len;

    len = (uint16_t)mystrlen(Text);

    for (Index = 0; Index < len; Index++)
    {
        CharacterCode = &Font5x7[FONT_WIDTH * (Text[Index] - 32)];

        Col = 0;
        while (Col < FONT_WIDTH)
        {
            Row = 0;
            while (Row < FONT_HEIGHT)
            {
                if (CharacterCode[Col] & (1 << Row))
                {
                    TextBox[(Row * FONT_WIDTH) + Col] = ForeColour;
                }
                else
                {
                    TextBox[(Row * FONT_WIDTH) + Col] = BackColour;
                }
                Row++;
            }
            Col++;
        }

        putImage(x, y, FONT_WIDTH, FONT_HEIGHT, (uint16_t *)TextBox, 0, 0);
        x = x + FONT_WIDTH + 2;
    }
}

void printTextX2(const char *Text, uint16_t x, uint16_t y, uint16_t ForeColour, uint16_t BackColour)
{
#define Scale 2
    uint8_t Index = 0;
    uint8_t Row, Col;
    const uint8_t *CharacterCode = 0;
    uint16_t len;
    uint16_t TextBox[FONT_WIDTH * FONT_HEIGHT * Scale * Scale];

    len = (uint16_t)mystrlen(Text);

    for (Index = 0; Index < len; Index++)
    {
        CharacterCode = &Font5x7[FONT_WIDTH * (Text[Index] - 32)];

        Col = 0;
        while (Col < FONT_WIDTH)
        {
            Row = 0;
            while (Row < FONT_HEIGHT)
            {
                if (CharacterCode[Col] & (1 << Row))
                {
                    TextBox[((Row * Scale) * FONT_WIDTH * Scale) + (Col * Scale)] = ForeColour;
                    TextBox[((Row * Scale) * FONT_WIDTH * Scale) + (Col * Scale) + 1] = ForeColour;
                    TextBox[(((Row * Scale) + 1) * FONT_WIDTH * Scale) + (Col * Scale)] = ForeColour;
                    TextBox[(((Row * Scale) + 1) * FONT_WIDTH * Scale) + (Col * Scale) + 1] = ForeColour;
                }
                else
                {
                    TextBox[((Row * Scale) * FONT_WIDTH * Scale) + (Col * Scale)] = BackColour;
                    TextBox[((Row * Scale) * FONT_WIDTH * Scale) + (Col * Scale) + 1] = BackColour;
                    TextBox[(((Row * Scale) + 1) * FONT_WIDTH * Scale) + (Col * Scale)] = BackColour;
                    TextBox[(((Row * Scale) + 1) * FONT_WIDTH * Scale) + (Col * Scale) + 1] = BackColour;
                }
                Row++;
            }
            Col++;
        }

        putImage(x, y, FONT_WIDTH * Scale, FONT_HEIGHT * Scale, (uint16_t *)TextBox, 0, 0);
        x = x + FONT_WIDTH * Scale + 2;
    }
}

void printNumber(uint16_t Number, uint16_t x, uint16_t y, uint16_t ForeColour, uint16_t BackColour)
{
    char Buffer[6];

    Buffer[5] = 0;
    Buffer[4] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[3] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[2] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[1] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[0] = Number % 10 + '0';

    printText(Buffer, x, y, ForeColour, BackColour);
}

void printNumberX2(uint16_t Number, uint16_t x, uint16_t y, uint16_t ForeColour, uint16_t BackColour)
{
    char Buffer[6];

    Buffer[5] = 0;
    Buffer[4] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[3] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[2] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[1] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[0] = Number % 10 + '0';

    printTextX2(Buffer, x, y, ForeColour, BackColour);
}

uint16_t swap_bytes(uint16_t val)
{
    uint16_t b1, b2;
    b1 = val & 0xFF;
    b2 = val >> 8;
    return (b1 << 8) + b2;
}

uint16_t RGBToWord(uint16_t R, uint16_t G, uint16_t B)
{
    uint16_t rvalue = 0;
    rvalue += G >> 5;
    rvalue += (G & (0x1C)) << 11;
    rvalue += (R >> 3) << 8;
    rvalue += (B >> 3) << 3;
    return rvalue;
}

void drawLineLowSlope(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t Colour)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yi = 1;
    int D;
    int y;
    int x;

    if (dy < 0)
    {
        yi = -1;
        dy = -dy;
    }

    D = 2 * dy - dx;
    y = y0;

    for (x = x0; x <= (int)x1; x++)
    {
        putPixel((uint16_t)x, (uint16_t)y, Colour);

        if (D > 0)
        {
            y = y + yi;
            D = D - 2 * dx;
        }

        D = D + 2 * dy;
    }
}

void drawLineHighSlope(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t Colour)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;
    int D;
    int x;
    int y;

    if (dx < 0)
    {
        xi = -1;
        dx = -dx;
    }

    D = 2 * dx - dy;
    x = x0;

    for (y = y0; y <= (int)y1; y++)
    {
        putPixel((uint16_t)x, (uint16_t)y, Colour);

        if (D > 0)
        {
            x = x + xi;
            D = D - 2 * dy;
        }

        D = D + 2 * dx;
    }
}

void clear()
{
    fillRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x0000);
}

uint32_t mystrlen(const char *s)
{
    uint32_t len = 0;

    while (*s)
    {
        s++;
        len++;
    }

    return len;
}