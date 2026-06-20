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

static uint16_t s_runBuffer[LCD_PORT_WIDTH];

static void SpriteRender_FlushRun(uint16_t startX, uint16_t y, uint16_t width)
{
  if (width == 0U)
  {
    return;
  }

  LCD_Port_DrawPixels(startX, y, width, s_runBuffer);
}

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

    uint16_t runWidth = 0U;
    uint16_t runStartX = 0U;

    for (uint16_t col = 0U; col < width; col++)
    {
      uint16_t srcCol = flipX ? (uint16_t)(width - 1U - col) : col;
      uint16_t color = pixels[(row * width) + srcCol];
      int16_t screenX = (int16_t)(x + (int16_t)col);

      if ((color == SPRITE_COLOR_KEY_RGB565) ||
          (screenX < 0) ||
          (screenX >= (int16_t)LCD_PORT_WIDTH))
      {
        SpriteRender_FlushRun(runStartX, (uint16_t)screenY, runWidth);
        runWidth = 0U;
        continue;
      }

      if (runWidth == 0U)
      {
        runStartX = (uint16_t)screenX;
      }

      s_runBuffer[runWidth] = color;
      runWidth++;
    }

    SpriteRender_FlushRun(runStartX, (uint16_t)screenY, runWidth);
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

    uint16_t runWidth = 0U;
    uint16_t runStartX = 0U;

    for (uint16_t col = 0U; col < width; col++)
    {
      uint16_t srcCol = flipX ? (uint16_t)(width - 1U - col) : col;
      uint32_t index = (uint32_t)(row * width) + srcCol;
      uint16_t oldColor = oldPixels[index];
      uint16_t newColor = newPixels[index];
      int16_t screenX = (int16_t)(x + (int16_t)col);

      if ((oldColor == newColor) ||
          (screenX < 0) ||
          (screenX >= (int16_t)LCD_PORT_WIDTH))
      {
        SpriteRender_FlushRun(runStartX, (uint16_t)screenY, runWidth);
        runWidth = 0U;
        continue;
      }

      uint16_t outColor;

      if (newColor != SPRITE_COLOR_KEY_RGB565)
      {
        outColor = newColor;
      }
      else if (oldColor != SPRITE_COLOR_KEY_RGB565)
      {
        outColor = backgroundAt((uint16_t)screenX, (uint16_t)screenY);
      }
      else
      {
        SpriteRender_FlushRun(runStartX, (uint16_t)screenY, runWidth);
        runWidth = 0U;
        continue;
      }

      if (runWidth == 0U)
      {
        runStartX = (uint16_t)screenX;
      }

      s_runBuffer[runWidth] = outColor;
      runWidth++;
    }

    SpriteRender_FlushRun(runStartX, (uint16_t)screenY, runWidth);
  }
}

void SpriteRender_Erase(int16_t x,
                        int16_t y,
                        const uint16_t *pixels,
                        uint16_t width,
                        uint16_t height,
                        uint8_t flipX,
                        SpriteRender_BackgroundAt backgroundAt)
{
  if ((pixels == 0) || (backgroundAt == 0))
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

    uint16_t runWidth = 0U;
    uint16_t runStartX = 0U;

    for (uint16_t col = 0U; col < width; col++)
    {
      uint16_t srcCol = flipX ? (uint16_t)(width - 1U - col) : col;
      uint16_t color = pixels[(row * width) + srcCol];
      int16_t screenX = (int16_t)(x + (int16_t)col);

      if ((color == SPRITE_COLOR_KEY_RGB565) ||
          (screenX < 0) ||
          (screenX >= (int16_t)LCD_PORT_WIDTH))
      {
        SpriteRender_FlushRun(runStartX, (uint16_t)screenY, runWidth);
        runWidth = 0U;
        continue;
      }

      if (runWidth == 0U)
      {
        runStartX = (uint16_t)screenX;
      }

      s_runBuffer[runWidth] = backgroundAt((uint16_t)screenX, (uint16_t)screenY);
      runWidth++;
    }

    SpriteRender_FlushRun(runStartX, (uint16_t)screenY, runWidth);
  }
}
