#ifndef __BATTLE_DEMO_H
#define __BATTLE_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "combat_types.h"

void BattleDemo_SetCharacters(CombatCharacterId playerCharacter,
                              CombatCharacterId cpuCharacter);
void BattleDemo_Init(void);
uint8_t BattleDemo_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* __BATTLE_DEMO_H */
