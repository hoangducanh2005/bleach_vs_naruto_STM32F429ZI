/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : sprite_render.c
  * @brief          : Sprite renderer with RGB565 color-key transparency.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "sprite_render.h"

#include "lcd_port.h"
#include "sprite_data.h"

void SpriteRender_Draw(int16_t x,
                       int16_t y,
                       const uint16_t *pixels,
                       uint16_t width,
                       uint16_t height,
                       uint8_t flipX)
{
  if (pixels == 0)
  {
    return;
  }

  for (uint16_t row = 0U; row < height; row++)
  {
    int16_t screenY = (int16_t)(y + (int16_t)row);

    if ((screenY < 0) || (screenY >= (int16_t)LCD_PORT_HEIGHT))
    {
      continue;
    }

    for (uint16_t col = 0U; col < width; col++)
    {
      uint16_t srcCol = flipX ? (uint16_t)(width - 1U - col) : col;
      uint16_t color = pixels[(row * width) + srcCol];

      if (color == SPRITE_COLOR_KEY_RGB565)
      {
        continue;
      }

      int16_t screenX = (int16_t)(x + (int16_t)col);

      if ((screenX < 0) || (screenX >= (int16_t)LCD_PORT_WIDTH))
      {
        continue;
      }

      LCD_Port_DrawPixel((uint16_t)screenX, (uint16_t)screenY, color);
    }
  }
}

void SpriteRender_DrawDiff(int16_t x,
                           int16_t y,
                           const uint16_t *oldPixels,
                           const uint16_t *newPixels,
                           uint16_t width,
                           uint16_t height,
                           uint8_t flipX,
                           SpriteRender_BackgroundAt backgroundAt)
{
  if ((oldPixels == 0) || (newPixels == 0) || (backgroundAt == 0))
  {
    return;
  }

  for (uint16_t row = 0U; row < height; row++)
  {
    int16_t screenY = (int16_t)(y + (int16_t)row);

    if ((screenY < 0) || (screenY >= (int16_t)LCD_PORT_HEIGHT))
    {
      continue;
    }

    for (uint16_t col = 0U; col < width; col++)
    {
      uint16_t srcCol = flipX ? (uint16_t)(width - 1U - col) : col;
      uint32_t index = (uint32_t)(row * width) + srcCol;
      uint16_t oldColor = oldPixels[index];
      uint16_t newColor = newPixels[index];

      if (oldColor == newColor)
      {
        continue;
      }

      int16_t screenX = (int16_t)(x + (int16_t)col);

      if ((screenX < 0) || (screenX >= (int16_t)LCD_PORT_WIDTH))
      {
        continue;
      }

      if (newColor != SPRITE_COLOR_KEY_RGB565)
      {
        LCD_Port_DrawPixel((uint16_t)screenX, (uint16_t)screenY, newColor);
      }
      else if (oldColor != SPRITE_COLOR_KEY_RGB565)
      {
        LCD_Port_DrawPixel((uint16_t)screenX,
                           (uint16_t)screenY,
                           backgroundAt((uint16_t)screenX, (uint16_t)screenY));
      }
    }
  }
}
