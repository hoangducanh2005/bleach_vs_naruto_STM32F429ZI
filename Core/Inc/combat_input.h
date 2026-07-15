#ifndef __COMBAT_INPUT_H
#define __COMBAT_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "combat_types.h"

void CombatInput_Init(void);
uint8_t CombatInput_Read(void);
uint8_t CombatInput_ReadPlayer2(void);

#ifdef __cplusplus
}
#endif

#endif /* __COMBAT_INPUT_H */
