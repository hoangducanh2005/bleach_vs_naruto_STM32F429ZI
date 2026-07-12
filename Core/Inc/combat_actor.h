#ifndef __COMBAT_ACTOR_H
#define __COMBAT_ACTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "combat_attack_data.h"

typedef struct
{
  CombatCharacterId character;
  int16_t x;
  int16_t y;
  int16_t groundY;
  int8_t facing;
  int16_t vx;
  int16_t vy;
  uint16_t hp;
  uint8_t onGround;
  CombatAnimState state;
  uint8_t frameIndex;
  uint8_t attackStep;
  uint8_t queuedAttack;
  uint8_t comboNextStep;
  uint8_t hitConnected;
  uint32_t stateStartedMs;
  uint32_t stunUntilMs;
  uint32_t comboExpiresMs;
  uint16_t mana;              // Năng lượng (0 -> 100)
  int16_t stability;          // Điểm kiên định (0 -> 100)
  uint32_t invincibleUntilMs; // Mốc thời gian kết thúc trạng thái bất tử (ms)
  uint32_t knockdownUntilMs;  // Mốc thời gian kết thúc nằm sàn (ms)
  uint32_t lastHitTimeMs;     // Mốc thời gian cuối cùng bị trúng đòn (ms)
} CombatActor;

void CombatActor_Init(CombatActor *actor,
                      CombatCharacterId character,
                      int16_t x,
                      int16_t y,
                      int8_t facing,
                      uint32_t nowMs);
void CombatActor_SetState(CombatActor *actor,
                          CombatAnimState state,
                          uint32_t nowMs);
void CombatActor_Update(CombatActor *actor,
                        uint8_t inputFlags,
                        uint32_t nowMs,
                        uint8_t allowInput);
void CombatActor_FaceToward(CombatActor *actor, const CombatActor *target);
CombatBox CombatActor_GetHurtboxWorld(const CombatActor *actor);
CombatBox CombatActor_GetHitboxWorld(const CombatActor *actor,
                                     const CombatHitboxDef **hitboxOut);
uint8_t CombatActor_IsBlockingFront(const CombatActor *target,
                                    const CombatActor *attacker);
uint8_t CombatActor_GetFrame(const CombatActor *actor, CombatFrameView *outFrame);
uint8_t CombatActor_IsActionLocked(const CombatActor *actor, uint32_t nowMs);

#ifdef __cplusplus
}
#endif

#endif /* __COMBAT_ACTOR_H */
