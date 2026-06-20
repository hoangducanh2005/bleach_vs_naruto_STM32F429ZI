/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : sprite_data.h
  * @brief          : RGB565 sprite data generated from PNG assets.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __SPRITE_DATA_H
#define __SPRITE_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SPRITE_COLOR_KEY_RGB565  0xF81FU
#define ICHIGO_IDLE_FRAME_COUNT  4U
#define ICHIGO_IDLE_WIDTH        64U
#define ICHIGO_IDLE_HEIGHT       48U

extern const uint16_t ichigo_idle_0[ICHIGO_IDLE_WIDTH * ICHIGO_IDLE_HEIGHT];
extern const uint16_t ichigo_idle_1[ICHIGO_IDLE_WIDTH * ICHIGO_IDLE_HEIGHT];
extern const uint16_t ichigo_idle_2[ICHIGO_IDLE_WIDTH * ICHIGO_IDLE_HEIGHT];
extern const uint16_t ichigo_idle_3[ICHIGO_IDLE_WIDTH * ICHIGO_IDLE_HEIGHT];
extern const uint16_t * const ichigo_idle_frames[ICHIGO_IDLE_FRAME_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __SPRITE_DATA_H */
