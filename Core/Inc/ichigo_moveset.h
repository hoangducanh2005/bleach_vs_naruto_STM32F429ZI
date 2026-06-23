/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ichigo_moveset.h
  * @brief          : Ichigo gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __ICHIGO_MOVESET_H
#define __ICHIGO_MOVESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  ICHIGO_MOVE_IDLE = 0U,
  ICHIGO_MOVE_RUN = 1U,
  ICHIGO_MOVE_ATTACK_LIGHT = 2U,
  ICHIGO_MOVE_BLOCK = 3U,
  ICHIGO_MOVE_SKILL = 4U,
  ICHIGO_MOVE_JUMP = 5U,
  ICHIGO_MOVE_HIT = 6U,
  ICHIGO_MOVE_DEAD = 7U,
  ICHIGO_MOVE_STATE_COUNT = 8U
} IchigoMoveState;

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} IchigoMoveFrame;

typedef struct
{
  const IchigoMoveFrame *frames;
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
} IchigoMoveAnimation;

extern const IchigoMoveAnimation ichigo_move_animations[ICHIGO_MOVE_STATE_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __ICHIGO_MOVESET_H */
