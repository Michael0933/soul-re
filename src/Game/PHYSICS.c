#include "common.h"
#include "Game/PHYSICS.h"
#include "Game/INSTANCE.h"
#include "Game/COLLIDE.h"
#include "Game/STREAM.h"

void SetNoPtCollideInFamily(Instance *instance)
{
    Instance *child;

    child = instance->LinkChild;

    instance->flags |= 0x40;

    while (child != NULL)
    {
        SetNoPtCollideInFamily(child);

        child = child->LinkSibling;
    }
}

void ResetNoPtCollideInFamily(Instance *instance)
{
    Instance *child;

    child = instance->LinkChild;

    instance->flags &= ~0x40;

    while (child != NULL)
    {
        ResetNoPtCollideInFamily(child);

        child = child->LinkSibling;
    }
}

void PHYSICS_CheckLineInWorld(Instance *instance, PCollideInfo *pcollideInfo)
{
    pcollideInfo->collideType = 63;

    PHYSICS_CheckLineInWorldMask(instance, pcollideInfo);
}

void PHYSICS_CheckLineInWorldMask(Instance *instance, PCollideInfo *pcollideInfo)
{
    Level *level;

    level = STREAM_GetLevelWithID(instance->currentStreamUnitID);

    pcollideInfo->inst = NULL;
    pcollideInfo->instance = instance;

    SetNoPtCollideInFamily(instance);

    if (level != NULL)
    {
        COLLIDE_PointAndWorld(pcollideInfo, level);
    }
    else
    {
        pcollideInfo->type = 0;
    }

    ResetNoPtCollideInFamily(instance);
}

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsCheckLinkedMove);

void PhysicsDefaultLinkedMoveResponse(Instance *instance, evPhysicsLinkedMoveData *Data, int updateTransforms)
{
    instance->position.x += Data->posDelta.x;
    instance->position.y += Data->posDelta.y;
    instance->position.z += Data->posDelta.z;

    if (updateTransforms != 0)
    {
        COLLIDE_UpdateAllTransforms(instance, (SVECTOR *)&Data->posDelta);
    }

    instance->rotation.z += Data->rotDelta.z;
}

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsCheckGravity);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsDefaultGravityResponse);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsCheckEdgeGrabbing);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsDefaultEdgeGrabResponse);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsCheckSliding);

int PhysicsUpdateTface(Instance *instance, int Data)
{
    PCollideInfo CInfo;
    SVECTOR Old;
    SVECTOR New;

    CInfo.oldPoint = &Old;
    CInfo.newPoint = &New;

    Old.vx = New.vx = instance->position.x;
    Old.vy = New.vy = instance->position.y;
    Old.vz = New.vz = instance->position.z;

    Old.vz = New.vz + ((short *)Data)[0];

    New.vz -= ((short *)Data)[1];

    PHYSICS_CheckLineInWorld(instance, &CInfo);

    if (CInfo.type == 3)
    {
        if (instance->tface != CInfo.prim)
        {
            instance->oldTFace = instance->tface;
            instance->tface = (TFace *)CInfo.prim;
            instance->tfaceLevel = CInfo.inst;
            instance->bspTree = CInfo.segment;
        }

        return 1;
    }

    if (instance->tface != NULL)
    {
        instance->oldTFace = instance->tface;
        instance->tface = NULL;
        instance->tfaceLevel = NULL;
        instance->bspTree = 0;
    }

    return 0;
}

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsCheckBlockers);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsCheckSwim);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsDefaultCheckSwimResponse);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsForceSetWater);

int PhysicsCheckLOS(Instance *instance, int Data, int Mode)
{
    PCollideInfo CInfo;

    CInfo.oldPoint = (SVECTOR *)(Data + 8);
    CInfo.newPoint = (SVECTOR *)Data;

    PHYSICS_CheckLineInWorld(instance, &CInfo);

    return CInfo.type == 0;
}

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsCheckDropHeight);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsCheckDropOff);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsFollowWall);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsMoveLocalZClamp);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsMove);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsSetVelFromZRot);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PhysicsSetVelFromRot);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PHYSICS_SetVAndAFromRot);

long PHYSICS_FindAFromDAndT(long d, long t)
{
    if (t != 0)
    {
        t = (d * 8192) / (t * t);

        return t / 4096;
    }

    return 0;
}

long PHYSICS_FindVFromAAndD(long a, long d)
{
    long vsq;

    vsq = a * 2 * d;

    if (vsq != 0)
    {
        return MATH3D_FastSqrt0(vsq);
    }

    return 0;
}

void PHYSICS_StopIfCloseToTarget(Instance *instance, int x, int y, int z)
{
    if ((instance->xAccl < 0) && (instance->xVel <= x) || (instance->xAccl > 0) && (instance->xVel >= x))
    {
        instance->xAccl = 0;
        instance->xVel = x;
    }

    if ((instance->yAccl < 0) && (instance->yVel <= y) || (instance->yAccl > 0) && (instance->yVel >= y))
    {
        instance->yAccl = 0;
        instance->yVel = y;
    }

    if ((instance->zAccl < 0) && (instance->zVel <= z) || (instance->zAccl > 0) && (instance->zVel >= z))
    {
        instance->zAccl = 0;
        instance->zVel = z;
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PHYSICS_CheckForTerrainCollide);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PHYSICS_CheckForObjectCollide);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PHYSICS_CheckForValidMove);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PHYSICS_CheckFaceStick);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PHYSICS_CheckDontGrabEdge);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PHYSICS_GenericLineCheckSetup);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PHYSICS_GenericLineCheck);

INCLUDE_ASM("asm/nonmatchings/Game/PHYSICS", PHYSICS_GenericLineCheckMask);
