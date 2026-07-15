#include "gameover.h"

#include "ILI9341_GFX.h"
#include "ILI9341_STM32_Driver.h"
#include "lcd_port.h"
#include "stm32f4xx_hal.h"

#define GAMEOVER_PROMPT_PERIOD_MS 500U
#define GAMEOVER_PROMPT_X         68U
#define GAMEOVER_PROMPT_Y         215U
#define GAMEOVER_PROMPT_BG        BLACK
#define GAMEOVER_PROMPT_COLOR     WHITE

static uint32_t s_lastPromptMs;
static uint8_t s_promptVisible;

static void GameOverDemo_DrawFrame(void);
static void GameOverDemo_DrawPrompt(uint8_t visible);

void GameOverDemo_Init(void)
{
  LCD_Port_Init();
  GameOverDemo_DrawFrame();
  GameOverDemo_DrawPrompt(1U);

  s_lastPromptMs = HAL_GetTick();
  s_promptVisible = 1U;
}

void GameOverDemo_Update(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_lastPromptMs) < GAMEOVER_PROMPT_PERIOD_MS)
  {
    return;
  }

  s_lastPromptMs = now;
  s_promptVisible = (uint8_t)(s_promptVisible == 0U);
  GameOverDemo_DrawPrompt(s_promptVisible);
}

static void GameOverDemo_DrawFrame(void)
{
  LCD_Port_DrawRGB565Bytes2x(0U,
                             0U,
                             GAMEOVER_WIDTH,
                             GAMEOVER_HEIGHT,
                             gameover_map);
  LCD_Port_Flush();
}

static void GameOverDemo_DrawPrompt(uint8_t visible)
{
  const uint16_t textColor = (visible != 0U) ? GAMEOVER_PROMPT_COLOR : GAMEOVER_PROMPT_BG;

  ILI9341_DrawText("PRESS START TO RETRY",
                   FONT2,
                   GAMEOVER_PROMPT_X,
                   GAMEOVER_PROMPT_Y,
                   textColor,
                   GAMEOVER_PROMPT_BG);
}
