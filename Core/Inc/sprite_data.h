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

#define ICHIGO_GETSUGA_FRAME_COUNT  5U
#define ICHIGO_GETSUGA_WIDTH        74U
#define ICHIGO_GETSUGA_HEIGHT       101U

extern const uint16_t ichigo_getsuga_0[ICHIGO_GETSUGA_WIDTH * ICHIGO_GETSUGA_HEIGHT];
extern const uint16_t ichigo_getsuga_1[ICHIGO_GETSUGA_WIDTH * ICHIGO_GETSUGA_HEIGHT];
extern const uint16_t ichigo_getsuga_2[ICHIGO_GETSUGA_WIDTH * ICHIGO_GETSUGA_HEIGHT];
extern const uint16_t ichigo_getsuga_3[ICHIGO_GETSUGA_WIDTH * ICHIGO_GETSUGA_HEIGHT];
extern const uint16_t ichigo_getsuga_4[ICHIGO_GETSUGA_WIDTH * ICHIGO_GETSUGA_HEIGHT];
extern const uint16_t * const ichigo_getsuga_frames[ICHIGO_GETSUGA_FRAME_COUNT];

#define GETSUGA_PROJECTILE_FRAME_COUNT  2U
#define GETSUGA_PROJECTILE_WIDTH        54U
#define GETSUGA_PROJECTILE_HEIGHT       75U

extern const uint16_t getsuga_projectile_0[GETSUGA_PROJECTILE_WIDTH * GETSUGA_PROJECTILE_HEIGHT];
extern const uint16_t getsuga_projectile_1[GETSUGA_PROJECTILE_WIDTH * GETSUGA_PROJECTILE_HEIGHT];
extern const uint16_t * const getsuga_projectile_frames[GETSUGA_PROJECTILE_FRAME_COUNT];

#define NARUTO_IDLE_FRAME_COUNT  4U
#define NARUTO_IDLE_WIDTH        35U
#define NARUTO_IDLE_HEIGHT       52U

extern const uint16_t naruto_idle_0[NARUTO_IDLE_WIDTH * NARUTO_IDLE_HEIGHT];
extern const uint16_t naruto_idle_1[NARUTO_IDLE_WIDTH * NARUTO_IDLE_HEIGHT];
extern const uint16_t naruto_idle_2[NARUTO_IDLE_WIDTH * NARUTO_IDLE_HEIGHT];
extern const uint16_t naruto_idle_3[NARUTO_IDLE_WIDTH * NARUTO_IDLE_HEIGHT];
extern const uint16_t * const naruto_idle_frames[NARUTO_IDLE_FRAME_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __SPRITE_DATA_H */
