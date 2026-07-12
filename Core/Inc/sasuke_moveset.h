/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : sasuke_moveset.h
  * @brief          : Sasuke gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __SASUKE_MOVESET_H
#define __SASUKE_MOVESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  SASUKE_MOVE_IDLE = 0U,
  SASUKE_MOVE_RUN = 1U,
  SASUKE_MOVE_DASH = 2U,
  SASUKE_MOVE_ATTACK1 = 3U,
  SASUKE_MOVE_ATTACK2 = 4U,
  SASUKE_MOVE_ATTACK3 = 5U,
  SASUKE_MOVE_BLOCK = 6U,
  SASUKE_MOVE_SKILL = 7U,
  SASUKE_MOVE_JUMP_STARTUP = 8U,
  SASUKE_MOVE_JUMP_AIR = 9U,
  SASUKE_MOVE_FALL = 10U,
  SASUKE_MOVE_JUMP_LAND = 11U,
  SASUKE_MOVE_HIT = 12U,
  SASUKE_MOVE_DEAD = 13U,
  SASUKE_MOVE_STATE_COUNT = 14U
} SasukeMoveState;

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} SasukeMoveFrame;

typedef struct
{
  const SasukeMoveFrame *frames;
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
} SasukeMoveAnimation;

extern const SasukeMoveAnimation sasuke_move_animations[SASUKE_MOVE_STATE_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __SASUKE_MOVESET_H */
