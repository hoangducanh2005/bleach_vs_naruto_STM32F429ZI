#include "difficulty_select_demo.h"

#include "game_ui.h"
#include "lcd_port.h"
#include "stm32f4xx_hal.h"

#define DIFFICULTY_DEMO_STEP_MS 900U

static uint32_t s_lastStepMs;
static uint8_t s_selectedDifficulty;

void DifficultySelectDemo_Init(void)
{
  LCD_Port_Init();

  s_selectedDifficulty = 1U;
  s_lastStepMs = HAL_GetTick();

  GameUI_DrawDifficultySelect(s_selectedDifficulty);
  LCD_Port_Flush();
}

void DifficultySelectDemo_Update(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_lastStepMs) < DIFFICULTY_DEMO_STEP_MS)
  {
    return;
  }

  s_lastStepMs = now;
  s_selectedDifficulty = (uint8_t)((s_selectedDifficulty + 1U) % 3U);

  GameUI_DrawDifficultySelect(s_selectedDifficulty);
  LCD_Port_Flush();
}
