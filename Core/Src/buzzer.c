#include "buzzer.h"

#define BUZZER_MAX_STEPS 8

typedef struct
{
  uint16_t steps[BUZZER_MAX_STEPS];
  uint8_t step_count;
} BuzzerPattern;

/* Beep pattern timing mapping for each BuzzerSfx.
 * Even steps (0, 2, 4...) are Buzzer ON (GPIO write 0).
 * Odd steps (1, 3, 5...) are Buzzer OFF (GPIO write 1).
 * Timings are in milliseconds.
 */
static const BuzzerPattern s_patterns[] = {
  /* BUZZER_SFX_NONE */
  { { 0 }, 0 },
  
  /* BUZZER_SFX_MENU_MOVE: 25ms chirp */
  { { 25 }, 1 },
  
  /* BUZZER_SFX_MENU_SELECT: double-beep (50ms on, 40ms off, 50ms on) */
  { { 50, 40, 50 }, 3 },
  
  /* BUZZER_SFX_HIT: 45ms strike chirp */
  { { 45 }, 1 },
  
  /* BUZZER_SFX_BLOCK: short double-click (15ms on, 20ms off, 15ms on) */
  { { 15, 20, 15 }, 3 },
  
  /* BUZZER_SFX_SKILL: 120ms long beep */
  { { 120 }, 1 },
  
  /* BUZZER_SFX_KNOCKDOWN: 250ms heavy beep */
  { { 250 }, 1 },
  
  /* BUZZER_SFX_GAME_WIN: victory pattern (100ms on, 50ms off, 100ms on, 50ms off, 300ms on) */
  { { 100, 50, 100, 50, 300 }, 5 },
  
  /* BUZZER_SFX_GAME_LOSE: 700ms sad continuous beep */
  { { 700 }, 1 }
};

static BuzzerPattern s_activePattern;
static uint8_t s_currentStep = 0U;
static uint32_t s_stepStartTimeMs = 0U;
static uint8_t s_isPlaying = 0U;

void Buzzer_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Enable GPIO Clock */
  if (BUZZER_PORT == GPIOA)      { __HAL_RCC_GPIOA_CLK_ENABLE(); }
  else if (BUZZER_PORT == GPIOB) { __HAL_RCC_GPIOB_CLK_ENABLE(); }
  else if (BUZZER_PORT == GPIOC) { __HAL_RCC_GPIOC_CLK_ENABLE(); }
  else if (BUZZER_PORT == GPIOD) { __HAL_RCC_GPIOD_CLK_ENABLE(); }
  else if (BUZZER_PORT == GPIOE) { __HAL_RCC_GPIOE_CLK_ENABLE(); }
  else if (BUZZER_PORT == GPIOF) { __HAL_RCC_GPIOF_CLK_ENABLE(); }
  else if (BUZZER_PORT == GPIOG) { __HAL_RCC_GPIOG_CLK_ENABLE(); }

  /* Set initial pin state to HIGH (Off for Active Low) before config */
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);

  /* Configure Pin as Output Push-Pull, Low speed, No Pull */
  GPIO_InitStruct.Pin = BUZZER_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);
}

void Buzzer_Play(BuzzerSfx sfx)
{
  uint32_t sfxIndex = (uint32_t)sfx;
  if (sfxIndex >= (sizeof(s_patterns) / sizeof(s_patterns[0])))
  {
    return;
  }

  s_activePattern = s_patterns[sfxIndex];
  
  if (s_activePattern.step_count == 0U)
  {
    s_isPlaying = 0U;
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET); /* Off */
    return;
  }

  s_currentStep = 0U;
  s_stepStartTimeMs = HAL_GetTick();
  s_isPlaying = 1U;

  /* Step 0 is always Buzzer ON (GPIO write 0 because active-low) */
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}

void Buzzer_Update(void)
{
  if (s_isPlaying == 0U)
  {
    return;
  }

  uint32_t now = HAL_GetTick();
  uint32_t elapsed = now - s_stepStartTimeMs;

  if (elapsed >= s_activePattern.steps[s_currentStep])
  {
    s_currentStep++;
    s_stepStartTimeMs = now;

    if (s_currentStep < s_activePattern.step_count)
    {
      /* Toggle buzzer: Even steps = ON (RESET), Odd steps = OFF (SET) */
      if ((s_currentStep & 1U) == 0U)
      {
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET); /* On */
      }
      else
      {
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);   /* Off */
      }
    }
    else
    {
      /* Pattern finished, turn off and clear playing state */
      s_isPlaying = 0U;
      HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);     /* Off */
    }
  }
}
