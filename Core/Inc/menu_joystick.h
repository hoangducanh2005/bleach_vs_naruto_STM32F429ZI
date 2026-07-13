/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : menu_joystick.h
  * @brief          : Temporary Y-axis-only joystick navigation for UI screens.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MENU_JOYSTICK_H
#define __MENU_JOYSTICK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  MENU_JOYSTICK_EVENT_NONE = 0U,
  MENU_JOYSTICK_EVENT_NEXT,
  MENU_JOYSTICK_EVENT_CONFIRM
} MenuJoystickEvent;

void MenuJoystick_Init(void);
MenuJoystickEvent MenuJoystick_ReadEvent(void);

#ifdef __cplusplus
}
#endif

#endif /* __MENU_JOYSTICK_H */
