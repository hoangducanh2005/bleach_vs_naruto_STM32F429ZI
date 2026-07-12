#include "app_flow.h"

#include "battle_demo.h"
#include "choose_ui_assets.h"
#include "combat_input.h"
#include "combat_types.h"
#include "game_ui.h"
#include "ILI9341_GFX.h"
#include "lcd_port.h"
#include "stm32f4xx_hal.h"

#define APP_SPLASH_DURATION_MS 1800U
#define APP_MAIN_MENU_ITEMS    3U
#define APP_DIFFICULTY_ITEMS   3U

#define APP_CPU_CHARACTER      CHOOSE_CHARACTER_SASUKE

typedef enum
{
  APP_SCREEN_SPLASH = 0U,
  APP_SCREEN_MAIN_MENU,
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
static uint8_t s_selectedMenu = APP_MENU_START;
static uint8_t s_selectedDifficulty = 1U;
static uint8_t s_selectedCharacter = CHOOSE_CHARACTER_HOLLOW;
static uint8_t s_navLatched = 0U;

static void AppFlow_ShowSplash(void);
static void AppFlow_ShowMainMenu(void);
static void AppFlow_ShowDifficulty(void);
static void AppFlow_ShowCharacter(void);
static void AppFlow_StartCombat(void);
static int8_t AppFlow_ReadNavEvent(uint8_t input);
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
  int8_t nav;

  if (s_screen == APP_SCREEN_COMBAT)
  {
    BattleDemo_Update();
    return;
  }

  input = CombatInput_Read();
  nav = AppFlow_ReadNavEvent(input);

  switch (s_screen)
  {
    case APP_SCREEN_SPLASH:
      if (((uint32_t)(HAL_GetTick() - s_screenStartedMs) >= APP_SPLASH_DURATION_MS) ||
          ((input & COMBAT_INPUT_ATTACK) != 0U))
      {
        AppFlow_ShowMainMenu();
      }
      break;

    case APP_SCREEN_MAIN_MENU:
      if (nav != 0)
      {
        s_selectedMenu = AppFlow_WrapAdd(s_selectedMenu, APP_MAIN_MENU_ITEMS, nav);
        AppFlow_ShowMainMenu();
      }
      else if ((input & COMBAT_INPUT_ATTACK) != 0U)
      {
        if (s_selectedMenu == APP_MENU_START)
        {
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

    case APP_SCREEN_DIFFICULTY:
      if (nav != 0)
      {
        s_selectedDifficulty = AppFlow_WrapAdd(s_selectedDifficulty,
                                               APP_DIFFICULTY_ITEMS,
                                               nav);
        AppFlow_ShowDifficulty();
      }
      else if (((input & COMBAT_INPUT_ATTACK) != 0U) ||
               ((input & COMBAT_INPUT_SKILL) != 0U))
      {
        AppFlow_ShowMainMenu();
      }
      break;

    case APP_SCREEN_CHARACTER:
      if (nav != 0)
      {
        uint8_t previousCharacter = s_selectedCharacter;
        s_selectedCharacter = AppFlow_WrapAdd(s_selectedCharacter,
                                              CHOOSE_CHARACTER_COUNT,
                                              nav);
        GameUI_UpdateCharacterSelect(previousCharacter,
                                     s_selectedCharacter,
                                     APP_CPU_CHARACTER);
        LCD_Port_Flush();
      }
      else if (((input & COMBAT_INPUT_ATTACK) != 0U) ||
               ((input & COMBAT_INPUT_SKILL) != 0U))
      {
        AppFlow_ShowMainMenu();
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
  s_navLatched = 0U;

  GameUI_DrawSplash();
  ILI9341_DrawText("ATTACK: SKIP", FONT1, 124U, 226U, 0xFFFFU, 0x0000U);
  LCD_Port_Flush();
}

static void AppFlow_ShowMainMenu(void)
{
  s_screen = APP_SCREEN_MAIN_MENU;
  s_navLatched = 0U;

  GameUI_DrawMainMenuSelection(s_selectedMenu,
                               s_selectedDifficulty,
                               s_selectedCharacter);
  LCD_Port_Flush();
}

static void AppFlow_ShowDifficulty(void)
{
  s_screen = APP_SCREEN_DIFFICULTY;
  s_navLatched = 0U;

  GameUI_DrawDifficultySelect(s_selectedDifficulty);
  LCD_Port_Flush();
}

static void AppFlow_ShowCharacter(void)
{
  s_screen = APP_SCREEN_CHARACTER;
  s_navLatched = 0U;

  GameUI_DrawCharacterSelect(s_selectedCharacter, APP_CPU_CHARACTER);
  LCD_Port_Flush();
}

static void AppFlow_StartCombat(void)
{
  s_screen = APP_SCREEN_COMBAT;
  s_navLatched = 0U;
  BattleDemo_SetCharacters(AppFlow_MapChooseCharacter(s_selectedCharacter),
                           COMBAT_CHARACTER_SASUKE);
  BattleDemo_Init();
}

static int8_t AppFlow_ReadNavEvent(uint8_t input)
{
  uint8_t up = (input & COMBAT_INPUT_UP) != 0U;
  uint8_t down = (input & COMBAT_INPUT_DOWN) != 0U;

  if ((up == 0U) && (down == 0U))
  {
    s_navLatched = 0U;
    return 0;
  }

  if ((s_navLatched != 0U) || (up == down))
  {
    return 0;
  }

  s_navLatched = 1U;
  return up ? -1 : 1;
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
    case CHOOSE_CHARACTER_NINE_TAIL:
      return COMBAT_CHARACTER_NARUTO;
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
