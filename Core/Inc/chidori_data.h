/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : chidori_data.h
  * @brief          : Chidori effects data structures and declarations.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __CHIDORI_DATA_H
#define __CHIDORI_DATA_H

#include <stdint.h>

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
} ChidoriFrame;

extern const ChidoriFrame chidori_1_frames[4];
extern const ChidoriFrame chidori_2_frames[3];
extern const ChidoriFrame chidori_3_frames[3];
extern const ChidoriFrame chidori_4_frames[2];

#endif /* __CHIDORI_DATA_H */
