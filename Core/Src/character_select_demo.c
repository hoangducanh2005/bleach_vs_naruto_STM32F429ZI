/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : character_select_demo.c
  * @brief          : Character select screen demo for the STM32 LCD.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "character_select_demo.h"

#include "choose_ui_assets.h"
#include "game_ui.h"
#include "lcd_port.h"
#include "stm32f4xx_hal.h"

#define CHARACTER_DEMO_STEP_MS  850U

static uint8_t s_selectedCharacter;
static uint32_t s_lastStepMs;

static uint8_t CharacterSelectDemo_GetCpuCharacter(uint8_t selectedCharacter)
{
  return (uint8_t)((selectedCharacter + 3U) % CHOOSE_CHARACTER_COUNT);
}

void CharacterSelectDemo_Init(void)
{
  s_selectedCharacter = CHOOSE_CHARACTER_NARUTO;
  s_lastStepMs = HAL_GetTick();

  LCD_Port_Init();
  GameUI_DrawCharacterSelect(s_selectedCharacter,
                             CharacterSelectDemo_GetCpuCharacter(s_selectedCharacter),
                             0U,
                             1U);
  LCD_Port_Flush();
}

void CharacterSelectDemo_Update(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_lastStepMs) < CHARACTER_DEMO_STEP_MS)
  {
    return;
  }

  uint8_t previousCharacter = s_selectedCharacter;
  s_selectedCharacter = (uint8_t)((s_selectedCharacter + 1U) % CHOOSE_CHARACTER_COUNT);
  s_lastStepMs = now;

  GameUI_UpdateCharacterSelect(previousCharacter,
                               s_selectedCharacter,
                               CharacterSelectDemo_GetCpuCharacter(s_selectedCharacter),
                               0U,
                               1U);
  LCD_Port_Flush();
}
