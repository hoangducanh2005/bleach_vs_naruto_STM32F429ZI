/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : lcd_port.c
  * @brief          : Weak LCD adapter functions for the game background demo.
  ******************************************************************************
  *
  * Replace these weak functions with calls to the real LCD driver used by your
  * board, for example BSP_LCD_*, ILI9341_*, ST7789_* or an FSMC LCD driver.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#if defined(__GNUC__)
#pragma GCC optimize ("O2")
#endif

#include "lcd_port.h"

#include "ILI9341_STM32_Driver.h"

void LCD_Port_Init(void)
{
  ILI9341_Init();
  ILI9341_SetRotation(SCREEN_HORIZONTAL_1);
  ILI9341_FillScreen(BLACK);
}

void LCD_Port_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
  if ((x >= LCD_PORT_WIDTH) || (y >= LCD_PORT_HEIGHT))
  {
    return;
  }

  ILI9341_DrawPixel(x, y, color);
}

void LCD_Port_DrawPixels(uint16_t x, uint16_t y, uint16_t width, const uint16_t *colors)
{
  if ((width == 0U) || (colors == 0) ||
      (x >= LCD_PORT_WIDTH) || (y >= LCD_PORT_HEIGHT))
  {
    return;
  }

  if (width > (uint16_t)(LCD_PORT_WIDTH - x))
  {
    width = (uint16_t)(LCD_PORT_WIDTH - x);
  }

  ILI9341_DrawRGB565Buffer(x, y, width, 1U, colors);
}

void LCD_Port_DrawRGB565Bytes(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *bytes)
{
  if ((width == 0U) || (height == 0U) || (bytes == 0) ||
      (x >= LCD_PORT_WIDTH) || (y >= LCD_PORT_HEIGHT))
  {
    return;
  }

  if ((width > (uint16_t)(LCD_PORT_WIDTH - x)) ||
      (height > (uint16_t)(LCD_PORT_HEIGHT - y)))
  {
    return;
  }

  ILI9341_DrawRGB565Bytes(x, y, width, height, bytes);
}

void LCD_Port_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
  if ((width == 0U) || (height == 0U) ||
      (x >= LCD_PORT_WIDTH) || (y >= LCD_PORT_HEIGHT))
  {
    return;
  }

  if (width > (uint16_t)(LCD_PORT_WIDTH - x))
  {
    width = (uint16_t)(LCD_PORT_WIDTH - x);
  }
  if (height > (uint16_t)(LCD_PORT_HEIGHT - y))
  {
    height = (uint16_t)(LCD_PORT_HEIGHT - y);
  }

  ILI9341_DrawRectangle(x, y, width, height, color);
}

void LCD_Port_Flush(void)
{
}

void LCD_Port_DrawRGB565Bytes2x(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *bytes)
{
  if ((width == 0U) || (height == 0U) || (bytes == 0))
  {
    return;
  }

  // Row buffer for 2x width (e.g. 160 pixels -> 320 pixels -> 640 bytes)
  static uint8_t row_buf[320 * 2];

  for (uint16_t r = 0U; r < height; r++)
  {
    for (uint16_t c = 0U; c < width; c++)
    {
      uint32_t src_idx = ((uint32_t)r * width + c) * 2U;
      uint8_t b1 = bytes[src_idx];
      uint8_t b2 = bytes[src_idx + 1U];

      // Duplicate pixel
      row_buf[c * 4] = b1;
      row_buf[c * 4 + 1] = b2;
      row_buf[c * 4 + 2] = b1;
      row_buf[c * 4 + 3] = b2;
    }

    // Draw the upscaled row twice
    LCD_Port_DrawRGB565Bytes(x, (uint16_t)(y + r * 2U), (uint16_t)(width * 2U), 1U, row_buf);
    LCD_Port_DrawRGB565Bytes(x, (uint16_t)(y + r * 2U + 1U), (uint16_t)(width * 2U), 1U, row_buf);
  }
}
