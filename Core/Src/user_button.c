/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : user_button.c
  * @brief          : Debounced USER button reader for menu/OK navigation.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "user_button.h"

#include "stm32f4xx_hal.h"

#define USER_BUTTON_PORT GPIOA
#define USER_BUTTON_PIN GPIO_PIN_0
#define USER_BUTTON_DEBOUNCE_MS 40U

static GPIO_PinState s_lastRawState = GPIO_PIN_RESET;
static uint8_t s_stablePressed = 0U;
static uint8_t s_previousStablePressed = 0U;
static uint32_t s_lastRawChangeMs = 0U;

void UserButton_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = USER_BUTTON_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USER_BUTTON_PORT, &GPIO_InitStruct);

  s_lastRawState = HAL_GPIO_ReadPin(USER_BUTTON_PORT, USER_BUTTON_PIN);
  s_stablePressed = (s_lastRawState == GPIO_PIN_SET) ? 1U : 0U;
  s_previousStablePressed = s_stablePressed;
  s_lastRawChangeMs = HAL_GetTick();
}

uint8_t UserButton_IsPressed(void)
{
  GPIO_PinState rawState = HAL_GPIO_ReadPin(USER_BUTTON_PORT, USER_BUTTON_PIN);
  uint32_t now = HAL_GetTick();

  if (rawState != s_lastRawState)
  {
    s_lastRawState = rawState;
    s_lastRawChangeMs = now;
  }

  if ((now - s_lastRawChangeMs) >= USER_BUTTON_DEBOUNCE_MS)
  {
    s_stablePressed = (rawState == GPIO_PIN_SET) ? 1U : 0U;
  }

  return s_stablePressed;
}

uint8_t UserButton_WasPressed(void)
{
  uint8_t pressed = UserButton_IsPressed();
  uint8_t edge = ((pressed != 0U) && (s_previousStablePressed == 0U)) ? 1U : 0U;

  s_previousStablePressed = pressed;
  return edge;
}
