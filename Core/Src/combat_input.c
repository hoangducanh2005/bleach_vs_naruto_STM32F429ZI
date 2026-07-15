#include "combat_input.h"

#include "stm32f4xx.h"

#define COMBAT_INPUT_ADC_X_CHANNEL 0U
#define COMBAT_INPUT_ADC_Y_CHANNEL 1U
#define COMBAT_INPUT_ADC_TIMEOUT 10000U
#define COMBAT_INPUT_CENTER_SAMPLES 16U
#define COMBAT_INPUT_AXIS_SAMPLES 4U
#define COMBAT_INPUT_DEADZONE 650
#define COMBAT_INPUT_BUTTON_ACTIVE_LOW 1U
#define COMBAT_INPUT_ATTACK_BUTTON_PORT GPIOB
#define COMBAT_INPUT_ATTACK_BUTTON_PIN GPIO_IDR_ID0
#define COMBAT_INPUT_JUMP_BUTTON_PORT GPIOB
#define COMBAT_INPUT_JUMP_BUTTON_PIN GPIO_IDR_ID1
#define COMBAT_INPUT_SKILL_BUTTON_PORT GPIOB
#define COMBAT_INPUT_SKILL_BUTTON_PIN GPIO_IDR_ID2
#define COMBAT_INPUT_DASH_BUTTON_PORT GPIOB
#define COMBAT_INPUT_DASH_BUTTON_PIN GPIO_IDR_ID3
#define COMBAT_BUTTON_ATTACK_MASK 1U
#define COMBAT_BUTTON_JUMP_MASK 2U
#define COMBAT_BUTTON_SKILL_MASK 4U
#define COMBAT_BUTTON_DASH_MASK 8U

static uint16_t s_centerX = 2048U;
static uint16_t s_centerY = 2048U;
static uint8_t s_lastButtons;

static uint16_t CombatInput_ReadAdc(uint8_t channel);
static uint16_t CombatInput_ReadAxis(uint8_t channel);
static void CombatInput_InitButtonGpio(void);
static uint8_t CombatInput_ReadButtonsRaw(void);
static uint8_t CombatInput_IsButtonPressed(GPIO_TypeDef *port, uint32_t pin);

void CombatInput_Init(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  (void)RCC->AHB1ENR;

  GPIOA->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1);
  GPIOA->MODER |= GPIO_MODER_MODE0 | GPIO_MODER_MODE1;
  GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1);

  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
  (void)RCC->APB2ENR;

  ADC->CCR = (ADC->CCR & ~ADC_CCR_ADCPRE) | ADC_CCR_ADCPRE_0;
  ADC1->CR1 = 0U;
  ADC1->CR2 = 0U;
  ADC1->SQR1 = 0U;
  ADC1->SMPR2 |= (ADC_SMPR2_SMP0_2 | ADC_SMPR2_SMP0_1 | ADC_SMPR2_SMP0_0 |
                  ADC_SMPR2_SMP1_2 | ADC_SMPR2_SMP1_1 | ADC_SMPR2_SMP1_0);
  ADC1->CR2 |= ADC_CR2_ADON;
  CombatInput_InitButtonGpio();

  uint32_t sumX = 0U;
  uint32_t sumY = 0U;

  for (uint8_t i = 0U; i < COMBAT_INPUT_CENTER_SAMPLES; i++)
  {
    sumX += CombatInput_ReadAxis(COMBAT_INPUT_ADC_X_CHANNEL);
    sumY += CombatInput_ReadAxis(COMBAT_INPUT_ADC_Y_CHANNEL);
  }

  s_centerX = (uint16_t)(sumX / COMBAT_INPUT_CENTER_SAMPLES);
  s_centerY = (uint16_t)(sumY / COMBAT_INPUT_CENTER_SAMPLES);
  s_lastButtons = CombatInput_ReadButtonsRaw();
}

uint8_t CombatInput_Read(void)
{
  uint8_t input = COMBAT_INPUT_NONE;
  uint8_t buttons = CombatInput_ReadButtonsRaw();
  uint8_t pressedEdges = (uint8_t)(buttons & (uint8_t)~s_lastButtons);
  int16_t dx = (int16_t)CombatInput_ReadAxis(COMBAT_INPUT_ADC_X_CHANNEL) -
               (int16_t)s_centerX;
  int16_t dy = (int16_t)CombatInput_ReadAxis(COMBAT_INPUT_ADC_Y_CHANNEL) -
               (int16_t)s_centerY;

  s_lastButtons = buttons;

  if (dx > COMBAT_INPUT_DEADZONE)
  {
    input |= COMBAT_INPUT_RIGHT;
  }
  else if (dx < -COMBAT_INPUT_DEADZONE)
  {
    input |= COMBAT_INPUT_LEFT;
  }

  if (dy > COMBAT_INPUT_DEADZONE)
  {
    input |= COMBAT_INPUT_BLOCK;
  }
  else if (dy < -COMBAT_INPUT_DEADZONE)
  {
    input |= COMBAT_INPUT_UP;
  }

  if ((pressedEdges & COMBAT_BUTTON_ATTACK_MASK) != 0U)
  {
    input |= COMBAT_INPUT_ATTACK;
  }

  if ((pressedEdges & COMBAT_BUTTON_JUMP_MASK) != 0U)
  {
    input |= COMBAT_INPUT_JUMP;
  }

  if ((pressedEdges & COMBAT_BUTTON_SKILL_MASK) != 0U)
  {
    input |= COMBAT_INPUT_SKILL;
  }

  if ((pressedEdges & COMBAT_BUTTON_DASH_MASK) != 0U)
  {
    input |= COMBAT_INPUT_DASH;
  }

  return input;
}

static uint16_t CombatInput_ReadAdc(uint8_t channel)
{
  uint32_t timeout = COMBAT_INPUT_ADC_TIMEOUT;

  ADC1->SQR3 = (uint32_t)channel & ADC_SQR3_SQ1;
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

static uint16_t CombatInput_ReadAxis(uint8_t channel)
{
  uint32_t sum = 0U;

  for (uint8_t i = 0U; i < COMBAT_INPUT_AXIS_SAMPLES; i++)
  {
    sum += CombatInput_ReadAdc(channel);
  }

  return (uint16_t)(sum / COMBAT_INPUT_AXIS_SAMPLES);
}

static void CombatInput_InitButtonGpio(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  (void)RCC->AHB1ENR;

  GPIOB->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1 |
                    GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
  GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1 |
                    GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3);
  GPIOB->PUPDR |= GPIO_PUPDR_PUPD0_0 | GPIO_PUPDR_PUPD1_0 |
                  GPIO_PUPDR_PUPD2_0 | GPIO_PUPDR_PUPD3_0;
}

static uint8_t CombatInput_ReadButtonsRaw(void)
{
  uint8_t buttons = 0U;

  if (CombatInput_IsButtonPressed(COMBAT_INPUT_ATTACK_BUTTON_PORT,
                                  COMBAT_INPUT_ATTACK_BUTTON_PIN) != 0U)
  {
    buttons |= COMBAT_BUTTON_ATTACK_MASK;
  }

  if (CombatInput_IsButtonPressed(COMBAT_INPUT_JUMP_BUTTON_PORT,
                                  COMBAT_INPUT_JUMP_BUTTON_PIN) != 0U)
  {
    buttons |= COMBAT_BUTTON_JUMP_MASK;
  }

  if (CombatInput_IsButtonPressed(COMBAT_INPUT_SKILL_BUTTON_PORT,
                                  COMBAT_INPUT_SKILL_BUTTON_PIN) != 0U)
  {
    buttons |= COMBAT_BUTTON_SKILL_MASK;
  }

  if (CombatInput_IsButtonPressed(COMBAT_INPUT_DASH_BUTTON_PORT,
                                  COMBAT_INPUT_DASH_BUTTON_PIN) != 0U)
  {
    buttons |= COMBAT_BUTTON_DASH_MASK;
  }

  return buttons;
}

static uint8_t CombatInput_IsButtonPressed(GPIO_TypeDef *port, uint32_t pin)
{
  uint8_t high = ((port->IDR & pin) != 0U) ? 1U : 0U;

#if COMBAT_INPUT_BUTTON_ACTIVE_LOW
  return (high == 0U) ? 1U : 0U;
#else
  return high;
#endif
}
