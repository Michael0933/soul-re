#ifndef _MONSTER_H_
#define _MONSTER_H_

#include "common.h"

void MON_DamageEffect(Instance *instance, evFXHitData *data);

void MON_Flee(Instance *instance);
void MON_Idle(Instance *instance);
void MON_IdleEntry(Instance *instance);
void MON_CombatEntry(Instance *instance);
void MON_Embrace(Instance *instance);
void MON_FleeEntry(Instance *instance);
void MON_EmbraceEntry(Instance *instance);
void MON_Combat(Instance *instance);
void MON_Stunned(Instance *instance);
void MON_CleanUp(Instance *instance);
void MON_DefaultInit(Instance *instance);
void MON_StunnedEntry(Instance *instance);
void MON_Dead(Instance *instance);
void MON_DeadEntry(Instance *instance);
void MON_DoCombatTimers(Instance *instance);
void MON_BirthEntry(Instance *instance);
void MON_Birth(Instance *instance);
void MON_PursueEntry(Instance *instance);
void MON_Pursue(Instance *instance);
void MON_MissileHitEntry(Instance *instance);
void MON_MissileHit(Instance *instance);
void MON_Fall(Instance *instance);
void MON_FallEntry(Instance *instance);
void MON_WanderEntry(Instance *instance);
void MON_Wander(Instance *instance);
void MON_AttackEntry(Instance *instance);
void MON_Attack(Instance *instance);
void MON_ImpaleDeathEntry(Instance *instance);
void MON_ImpaleDeath(Instance *instance);
void MON_HitEntry(Instance *instance);
void MON_Hit(Instance *instance);
void MON_GrabbedEntry(Instance *instance);
void MON_Grabbed(Instance *instance);
void MON_ThrownEntry(Instance *instance);
void MON_Thrown(Instance *instance);
void MON_ImpactEntry(Instance *instance);
void MON_Impact(Instance *instance);
void MON_BreakHoldEntry(Instance *instance);
void MON_BreakHold(Instance *instance);
void MON_LandOnFeetEntry(Instance *instance);
void MON_LandOnFeet(Instance *instance);
void MON_GeneralDeathEntry(Instance *instance);
void MON_GeneralDeath(Instance *instance);
void MON_EnvironmentDamageEntry(Instance *instance);
void MON_EnvironmentDamage(Instance *instance);
void MON_LandInWaterEntry(Instance *instance);
void MON_LandInWater(Instance *instance);
void MON_HideEntry(Instance *instance);
void MON_Hide(Instance *instance);
void MON_SurpriseAttackEntry(Instance *instance);
void MON_SurpriseAttack(Instance *instance);
void MON_ParryEntry(Instance *instance);
void MON_Parry(Instance *instance);
void MON_SurprisedEntry(Instance *instance);
void MON_Surprised(Instance *instance);
void MON_NoticeEntry(Instance *instance);
void MON_Notice(Instance *instance);
void MON_PupateEntry(Instance *instance);
void MON_Pupate(Instance *instance);
void MON_ProjectileEntry(Instance *instance);
void MON_Projectile(Instance *instance);
void MON_TerrainImpaleDeathEntry(Instance *instance);
void MON_TerrainImpaleDeath(Instance *instance);
void MON_PetrifiedEntry(Instance *instance);
void MON_Petrified(Instance *instance);

extern char monVersion[];

#endif
