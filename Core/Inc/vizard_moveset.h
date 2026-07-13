/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : vizard_moveset.h
  * @brief          : Vizard Ichigo gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __VIZARD_MOVESET_H
#define __VIZARD_MOVESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  VIZARD_MOVE_IDLE = 0U,
  VIZARD_MOVE_RUN = 1U,
  VIZARD_MOVE_DASH = 2U,
  VIZARD_MOVE_ATTACK_LIGHT = 3U,
  VIZARD_MOVE_ATTACK2 = 4U,
  VIZARD_MOVE_ATTACK3 = 5U,
  VIZARD_MOVE_BLOCK = 6U,
  VIZARD_MOVE_SKILL = 7U,
  VIZARD_MOVE_SKILL_PROJECTILE = 8U,
  VIZARD_MOVE_JUMP = 9U,
  VIZARD_MOVE_STATE_COUNT = 10U
} VizardMoveState;

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} VizardMoveFrame;

typedef struct
{
  const VizardMoveFrame *frames;
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
} VizardMoveAnimation;

extern const VizardMoveAnimation vizard_move_animations[VIZARD_MOVE_STATE_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __VIZARD_MOVESET_H */
