/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : menu_joystick.c
  * @brief          : Temporary Y-axis-only joystick navigation for UI screens.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "menu_joystick.h"

#include "stm32f4xx.h"

#define MENU_JOYSTICK_ADC_Y_CHANNEL 1U
#define MENU_JOYSTICK_DEADZONE 650
#define MENU_JOYSTICK_ADC_TIMEOUT 10000U
#define MENU_JOYSTICK_CENTER_SAMPLES 16U

static uint16_t s_centerY = 2048U;
static uint8_t s_yWasActive = 0U;

static uint16_t MenuJoystick_ReadY(void);

void MenuJoystick_Init(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  (void)RCC->AHB1ENR;

  GPIOA->MODER |= GPIO_MODER_MODE1;
  GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD1;

  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
  (void)RCC->APB2ENR;

  ADC->CCR = (ADC->CCR & ~ADC_CCR_ADCPRE) | ADC_CCR_ADCPRE_0;
  ADC1->CR1 = 0U;
  ADC1->CR2 = 0U;
  ADC1->SQR1 = 0U;
  ADC1->SMPR2 |= (ADC_SMPR2_SMP1_2 | ADC_SMPR2_SMP1_1 | ADC_SMPR2_SMP1_0);
  ADC1->CR2 |= ADC_CR2_ADON;

  uint32_t sumY = 0U;

  for (uint8_t i = 0U; i < MENU_JOYSTICK_CENTER_SAMPLES; i++)
  {
    sumY += MenuJoystick_ReadY();
  }

  s_centerY = (uint16_t)(sumY / MENU_JOYSTICK_CENTER_SAMPLES);
  s_yWasActive = 0U;
}

MenuJoystickEvent MenuJoystick_ReadEvent(void)
{
  uint16_t y = MenuJoystick_ReadY();
  int16_t dy = (int16_t)y - (int16_t)s_centerY;
  uint8_t yIsActive = ((dy > MENU_JOYSTICK_DEADZONE) ||
                       (dy < -MENU_JOYSTICK_DEADZONE)) ? 1U : 0U;
  MenuJoystickEvent event = MENU_JOYSTICK_EVENT_NONE;

  if ((yIsActive != 0U) && (s_yWasActive == 0U))
  {
    event = (dy > 0) ? MENU_JOYSTICK_EVENT_NEXT : MENU_JOYSTICK_EVENT_CONFIRM;
  }

  s_yWasActive = yIsActive;
  return event;
}

static uint16_t MenuJoystick_ReadY(void)
{
  uint32_t timeout = MENU_JOYSTICK_ADC_TIMEOUT;

  ADC1->SQR3 = MENU_JOYSTICK_ADC_Y_CHANNEL & ADC_SQR3_SQ1;
  ADC1->SR = 0U;
  ADC1->CR2 |= ADC_CR2_SWSTART;

  while (((ADC1->SR & ADC_SR_EOC) == 0U) && (timeout > 0U))
  {
    timeout--;
  }

  if (timeout == 0U)
  {
    return 2048U;
  }

  return (uint16_t)(ADC1->DR & 0x0FFFU);
}
