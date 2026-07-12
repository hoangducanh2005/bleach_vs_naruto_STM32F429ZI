#ifndef __COMBAT_BOX_H
#define __COMBAT_BOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "combat_types.h"

CombatBox CombatBox_ToWorld(CombatBox local,
                            int16_t actorX,
                            int16_t actorY,
                            int8_t facing);
uint8_t CombatBox_Overlap(CombatBox a, CombatBox b);
int16_t CombatBox_ClampI16(int16_t value, int16_t minValue, int16_t maxValue);

#ifdef __cplusplus
}
#endif

#endif /* __COMBAT_BOX_H */
