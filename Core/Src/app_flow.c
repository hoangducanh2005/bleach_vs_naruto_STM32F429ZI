#include "app_flow.h"

#include "battle_demo.h"
#include "choose_ui_assets.h"
#include "combat_input.h"
#include "combat_types.h"
#include "game_ui.h"
#include "ILI9341_GFX.h"
#include "lcd_port.h"
#include "stm32f4xx_hal.h"
#include "buzzer.h"

#define APP_SPLASH_DURATION_MS 1800U
#define APP_MENU_INPUT_LOCK_MS 220U
#define APP_COMBAT_LOAD_MS     2000U
#define APP_MODE_ITEMS         2U
#define APP_MAIN_MENU_ITEMS    3U
#define APP_DIFFICULTY_ITEMS   3U

#define APP_CPU_CHARACTER      CHOOSE_CHARACTER_SASUKE

typedef enum
{
  APP_SCREEN_SPLASH = 0U,
  APP_SCREEN_MODE_SELECT,
  APP_SCREEN_MAIN_MENU,
  APP_SCREEN_VS_PLAYER_MENU,
  APP_SCREEN_DIFFICULTY,
  APP_SCREEN_CHARACTER,
  APP_SCREEN_COMBAT
} AppScreen;

typedef enum
{
  APP_MENU_START = 0U,
  APP_MENU_DIFFICULTY,
  APP_MENU_CHARACTER
} AppMenuItem;

static AppScreen s_screen = APP_SCREEN_SPLASH;
static uint32_t s_screenStartedMs = 0U;
static uint8_t s_selectedMode = 0U;
static uint8_t s_selectedMenu = APP_MENU_START;
static uint8_t s_selectedDifficulty = 1U;
static uint8_t s_selectedCharacter = CHOOSE_CHARACTER_HOLLOW;
static uint8_t s_selectedPlayer2Character = CHOOSE_CHARACTER_SASUKE;
static uint8_t s_vsPlayer = 0U;
static uint8_t s_characterSelectPlayer = 1U;

static void AppFlow_ShowSplash(void);
static void AppFlow_ShowModeSelect(void);
static void AppFlow_ShowMainMenu(void);
static void AppFlow_ShowVersusPlayerMenu(void);
static void AppFlow_ShowDifficulty(void);
static void AppFlow_ShowCharacter(void);
static void AppFlow_StartCombat(void);
static uint8_t AppFlow_WrapAdd(uint8_t value, uint8_t count, int8_t delta);
static CombatCharacterId AppFlow_MapChooseCharacter(uint8_t character);

void AppFlow_Init(void)
{
  LCD_Port_Init();
  CombatInput_Init();
  AppFlow_ShowSplash();
}

void AppFlow_Update(void)
{
  uint8_t input;

  if (s_screen == APP_SCREEN_COMBAT)
  {
    if (BattleDemo_Update() != 0U)
    {
      AppFlow_ShowModeSelect();
    }
    return;
  }

  input = CombatInput_Read();

  switch (s_screen)
  {
    case APP_SCREEN_SPLASH:
      if (((uint32_t)(HAL_GetTick() - s_screenStartedMs) >= APP_SPLASH_DURATION_MS) ||
          ((input & COMBAT_INPUT_ATTACK) != 0U))
      {
        Buzzer_Play(BUZZER_SFX_MENU_SELECT);
        AppFlow_ShowModeSelect();
      }
      break;

    case APP_SCREEN_MODE_SELECT:
      if ((uint32_t)(HAL_GetTick() - s_screenStartedMs) < APP_MENU_INPUT_LOCK_MS)
      {
        break;
      }

      if ((input & COMBAT_INPUT_JUMP) != 0U)
      {
        Buzzer_Play(BUZZER_SFX_MENU_MOVE);
        s_selectedMode = AppFlow_WrapAdd(s_selectedMode, APP_MODE_ITEMS, 1);
        GameUI_DrawModeSelect(s_selectedMode);
        LCD_Port_Flush();
      }
      else if ((input & COMBAT_INPUT_ATTACK) != 0U)
      {
        Buzzer_Play(BUZZER_SFX_MENU_SELECT);
        if (s_selectedMode == 0U)
        {
          s_vsPlayer = 1U;
          s_characterSelectPlayer = 1U;
          AppFlow_ShowCharacter();
        }
        else
        {
          s_vsPlayer = 0U;
          s_characterSelectPlayer = 1U;
          AppFlow_ShowMainMenu();
        }
      }
      break;

    case APP_SCREEN_MAIN_MENU:
      if ((uint32_t)(HAL_GetTick() - s_screenStartedMs) < APP_MENU_INPUT_LOCK_MS)
      {
        break;
      }

      if ((input & COMBAT_INPUT_JUMP) != 0U)
      {
        Buzzer_Play(BUZZER_SFX_MENU_MOVE);
        s_selectedMenu = AppFlow_WrapAdd(s_selectedMenu, APP_MAIN_MENU_ITEMS, 1);
        GameUI_DrawMainMenuSelection(s_selectedMenu,
                                     s_selectedDifficulty,
                                     s_selectedCharacter,
                                     s_vsPlayer);
        LCD_Port_Flush();
      }
      else if ((input & COMBAT_INPUT_ATTACK) != 0U)
      {
        Buzzer_Play(BUZZER_SFX_MENU_SELECT);
        if (s_selectedMenu == APP_MENU_START)
        {
          s_vsPlayer = 0U;
          AppFlow_StartCombat();
        }
        else if (s_selectedMenu == APP_MENU_DIFFICULTY)
        {
          AppFlow_ShowDifficulty();
        }
        else
        {
          AppFlow_ShowCharacter();
        }
      }
      break;

    case APP_SCREEN_VS_PLAYER_MENU:
      if ((uint32_t)(HAL_GetTick() - s_screenStartedMs) < APP_MENU_INPUT_LOCK_MS)
      {
        break;
      }

      if ((input & COMBAT_INPUT_ATTACK) != 0U)
      {
        Buzzer_Play(BUZZER_SFX_MENU_SELECT);
        s_vsPlayer = 1U;
        AppFlow_StartCombat();
      }
      break;

    case APP_SCREEN_DIFFICULTY:
      if ((input & COMBAT_INPUT_JUMP) != 0U)
      {
        Buzzer_Play(BUZZER_SFX_MENU_MOVE);
        s_selectedDifficulty = AppFlow_WrapAdd(s_selectedDifficulty,
                                               APP_DIFFICULTY_ITEMS,
                                               1);
        GameUI_DrawDifficultySelect(s_selectedDifficulty);
        LCD_Port_Flush();
      }
      else if (((input & COMBAT_INPUT_ATTACK) != 0U) ||
               ((input & COMBAT_INPUT_SKILL) != 0U))
      {
        Buzzer_Play(BUZZER_SFX_MENU_SELECT);
        AppFlow_ShowMainMenu();
      }
      break;

    case APP_SCREEN_CHARACTER:
      if ((input & COMBAT_INPUT_JUMP) != 0U)
      {
        Buzzer_Play(BUZZER_SFX_MENU_MOVE);
        uint8_t previousCharacter =
            (s_characterSelectPlayer == 2U) ? s_selectedPlayer2Character : s_selectedCharacter;
        uint8_t *activeCharacter =
            (s_characterSelectPlayer == 2U) ? &s_selectedPlayer2Character : &s_selectedCharacter;
        *activeCharacter = AppFlow_WrapAdd(*activeCharacter,
                                           CHOOSE_CHARACTER_COUNT,
                                           1);
        uint8_t opponentCharacter = APP_CPU_CHARACTER;
        if (s_vsPlayer != 0U)
        {
          opponentCharacter = (s_characterSelectPlayer == 2U)
                                  ? s_selectedCharacter
                                  : s_selectedPlayer2Character;
        }
        GameUI_UpdateCharacterSelect(previousCharacter,
                                     *activeCharacter,
                                     opponentCharacter,
                                     s_vsPlayer,
                                     s_characterSelectPlayer);
        LCD_Port_Flush();
      }
      else if (((input & COMBAT_INPUT_ATTACK) != 0U) ||
               ((input & COMBAT_INPUT_SKILL) != 0U))
      {
        Buzzer_Play(BUZZER_SFX_MENU_SELECT);
        if ((s_vsPlayer != 0U) && (s_characterSelectPlayer == 1U))
        {
          s_characterSelectPlayer = 2U;
          AppFlow_ShowCharacter();
        }
        else if (s_vsPlayer != 0U)
        {
          AppFlow_ShowVersusPlayerMenu();
        }
        else
        {
          AppFlow_ShowMainMenu();
        }
      }
      break;

    default:
      AppFlow_ShowSplash();
      break;
  }
}

static void AppFlow_ShowSplash(void)
{
  s_screen = APP_SCREEN_SPLASH;
  s_screenStartedMs = HAL_GetTick();

  GameUI_DrawSplash();
  ILI9341_DrawText("ATTACK: SKIP", FONT1, 124U, 226U, 0xFFFFU, 0x0000U);
  LCD_Port_Flush();
}

static void AppFlow_ShowModeSelect(void)
{
  s_screen = APP_SCREEN_MODE_SELECT;
  s_screenStartedMs = HAL_GetTick();

  GameUI_DrawModeSelect(s_selectedMode);
  LCD_Port_Flush();
}

static void AppFlow_ShowMainMenu(void)
{
  s_screen = APP_SCREEN_MAIN_MENU;
  s_screenStartedMs = HAL_GetTick();

  GameUI_DrawMainMenuSelection(s_selectedMenu,
                               s_selectedDifficulty,
                               s_selectedCharacter,
                               s_vsPlayer);
  LCD_Port_Flush();
}

static void AppFlow_ShowVersusPlayerMenu(void)
{
  s_screen = APP_SCREEN_VS_PLAYER_MENU;
  s_screenStartedMs = HAL_GetTick();

  GameUI_DrawVersusPlayerMenu();
  LCD_Port_Flush();
}

static void AppFlow_ShowDifficulty(void)
{
  s_screen = APP_SCREEN_DIFFICULTY;

  GameUI_DrawDifficultySelect(s_selectedDifficulty);
  LCD_Port_Flush();
}

static void AppFlow_ShowCharacter(void)
{
  s_screen = APP_SCREEN_CHARACTER;

  uint8_t activeCharacter =
      (s_characterSelectPlayer == 2U) ? s_selectedPlayer2Character : s_selectedCharacter;

  uint8_t opponentCharacter = APP_CPU_CHARACTER;
  if (s_vsPlayer != 0U)
  {
    opponentCharacter = (s_characterSelectPlayer == 2U)
                            ? s_selectedCharacter
                            : s_selectedPlayer2Character;
  }

  GameUI_DrawCharacterSelect(activeCharacter,
                             opponentCharacter,
                             s_vsPlayer,
                             s_characterSelectPlayer);
  LCD_Port_Flush();
}

static void AppFlow_StartCombat(void)
{
  s_screen = APP_SCREEN_COMBAT;
  CombatCharacterId playerChar = AppFlow_MapChooseCharacter(s_selectedCharacter);
  CombatCharacterId opponentChar;

  if (s_vsPlayer != 0U)
  {
    opponentChar = AppFlow_MapChooseCharacter(s_selectedPlayer2Character);
  }
  else
  {
    opponentChar = (CombatCharacterId)(HAL_GetTick() % 4U);

    /* Prevent mirror match to keep combat diverse */
    if (opponentChar == playerChar)
    {
      opponentChar = (CombatCharacterId)(((uint8_t)opponentChar + 1U) % 4U);
    }
  }

  LCD_Port_FillRect(0U, 0U, 320U, 240U, 0x0000U);
  ILI9341_DrawText("LOADING", FONT4, 112U, 104U, 0xFFFFU, 0x0000U);
  LCD_Port_Flush();
  HAL_Delay(APP_COMBAT_LOAD_MS);

  BattleDemo_SetCharacters(playerChar, opponentChar);
  BattleDemo_SetVsPlayer(s_vsPlayer);
  BattleDemo_Init(s_selectedDifficulty);
}

static uint8_t AppFlow_WrapAdd(uint8_t value, uint8_t count, int8_t delta)
{
  if (count == 0U)
  {
    return 0U;
  }

  if (delta < 0)
  {
    return (value == 0U) ? (uint8_t)(count - 1U) : (uint8_t)(value - 1U);
  }

  return (uint8_t)((value + 1U) % count);
}

static CombatCharacterId AppFlow_MapChooseCharacter(uint8_t character)
{
  switch (character % CHOOSE_CHARACTER_COUNT)
  {
    case CHOOSE_CHARACTER_NARUTO:
      return COMBAT_CHARACTER_NARUTO;
    case CHOOSE_CHARACTER_NINE_TAIL:
      return COMBAT_CHARACTER_NARUTO_FULL_NINE_TAILS;
    case CHOOSE_CHARACTER_SASUKE:
      return COMBAT_CHARACTER_SASUKE;
    case CHOOSE_CHARACTER_ICHIGO:
      return COMBAT_CHARACTER_ICHIGO;
    case CHOOSE_CHARACTER_HOLLOW:
    case CHOOSE_CHARACTER_GIN:
    default:
      return COMBAT_CHARACTER_VIZARD_ICHIGO;
  }
}
