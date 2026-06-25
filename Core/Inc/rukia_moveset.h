/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : rukia_moveset.h
  * @brief          : Rukia gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __RUKIA_MOVESET_H
#define __RUKIA_MOVESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  RUKIA_MOVE_IDLE = 0U,
  RUKIA_MOVE_RUN = 1U,
  RUKIA_MOVE_ATTACK1 = 2U,
  RUKIA_MOVE_ATTACK2 = 3U,
  RUKIA_MOVE_ATTACK3 = 4U,
  RUKIA_MOVE_BLOCK = 5U,
  RUKIA_MOVE_SKILL = 6U,
  RUKIA_MOVE_JUMP_STARTUP = 7U,
  RUKIA_MOVE_JUMP_AIR = 8U,
  RUKIA_MOVE_FALL = 9U,
  RUKIA_MOVE_JUMP_LAND = 10U,
  RUKIA_MOVE_HIT = 11U,
  RUKIA_MOVE_DEAD = 12U,
  RUKIA_MOVE_STATE_COUNT = 13U
} RukiaMoveState;

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} RukiaMoveFrame;

typedef struct
{
  const RukiaMoveFrame *frames;
  uint8_t frameCount;
  uint8_t loop;
  uint8_t holdLast;
  const uint16_t * const *casterEffectFrames;
  uint8_t casterEffectFrameCount;
  uint16_t casterEffectWidth;
  uint16_t casterEffectHeight;
  const uint16_t * const *projectileFrames;
  uint8_t projectileFrameCount;
  uint16_t projectileWidth;
  uint16_t projectileHeight;
} RukiaMoveAnimation;

extern const RukiaMoveAnimation rukia_move_animations[RUKIA_MOVE_STATE_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __RUKIA_MOVESET_H */
