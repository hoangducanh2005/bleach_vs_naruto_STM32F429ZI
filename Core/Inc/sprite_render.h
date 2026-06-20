/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : sprite_render.h
  * @brief          : Sprite renderer with RGB565 color-key transparency.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __SPRITE_RENDER_H
#define __SPRITE_RENDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef uint16_t (*SpriteRender_BackgroundAt)(uint16_t x, uint16_t y);

void SpriteRender_Draw(int16_t x,
                       int16_t y,
                       const uint16_t *pixels,
                       uint16_t width,
                       uint16_t height,
                       uint8_t flipX);

void SpriteRender_DrawDiff(int16_t x,
                           int16_t y,
                           const uint16_t *oldPixels,
                           const uint16_t *newPixels,
                           uint16_t width,
                           uint16_t height,
                           uint8_t flipX,
                           SpriteRender_BackgroundAt backgroundAt);

#ifdef __cplusplus
}
#endif

#endif /* __SPRITE_RENDER_H */
