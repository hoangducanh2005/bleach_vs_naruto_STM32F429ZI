#ifndef __BUZZER_H
#define __BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* Port and Pin definition for the Active Buzzer */
#define BUZZER_PORT           GPIOD
#define BUZZER_PIN            GPIO_PIN_4

/* Buzzer Sound Effects */
typedef enum
{
  BUZZER_SFX_NONE = 0,
  BUZZER_SFX_MENU_MOVE,
  BUZZER_SFX_MENU_SELECT,
  BUZZER_SFX_HIT,
  BUZZER_SFX_BLOCK,
  BUZZER_SFX_SKILL,
  BUZZER_SFX_KNOCKDOWN,
  BUZZER_SFX_GAME_WIN,
  BUZZER_SFX_GAME_LOSE
} BuzzerSfx;

/**
 * @brief Initialize the buzzer GPIO pin.
 */
void Buzzer_Init(void);

/**
 * @brief Play a specific sound effect pattern non-blockingly.
 * @param sfx The sound effect to play.
 */
void Buzzer_Play(BuzzerSfx sfx);

/**
 * @brief Update the buzzer state machine. Must be called regularly in the main loop.
 */
void Buzzer_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUZZER_H */
