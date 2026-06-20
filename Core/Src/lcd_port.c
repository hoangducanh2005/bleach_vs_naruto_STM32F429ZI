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
  ILI9341_DrawPixel(x, y, color);
}

void LCD_Port_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
  if ((width == 0U) || (height == 0U))
  {
    return;
  }

  ILI9341_DrawRectangle(x, y, width, height, color);
}

void LCD_Port_Flush(void)
{
}
