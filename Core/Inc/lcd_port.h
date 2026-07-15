/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : lcd_port.h
  * @brief          : Small LCD adapter used by the game background demo.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __LCD_PORT_H
#define __LCD_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define LCD_PORT_WIDTH   320U
#define LCD_PORT_HEIGHT  240U

void LCD_Port_Init(void);
void LCD_Port_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void LCD_Port_DrawPixels(uint16_t x, uint16_t y, uint16_t width, const uint16_t *colors);
void LCD_Port_DrawRGB565Bytes(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *bytes);
void LCD_Port_DrawRGB565Bytes2x(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *bytes);
void LCD_Port_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_Port_Flush(void);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_PORT_H */
