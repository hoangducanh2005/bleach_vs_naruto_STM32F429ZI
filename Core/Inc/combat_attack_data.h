#ifndef __COMBAT_ATTACK_DATA_H
#define __COMBAT_ATTACK_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "combat_types.h"

typedef struct
{
  CombatBox idle;
  CombatBox run;
  CombatBox block;
  CombatBox jump;
  CombatBox attack;
  CombatBox skill;
  CombatBox hit;
} CombatHurtboxProfile;

const CombatHurtboxProfile *CombatAttackData_GetHurtboxProfile(
    CombatCharacterId character);
CombatBox CombatAttackData_GetHurtbox(CombatCharacterId character,
                                      CombatAnimState state);
const CombatHitboxDef *CombatAttackData_GetHitbox(CombatCharacterId character,
                                                  CombatAnimState state,
                                                  uint8_t frameIndex);
CombatBox CombatAttackData_GetGuardBox(void);

#ifdef __cplusplus
}
#endif

#endif /* __COMBAT_ATTACK_DATA_H */
