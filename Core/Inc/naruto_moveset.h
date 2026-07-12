/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : naruto_moveset.h
  * @brief          : Naruto gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __NARUTO_MOVESET_H
#define __NARUTO_MOVESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  NARUTO_MOVE_IDLE = 0U,
  NARUTO_MOVE_RUN = 1U,
  NARUTO_MOVE_ATTACK_LIGHT = 2U,
  NARUTO_MOVE_BLOCK = 3U,
  NARUTO_MOVE_SKILL = 4U,
  NARUTO_MOVE_JUMP = 5U,
  NARUTO_MOVE_HIT = 6U,
  NARUTO_MOVE_DEAD = 7U,
  NARUTO_MOVE_STATE_COUNT = 8U
} NarutoMoveState;

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} NarutoMoveFrame;

typedef struct
{
  const NarutoMoveFrame *frames;
  uint8_t frameCount;
  uint8_t loop;
  uint8_t holdLast;
} NarutoMoveAnimation;

extern const NarutoMoveAnimation naruto_move_animations[NARUTO_MOVE_STATE_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __NARUTO_MOVESET_H */
