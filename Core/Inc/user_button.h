/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : user_button.h
  * @brief          : Debounced USER button reader for menu/OK navigation.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __USER_BUTTON_H
#define __USER_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void UserButton_Init(void);
uint8_t UserButton_IsPressed(void);
uint8_t UserButton_WasPressed(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_BUTTON_H */
