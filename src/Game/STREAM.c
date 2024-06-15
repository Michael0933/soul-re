#include "common.h"
#include "Game/STREAM.h"
#include "Game/STRMLOAD.h"
#include "Game/GAMELOOP.h"
#include "Game/MEMPACK.h"
#include "Game/INSTANCE.h"
#include "Game/OBTABLE.h"
#include "Game/PSX/AADLIB.h"
#include "Game/TIMER.h"
#include "Game/LIGHT3D.h"
#include "Game/SAVEINFO.h"
#include "Game/EVENT.h"
#include "Game/PLAN/PLANAPI.h"
#include "Game/VRAM.h"
#include "Game/FX.h"
#include "Game/RELMOD.h"
#include "Game/SPLINE.h"
#include "Game/CAMERA.h"
#include "Game/GLYPH.h"
#include "Game/LOAD3D.h"
#include "Game/DEBUG.h"

long CurrentWarpNumber;

short M_TrackClutUpdate;

WarpRoom WarpRoomArray[14];

WarpGateLoadInformation WarpGateLoadInfo;

extern char D_800D1954[];

extern char D_800D17B0[];

extern char D_800D17BC[];

extern char D_800D17D8[];

extern char D_800D17F4[];

extern char D_800D1810[];

extern char D_800D18E4[];

extern char D_800D1910[];

extern char D_800D1928[];

extern char D_800D1940[];

extern char D_800D194C[];

void STREAM_FillOutFileNames(char *baseAreaName, char *dramName, char *vramName, char *sfxName)
{
    char text[16];
    char *number;

    strcpy(text, baseAreaName);

    number = strpbrk(text, D_800D17B0);

    if (number != 0)
    {
        *number = 0;
    }

    if (dramName != NULL)
    {
        sprintf(dramName, D_800D17BC, text, baseAreaName);
    }

    if (vramName != NULL)
    {
        sprintf(vramName, D_800D17D8, text, baseAreaName);
    }

    if (sfxName != NULL)
    {
        sprintf(sfxName, D_800D17F4, text, baseAreaName);
    }
}

void STREAM_AbortAreaLoad(char *baseAreaName)
{
    char vramName[80];

    STREAM_FillOutFileNames(baseAreaName, NULL, vramName, NULL);

    LOAD_AbortDirectoryChange(baseAreaName);

    LOAD_AbortFileLoad(vramName, (void *)VRAM_LoadReturn);
}

void STREAM_Init()
{
    int i;

    for (i = 0; i < 16; i++)
    {
        StreamTracker.StreamList[i].used = 0;

        StreamTracker.StreamList[i].flags = 0;

        StreamTracker.StreamList[i].StreamUnitID = 0;
    }
}

int FindObjectName(char *name)
{
    int i;
    ObjectTracker *otr;

    otr = gameTrackerX.GlobalObjects;

    for (i = 0; i < 48; i++, otr++)
    {
        if ((otr->objectStatus != 0) && (strcmpi(otr->name, name) == 0))
        {
            return i;
        }
    }

    return -1;
}

ObjectTracker *FindObjectInTracker(Object *object)
{
    int i;
    ObjectTracker *otr;

    otr = gameTrackerX.GlobalObjects;

    for (i = 0; i < 48; i++, otr++)
    {
        if ((otr->objectStatus != 0) && (otr->object == object))
        {
            return otr;
        }
    }

    return NULL;
}

StreamUnit *FindStreamUnitFromLevel(Level *level)
{
    StreamUnit *ret;
    long i;

    ret = NULL;

    for (i = 0; i < 16; i++)
    {
        if ((StreamTracker.StreamList[i].used == 2) && (StreamTracker.StreamList[i].level == level))
        {
            ret = &StreamTracker.StreamList[i];
            break;
        }
    }

    return ret;
}

void STREAM_LoadObjectReturn(void *loadData, void *data, void *data2)
{
    Object *object;
    ObjectTracker *objectTracker;

    (void)data2;

    GetRCnt(0xF2000000);

    object = (Object *)loadData;

    objectTracker = (ObjectTracker *)data;

    gameTimer;

    if (((object->oflags & 0x8000000)) && (object->relocList != NULL) && (object->relocModule != NULL))
    {
        RELMOD_InitModulePointers((int)object->relocModule, (int *)object->relocList);
    }

    STREAM_PackVRAMObject(objectTracker);

    OBTABLE_InitAnimPointers(objectTracker);
    OBTABLE_InitObjectWithID(object);

    if ((object->oflags2 & 0x800000))
    {
        char objDsfxFileName[64];

        sprintf(objDsfxFileName, D_800D1810, objectTracker->name, objectTracker->name);

        object->sfxFileHandle = 0;

        if (LOAD_DoesFileExist(objDsfxFileName) != 0)
        {
            object->sfxFileHandle = aadLoadDynamicSfx(objectTracker->name, 0, 0);
        }
    }

    if (objectTracker->vramBlock == NULL)
    {
        objectTracker->objectStatus = 2;
    }
    else
    {
        objectTracker->objectStatus = 4;
    }
}

void STREAM_DumpMonster(ObjectTracker *dumpee)
{
    Object *object;
    Instance *instance;

    object = dumpee->object;

    if (dumpee->vramBlock != NULL)
    {
        VRAM_ClearVramBlock((BlockVramEntry *)dumpee->vramBlock);
    }

    OBTABLE_RemoveObjectEntry(object);

    if (((object->oflags2 & 0x800000)) && (object->sfxFileHandle != 0))
    {
        aadFreeDynamicSfx(object->sfxFileHandle);
    }

    instance = gameTrackerX.instanceList->first;

    if (instance != NULL)
    {
        Instance *next;

        while (instance != NULL)
        {
            next = instance->next;

            if (object == instance->object)
            {
                INSTANCE_ReallyRemoveInstance(gameTrackerX.instanceList, instance, 0);
            }

            instance = next;
        }
    }

    MEMPACK_Free((char *)object);

    dumpee->object = NULL;
}

int STREAM_InList(char *name, char **nameList)
{
    char **mon;

    for (mon = nameList; *mon != NULL; mon++)
    {
        if (strcmpi(name, *mon) == 0)
        {
            return 1;
        }
    }

    return 0;
}

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_IsSpecialMonster);

void STREAM_DumpSomeMonsters()
{
    ObjectTracker *otr;
    int i;

    otr = gameTrackerX.GlobalObjects;

    for (i = 0; i < 48; i++, otr++)
    {
        if (((otr->objectStatus == 2) && (otr->object != NULL)) && (STREAM_IsSpecialMonster((char *)otr) != 0))
        {
            STREAM_DumpMonster(otr);
        }
    }
}

void STREAM_NoMonsters()
{
    gameTrackerX.gameFlags |= 0x4000000;

    STREAM_DumpSomeMonsters();
}

void STREAM_YesMonsters()
{
    gameTrackerX.gameFlags &= ~0x4000000;
}

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_IsMonster);

int STREAM_TryAndDumpANonResidentObject()
{
    ObjectTracker *otr;
    int i;

    otr = gameTrackerX.GlobalObjects;

    for (i = 0; i < 48; i++, otr++)
    {
        if (STREAM_TryAndDumpNonResident(otr) != 0)
        {
            return i;
        }
    }

    return -1;
}

int InsertGlobalObject(char *name, GameTracker *gameTracker)
{
    char string[64];
    char vramname[64];
    int i;
    ObjectTracker *otr;

    i = -1;

    if (((!(gameTrackerX.gameFlags & 0x4000000)) || (STREAM_IsSpecialMonster(name) == 0)) && ((!(gameTracker->debugFlags2 & 0x8000)) || (STREAM_IsMonster(name) == 0)))
    {
        otr = gameTracker->GlobalObjects;

        for (i = 0; i < 48; i++, otr++)
        {
            if ((otr->objectStatus != 0) && (strcmpi(otr->name, name) == 0))
            {
                break;
            }
        }

        if (i == 48)
        {
            otr = gameTracker->GlobalObjects;

            for (i = 0; i < 48; i++, otr++)
            {
                if (otr->objectStatus == 0)
                {
                    break;
                }
            }

            if (i == 48)
            {
                i = STREAM_TryAndDumpANonResidentObject();

                if (i == -1)
                {
                    DEBUG_FatalError(D_800D18E4, 48);
                }
            }

            sprintf(string, D_800D1910, name, name);
            sprintf(vramname, D_800D1928, name, name);

            strcpy(otr->name, name);

            otr->objectStatus = 1;

            LOAD_NonBlockingBinaryLoad(string, (void *)STREAM_LoadObjectReturn, (void *)otr, NULL, (void **)&otr->object, 1);

            otr->numInUse = 0;
            otr->numObjectsUsing = 0;
        }
    }

    return i;
}

ObjectTracker *STREAM_GetObjectTracker(char *name)
{
    int i;

    i = InsertGlobalObject(name, &gameTrackerX);

    if (i == -1)
    {
        return NULL;
    }
    else
    {
        return &gameTrackerX.GlobalObjects[i];
    }
}

void LoadLevelObjects(StreamUnit *stream)
{
    int objlist_pos;
    char name[20];
    Level *level;
    int i;

    STREAM_NextLoadAsNormal();

    objlist_pos = 0;

    while (((unsigned char *)stream->level->objectNameList)[objlist_pos] != 255)
    {
        strcpy(name, (char *)stream->level->objectNameList + objlist_pos);

        InsertGlobalObject(name, &gameTrackerX);

        objlist_pos += 16;
    }

    level = stream->level;

    for (i = 0; i < level->numIntros; i++)
    {
        if (FindObjectName(level->introList[i].name) != -1)
        {
            level->introList[i].flags &= ~0x4000;
        }
        else
        {
            level->introList[i].flags |= 0x4000;
        }
    }
}

long STREAM_IsAnInstanceUsingObject(Object *object)
{
    Instance *instance;
    Instance *next;
    long ret;

    instance = gameTrackerX.instanceList->first;

    ret = 0;

    while (instance != NULL)
    {
        next = instance->next;

        if (instance->object == object)
        {
            ret = 1;
            break;
        }

        instance = next;
    }

    return ret;
}

void STREAM_StreamLoadObjectAbort(void *loadData, void *data, void *data2)
{
    ObjectTracker *objectTracker;

    (void)data2;

    objectTracker = (ObjectTracker *)data;

    if (loadData != NULL)
    {
        MEMPACK_Free((char *)loadData);
    }

    objectTracker->objectStatus = 0;
}

void STREAM_DumpLoadingObjects()
{
    int i;
    ObjectTracker *tracker;

    tracker = gameTrackerX.GlobalObjects;

    for (i = 0; i < 48; i++, tracker++)
    {
        if (tracker->objectStatus == 1)
        {
            STREAM_DumpObject(tracker);
        }
    }
}

void STREAM_DumpObject(ObjectTracker *objectTracker)
{
    Object *object;
    char dramName[64];

    object = objectTracker->object;

    if (objectTracker->objectStatus == 1)
    {
        sprintf(dramName, D_800D1910, objectTracker->name, objectTracker->name);

        LOAD_AbortFileLoad(dramName, (void *)STREAM_StreamLoadObjectAbort);
    }
    else if (object != NULL)
    {
        if (!(object->oflags & 0x2000000))
        {
            if (objectTracker->vramBlock != NULL)
            {
                VRAM_ClearVramBlock((BlockVramEntry *)objectTracker->vramBlock);
            }

            if (((object->oflags2 & 0x800000)) && (object->sfxFileHandle != 0))
            {
                aadFreeDynamicSfx(object->sfxFileHandle);
            }

            OBTABLE_RemoveObjectEntry(object);

            MEMPACK_Free((char *)object);

            objectTracker->objectStatus = 0;
        }

        if (object == NULL)
        {
            objectTracker->objectStatus = 0;
        }
    }
    else
    {
        objectTracker->objectStatus = 0;
    }
}

int STREAM_IsObjectInAnyUnit(ObjectTracker *tracker)
{
    int d;
    unsigned char *objlist;

    for (d = 0; d < 16; d++)
    {
        if ((StreamTracker.StreamList[d].used == 2) && (StreamTracker.StreamList[d].level != NULL))
        {
            for (objlist = (unsigned char *)StreamTracker.StreamList[d].level->objectNameList; *objlist != 255; objlist += 16)
            {
                if (strcmpi(&tracker->name, (char *)objlist) == 0)
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

void STREAM_RemoveAllObjectsNotInUse()
{
    int i;
    int abort;
    ObjectTracker *tracker;
    ObjectTracker *trackerList;

    trackerList = gameTrackerX.GlobalObjects;

    for (tracker = trackerList, i = 0; i < 48; i++, tracker++)
    {
        Object *object;

        if (tracker->objectStatus == 2)
        {
            object = tracker->object;

            if ((!(object->oflags & 0x2000000)) && (STREAM_IsObjectInAnyUnit(tracker) == 0)
            && (STREAM_IsAnInstanceUsingObject(object) == 0))
            {
                tracker->objectStatus = 3;
            }
        }
    }

    do
    {
        abort = 1;

        for (tracker = trackerList, i = 0; i < 48; i++, tracker++)
        {
            if (tracker->objectStatus == 3)
            {
                int j;

                for (j = 0; j < (char)tracker->numObjectsUsing; j++)
                {
                    if (trackerList[(int)tracker->objectsUsing[j]].objectStatus != 3)
                    {
                        tracker->objectStatus = 2;

                        abort = 0;
                        break;
                    }
                }
            }
        }
    } while (abort == 0);

    for (tracker = trackerList, i = 0; i < 48; i++, tracker++)
    {
        if (tracker->objectStatus == 3)
        {
            int j;
            ObjectTracker *otr;

            for (otr = trackerList, j = 0; j < 48; j++, otr++)
            {
                if ((otr->objectStatus == 1) || (otr->objectStatus == 2) || (otr->objectStatus == 4))
                {
                    int k;

                    for (k = 0; k < (char)otr->numObjectsUsing; k++)
                    {
                        if ((char)otr->objectsUsing[k] == i)
                        {
                            int l;

                            otr->numObjectsUsing--;

                            for (l = k; l < (char)otr->numObjectsUsing; l++)
                            {
                                otr->objectsUsing[l] = otr->objectsUsing[l + 1];
                            }

                            break;
                        }
                    }
                }
            }

            STREAM_DumpObject(tracker);
        }
    }

    for (tracker = trackerList, i = 0; i < 48; i++, tracker++)
    {
        if ((tracker->objectStatus == 1) && (STREAM_IsObjectInAnyUnit(tracker) == 0) && ((char)tracker->numObjectsUsing == 0))
        {
            STREAM_DumpObject(tracker);
        }
    }
}

void RemoveAllObjects(GameTracker *gameTracker)
{
    int i;
    ObjectTracker *tracker;

    for (i = 0, tracker = &gameTracker->GlobalObjects[i]; i < 48; i++, tracker++)
    {
        if (tracker->objectStatus != 0)
        {
            STREAM_DumpObject(tracker);
        }
    }
}

Level *STREAM_GetLevelWithID(long id)
{
    Level *retLevel;
    long i;

    retLevel = NULL;

    for (i = 0; i < 16; i++)
    {
        if ((StreamTracker.StreamList[i].used == 2) && (StreamTracker.StreamList[i].StreamUnitID == id))
        {
            retLevel = StreamTracker.StreamList[i].level;
            break;
        }
    }

    return retLevel;
}

StreamUnit *STREAM_GetStreamUnitWithID(long id)
{
    StreamUnit *retUnit;
    long i;

    retUnit = NULL;

    for (i = 0; i < 16; i++)
    {
        if ((StreamTracker.StreamList[i].used == 2) && (StreamTracker.StreamList[i].StreamUnitID == id))
        {
            retUnit = &StreamTracker.StreamList[i];
            break;
        }
    }

    return retUnit;
}

void STREAM_CalculateWaterLevel(Level *level)
{
    Terrain *terrain;
    int i;
    TFace *tface;
    long waterZLevel;

    waterZLevel = -32767;

    if (level->waterZLevel == 0)
    {
        if ((level->unitFlags & 0x1))
        {
            level->waterZLevel = 32767;
        }
        else
        {
            terrain = level->terrain;

            tface = (TFace *)terrain->faceList;

            for (i = terrain->numFaces; i > 0; i--, tface++)
            {
                if (((tface->attr & 0x8)) && (terrain->vertexList[tface->face.v0].vertex.z == terrain->vertexList[tface->face.v1].vertex.z)
                && (terrain->vertexList[tface->face.v0].vertex.z == terrain->vertexList[tface->face.v2].vertex.z))
                {
                    if (waterZLevel == -32767)
                    {
                        waterZLevel = terrain->vertexList[tface->face.v0].vertex.z;
                    }
                    else if (waterZLevel != terrain->vertexList[tface->face.v0].vertex.z)
                    {
                        break;
                    }
                }
            }

            if (waterZLevel == -32767)
            {
                level->waterZLevel = -32767;
            }
            else
            {
                level->waterZLevel = waterZLevel + level->terrain->BSPTreeArray[0].globalOffset.z;
            }
        }
    }
}

int STREAM_IsMorphInProgress()
{
    return gameTrackerX.gameData.asmData.MorphTime != 1000;
}

long STREAM_GetWaterZLevel(Level *level, Instance *instance)
{
    int waterZLevel;

    if ((instance->flags2 & 0x8000000))
    {
        if ((!(instance->object->oflags2 & 0x2000000)) && (gameTrackerX.gameData.asmData.MorphTime != 1000))
        {
            waterZLevel = level->waterZLevel;
        }
        else
        {
            waterZLevel = -32767;
        }
    }
    else if ((!(instance->object->oflags2 & 0x2000000)) && (gameTrackerX.gameData.asmData.MorphTime != 1000))
    {
        waterZLevel = -32767;
    }
    else
    {
        waterZLevel = level->waterZLevel;
    }

    return waterZLevel;
}

void STREAM_SetMainFog(StreamUnit *streamUnit)
{
    Level *level;

    level = streamUnit->level;

    if (gameTrackerX.gameData.asmData.MorphType != 0)
    {
        streamUnit->UnitFogNear = level->spectralFogNear;
        streamUnit->UnitFogFar = level->spectralFogFar;
    }
    else
    {
        streamUnit->UnitFogNear = level->holdFogNear;
        streamUnit->UnitFogFar = level->holdFogFar;
    }

    streamUnit->TargetFogNear = streamUnit->UnitFogNear;
    streamUnit->TargetFogFar = streamUnit->UnitFogFar;
}

void STREAM_SetStreamFog(StreamUnit *streamUnit, short fogNear, short fogFar)
{
    short unitFogFar;
    int unitFogHold;

    unitFogFar = FindStreamUnitFromLevel(gameTrackerX.level)->UnitFogFar;

    if (fogFar < unitFogFar)
    {
        unitFogFar = fogFar;
    }

    unitFogHold = unitFogFar - 2000;

    streamUnit->TargetFogFar = unitFogFar;
    streamUnit->UnitFogFar = unitFogFar;

    if (fogNear < unitFogHold)
    {
        unitFogHold = fogNear;
    }

    streamUnit->TargetFogNear = unitFogHold;
    streamUnit->UnitFogNear = unitFogHold;
}

void STREAM_ConnectStream(StreamUnit *streamUnit)
{
    StreamUnit *mainUnit;
    StreamUnitPortal *streamPortal;
    int numportals;
    StreamUnitPortal *streamPortal2;
    int numportals2;
    int i;
    int j;
    char text[16];
    char *commapos;
    int signalID;

    WARPGATE_UpdateAddToArray(streamUnit);

    if (gameTrackerX.StreamUnitID != streamUnit->StreamUnitID)
    {
        mainUnit = STREAM_GetStreamUnitWithID(gameTrackerX.StreamUnitID);

        numportals2 = *(long *)streamUnit->level->terrain->StreamUnits;

        streamPortal2 = (StreamUnitPortal *)((long *)streamUnit->level->terrain->StreamUnits + 1);

        for (j = 0; j < numportals2; j++, streamPortal2++)
        {
            StreamUnit *connectStream;

            strcpy(text, streamPortal2->tolevelname);

            commapos = strchr(text, ',');

            if (commapos != NULL)
            {
                *commapos = 0;

                signalID = atoi(commapos + 1);
            }
            else
            {
                signalID = 0;
            }

            connectStream = STREAM_GetStreamUnitWithID(streamPortal2->streamID);

            if ((strcmpi(text, D_800D1940) == 0) && (WARPGATE_IsUnitWarpRoom(mainUnit) != 0))
            {
                connectStream = mainUnit;
            }

            streamPortal2->toStreamUnit = connectStream;

            if ((connectStream == NULL) || (connectStream != mainUnit))
            {
                continue;
            }

            numportals = *(long *)mainUnit->level->terrain->StreamUnits;

            streamPortal = (StreamUnitPortal *)((long *)mainUnit->level->terrain->StreamUnits + 1);

            for (i = 0; i < numportals; i++, streamPortal++)
            {
                if (signalID == streamPortal->MSignalID)
                {
                    SVector offset;

                    offset.x = streamPortal->minx - streamPortal2->minx;
                    offset.y = streamPortal->miny - streamPortal2->miny;
                    offset.z = streamPortal->minz - streamPortal2->minz;

                    RelocateLevel(streamUnit->level, &offset);
                    break;
                }
            }
        }

        {
            long d;
            StreamUnit *connectStream;

            connectStream = StreamTracker.StreamList;

            for (d = 0; d < 16; d++, connectStream++)
            {
                if ((connectStream->used == 2) && (connectStream != streamUnit))
                {
                    numportals2 = *(long *)connectStream->level->terrain->StreamUnits;

                    streamPortal2 = (StreamUnitPortal *)((long *)connectStream->level->terrain->StreamUnits + 1);

                    for (j = 0; j < numportals2; j++, streamPortal2++)
                    {
                        long hookedUp;

                        hookedUp = 0;

                        strcpy(text, streamPortal2->tolevelname);

                        commapos = strchr(text, ',');

                        if (commapos != NULL)
                        {
                            *commapos = 0;

                            signalID = atoi(commapos + 1);
                        }
                        else
                        {
                            signalID = 0;
                        }

                        if (streamPortal2->streamID == streamUnit->StreamUnitID)
                        {
                            streamPortal2->toStreamUnit = streamUnit;

                            hookedUp = 1;
                        }
                        else if ((strcmpi(text, D_800D1940) == 0) && (WARPGATE_IsUnitWarpRoom(streamUnit) != 0))
                        {
                            streamPortal2->toStreamUnit = streamUnit;

                            hookedUp = 1;
                        }

                        if ((hookedUp == 1) && (connectStream == mainUnit))
                        {
                            numportals = *(long *)streamUnit->level->terrain->StreamUnits;

                            streamPortal = (StreamUnitPortal *)((long *)streamUnit->level->terrain->StreamUnits + 1);

                            for (i = 0; i < numportals; i++, streamPortal++)
                            {
                                if (signalID == streamPortal->MSignalID)
                                {
                                    SVector offset;

                                    offset.x = streamPortal2->minx - streamPortal->minx;
                                    offset.y = streamPortal2->miny - streamPortal->miny;
                                    offset.z = streamPortal2->minz - streamPortal->minz;

                                    RelocateLevel(streamUnit->level, &offset);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        for (i = 0; i < streamUnit->level->numIntros; i++)
        {
            if (strcmpi(streamUnit->level->introList[i].name, D_800D194C) == 0)
            {
                streamUnit->level->introList[i].flags |= 0x8;
                break;
            }
        }
    }
}

void STREAM_StreamLoadLevelAbort(void *loadData, void *data, void *data2)
{
    StreamUnit *streamUnit;

    (void)data;

    streamUnit = (StreamUnit *)data2;

    if (loadData != NULL)
    {
        MEMPACK_Free((char *)loadData);
    }

    streamUnit->level = NULL;

    streamUnit->used = 0;

    streamUnit->flags = 0;
}

void STREAM_DoObjectLoadAndDump(StreamUnit *streamUnit)
{
    int i;

    (void)streamUnit;

    for (i = 0; i < 16; i++)
    {
        if (StreamTracker.StreamList[i].used == 1)
        {
            return;
        }
    }

    STREAM_RemoveAllObjectsNotInUse();
}

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_FinishLoad);

void STREAM_LoadLevelReturn(void *loadData, void *data, void *data2)
{
    (void)data;

    GetRCnt(0xF2000000);

    gameTimer;

    ((StreamUnit *)data2)->StreamUnitID = ((Level *)loadData)->streamUnitID;

    gameTrackerX.StreamUnitID = ((Level *)loadData)->streamUnitID;

    gameTrackerX.level = (Level *)loadData;

    STREAM_SetMainFog(((StreamUnit *)data2));

    STREAM_FinishLoad(((StreamUnit *)data2));
}

void STREAM_StreamLoadLevelReturn(void *loadData, void *data, void *data2)
{
    Level *level;
    StreamUnit *streamUnit;

    (void)data;

    GetRCnt(0xF2000000);

    gameTimer;

    level = (Level *)loadData;

    streamUnit = (StreamUnit *)data2;

    streamUnit->StreamUnitID = level->streamUnitID;

    if (streamUnit->used == 3)
    {
        streamUnit->used = 0;

        streamUnit->flags = 0;

        MEMPACK_Free((char *)streamUnit->level);

        streamUnit->level = NULL;

        return;
    }

    if (gameTrackerX.gameData.asmData.MorphType != 0)
    {
        STREAM_SetStreamFog(streamUnit, level->spectralFogNear, level->spectralFogFar);
    }
    else
    {
        STREAM_SetStreamFog(streamUnit, level->holdFogNear, level->holdFogFar);
    }

    STREAM_FinishLoad(streamUnit);

    if ((gameTrackerX.playerInstance != NULL) && (level->streamUnitID == gameTrackerX.playerInstance->currentStreamUnitID))
    {
        strcpy(gameTrackerX.baseAreaName, level->worldName);

        STREAM_SetMainFog(streamUnit);

        gameTrackerX.StreamUnitID = level->streamUnitID;

        gameTrackerX.level = level;
    }
}

void STREAM_UpdateLevelPointer(Level *oldLevel, Level *newLevel, long sizeOfLevel)
{
    long i;
    long offset;
    GameTracker *gameTracker;

    offset = (int)newLevel - (int)oldLevel;

    for (i = 0; i < 16; i++)
    {
        if ((StreamTracker.StreamList[i].used == 2) && (StreamTracker.StreamList[i].level == oldLevel))
        {
            StreamTracker.StreamList[i].level = newLevel;
            break;
        }
    }

    gameTracker = &gameTrackerX;

    if (gameTrackerX.level == oldLevel)
    {
        gameTrackerX.level = newLevel;
    }

    {
        Instance *instance;

        instance = gameTracker->instanceList->first;

        while (instance != NULL)
        {
            i = (int)oldLevel + sizeOfLevel;

            if (IN_BOUNDS(instance->intro, oldLevel, i) != 0)
            {
                instance->intro = (Intro *)OFFSET_DATA(instance->intro, offset);
            }

            if (IN_BOUNDS(instance->introData, oldLevel, i) != 0)
            {
                instance->introData = (void *)OFFSET_DATA(instance->introData, offset);
            }

            if (IN_BOUNDS(instance->tface, oldLevel, i) != 0)
            {
                instance->tface = (TFace *)OFFSET_DATA(instance->tface, offset);
            }

            if (IN_BOUNDS(instance->waterFace, oldLevel, i) != 0)
            {
                instance->waterFace = (TFace *)OFFSET_DATA(instance->waterFace, offset);
            }

            if (IN_BOUNDS(instance->waterFaceTerrain, oldLevel, i) != 0)
            {
                instance->waterFaceTerrain = (Terrain *)OFFSET_DATA(instance->waterFaceTerrain, offset);
            }

            if (IN_BOUNDS(instance->oldTFace, oldLevel, i) != 0)
            {
                instance->oldTFace = (TFace *)OFFSET_DATA(instance->oldTFace, offset);
            }

            if (IN_BOUNDS(instance->tfaceLevel, oldLevel, i) != 0)
            {
                instance->tfaceLevel = (void *)OFFSET_DATA(instance->tfaceLevel, offset);
            }

            if (IN_BOUNDS(instance->cachedTFaceLevel, oldLevel, i) != 0)
            {
                instance->cachedTFaceLevel = (void *)OFFSET_DATA(instance->cachedTFaceLevel, offset);
            }

            instance = instance->next;
        }
    }

    if (IN_BOUNDS(theCamera.data.Cinematic.posSpline, oldLevel, (int)oldLevel + sizeOfLevel) != 0)
    {
        theCamera.data.Cinematic.posSpline = (MultiSpline *)OFFSET_DATA(theCamera.data.Cinematic.posSpline, offset);
    }

    if (IN_BOUNDS(theCamera.data.Cinematic.targetSpline, oldLevel, (int)oldLevel + sizeOfLevel) != 0)
    {
        theCamera.data.Cinematic.targetSpline = (MultiSpline *)OFFSET_DATA(theCamera.data.Cinematic.targetSpline, offset);
    }

    for (i = 0; i <= theCamera.stack; i++)
    {
        if (IN_BOUNDS(theCamera.savedCinematic[i].posSpline, oldLevel, (int)oldLevel + sizeOfLevel) != 0)
        {
            theCamera.savedCinematic[i].posSpline = (MultiSpline *)OFFSET_DATA(theCamera.savedCinematic[i].posSpline, offset);
        }

        if (IN_BOUNDS(theCamera.savedCinematic[i].targetSpline, oldLevel, (int)oldLevel + sizeOfLevel) != 0)
        {
            theCamera.savedCinematic[i].targetSpline = (MultiSpline *)OFFSET_DATA(theCamera.savedCinematic[i].targetSpline, offset);
        }
    }

    EVENT_UpdateResetSignalArrayAndWaterMovement(oldLevel, newLevel, sizeOfLevel);
}

StreamUnit *STREAM_WhichUnitPointerIsIn(void *pointer)
{
    int i;
    int size;
    Level *level;

    for (i = 0; i < 16; i++)
    {
        if (StreamTracker.StreamList[i].used == 2)
        {
            level = StreamTracker.StreamList[i].level;

            if (level != NULL)
            {
                size = MEMPACK_Size((char *)level);

                if ((level <= (Level *)pointer) && (((char *)level + size) >= (char *)pointer))
                {
                    return &StreamTracker.StreamList[i];
                }
            }
        }
    }

    return NULL;
}

void STREAM_UpdateObjectPointer(Object *oldObject, Object *newObject, long sizeOfObject)
{
    long i;
    long d;
    GameTracker *gameTracker;
    long offset;
    ObjectTracker *otr;

    gameTracker = &gameTrackerX;

    offset = (int)newObject - (int)oldObject;

    otr = FindObjectInTracker(oldObject);

    if (otr != NULL)
    {
        otr->object = newObject;

        for (i = 0; i < otr->numObjectsUsing; i++)
        {
            int j;
            Object *object;

            object = gameTracker->GlobalObjects[(int)otr->objectsUsing[i]].object;

            if (object != NULL)
            {
                for (j = 0; j < object->numAnims; j++)
                {
                    if (IN_BOUNDS(object->animList[j], oldObject, (int)oldObject + sizeOfObject) != 0)
                    {
                        object->animList[j] = (G2AnimKeylist *)OFFSET_DATA(object->animList[j], offset);
                    }
                }
            }
        }

        OBTABLE_ChangeObjectAccessPointers(oldObject, newObject);

        if (((newObject->oflags & 0x8000000)) && (newObject->relocList != NULL) && (newObject->relocModule != NULL))
        {
            RELMOD_RelocModulePointers((int)newObject->relocModule, offset, (int *)newObject->relocList);
        }

        {
            Instance *instance;

            instance = gameTracker->instanceList->first;

            while (instance != NULL)
            {
                if (instance->object == oldObject)
                {
                    instance->object = newObject;

                    if (instance->hModelList != NULL)
                    {
                        for (i = 0; i < instance->object->numModels; i++)
                        {
                            for (d = 0; d < instance->hModelList[i].numHPrims; d++)
                            {
                                instance->hModelList[i].hPrimList[d].data.hsphere = (HSphere *)OFFSET_DATA(instance->hModelList[i].hPrimList[d].data.hsphere, offset);
                            }
                        }
                    }

                    OBTABLE_RelocateInstanceObject(instance, offset);
                }

                if (IN_BOUNDS(instance->data, oldObject, (int)oldObject + sizeOfObject) != 0)
                {
                    instance->data = (void *)OFFSET_DATA(instance->data, offset);
                }

                instance = instance->next;
            }
        }
    }

    OBTABLE_RelocateObjectTune(newObject, offset);

    if ((newObject->oflags2 & 0x20000000))
    {
        FX_RelocateFXPointers(oldObject, newObject, sizeOfObject);
    }
}

void STREAM_UpdateInstanceCollisionInfo(HModel *oldHModel, HModel *newHModel)
{
    Instance *instance;

    instance = gameTrackerX.instanceList->first;

    while (instance != NULL)
    {
        if (instance->hModelList == oldHModel)
        {
            instance->hModelList = newHModel;
        }

        instance = instance->next;
    }
}

void STREAM_LoadMainVram(GameTracker *gameTracker, char *baseAreaName, StreamUnit *streamUnit)
{
    char dramName[80];
    char vramName[80];
    VramBuffer *vramBuffer;
    Level *level;

    (void)baseAreaName;

    level = streamUnit->level;

    STREAM_FillOutFileNames(gameTracker->baseAreaName, dramName, vramName, NULL);

    vramBuffer = (VramBuffer *)MEMPACK_Malloc((level->vramSize.w << 1) + 20, 35);

    vramBuffer->lineOverFlow = (short *)(vramBuffer + 1);

    vramBuffer->flags = 0;

    vramBuffer->x = level->vramSize.x + 512;
    vramBuffer->y = level->vramSize.y;
    vramBuffer->w = level->vramSize.w;
    vramBuffer->h = level->vramSize.h;

    M_TrackClutUpdate = 0;

    vramBuffer->yOffset = 0;

    vramBuffer->lengthOfLeftOverData = 0;

    LOAD_NonBlockingBufferedLoad(vramName, (void *)VRAM_TransferBufferToVram, vramBuffer, NULL);
}

void STREAM_MoveIntoNewStreamUnit()
{
    gameTrackerX.playerInstance->cachedTFace = -1;
    gameTrackerX.playerInstance->cachedTFaceLevel = NULL;

    gameTrackerX.playerInstance->currentStreamUnitID = gameTrackerX.moveRazielToStreamID;

    INSTANCE_UpdateFamilyStreamUnitID(gameTrackerX.playerInstance);

    GAMELOOP_StreamLevelLoadAndInit(gameTrackerX.S_baseAreaName, &gameTrackerX, gameTrackerX.toSignal, gameTrackerX.fromSignal);

    gameTrackerX.SwitchToNewStreamUnit = 0;

    if (gameTrackerX.SwitchToNewWarpIndex != -1)
    {
        SndPlayVolPan(388, 127, 64, 0);

        CurrentWarpNumber = gameTrackerX.SwitchToNewWarpIndex;
    }
}

StreamUnit *STREAM_LoadLevel(char *baseAreaName, StreamUnitPortal *streamPortal, int loadnext)
{
    int i;
    long streamID;
    StreamUnit *streamUnit;
    Level *level;
    char dramName[80];

    (void)loadnext;

    streamID = -1;

    if (streamPortal != NULL)
    {
        streamID = streamPortal->streamID;
    }

    for (i = 0; i < 16; i++)
    {
        streamUnit = &StreamTracker.StreamList[i];

        if ((streamUnit->used != 0) && (strcmpi(streamUnit->baseAreaName, baseAreaName) == 0))
        {
            if (streamUnit->used == 3)
            {
                streamUnit->used = 1;
                break;
            }
            else if (streamUnit->used != 1)
            {
                streamUnit->FrameCount = 0;

                if (streamPortal == NULL)
                {
                    strcpy(gameTrackerX.baseAreaName, baseAreaName);

                    STREAM_SetMainFog(streamUnit);

                    gameTrackerX.StreamUnitID = streamUnit->StreamUnitID;

                    gameTrackerX.level = streamUnit->level;
                }
                else
                {
                    level = streamUnit->level;

                    STREAM_ConnectStream(streamUnit);

                    if (gameTrackerX.gameData.asmData.MorphType != 0)
                    {
                        STREAM_SetStreamFog(streamUnit, (short)level->spectralFogNear, (short)level->spectralFogFar);
                    }
                    else
                    {
                        STREAM_SetStreamFog(streamUnit, (short)level->holdFogNear, (short)level->holdFogFar);
                    }
                }

                break;
            }
            else
            {
                break;
            }
        }
    }

    if (i == 16)
    {
        for (i = 0; i < 16; i++)
        {
            streamUnit = &StreamTracker.StreamList[i];

            if (streamUnit->used == 0)
            {
                STREAM_FillOutFileNames(baseAreaName, dramName, NULL, NULL);

                streamUnit->used = 1;

                strcpy(streamUnit->baseAreaName, baseAreaName);

                streamUnit->StreamUnitID = streamID;

                streamUnit->FrameCount = 0;

                streamUnit->flags = 0;

                if (streamPortal == NULL)
                {
                    strcpy(gameTrackerX.baseAreaName, baseAreaName);

                    gameTrackerX.StreamUnitID = streamUnit->StreamUnitID;

                    LOAD_NonBlockingBinaryLoad(dramName, (void *)STREAM_LoadLevelReturn, NULL, streamUnit, (void **)&streamUnit->level, 2);

                    break;
                }
                else
                {
                    streamPortal->toStreamUnit = NULL;

                    LOAD_NonBlockingBinaryLoad(dramName, (void *)STREAM_StreamLoadLevelReturn, NULL, streamUnit, (void **)&streamUnit->level, 2);

                    break;
                }
            }
        }
    }

    return streamUnit;
}

void RemoveIntroducedLights(Level *level)
{
    int i;

    LIGHT_Restore(gameTrackerX.lightInfo);

    gameTrackerX.lightInfo->numSavedColors = 0;

    for (i = 0; i < level->numSpotLights; i++)
    {
        if ((level->spotLightList[i].flags & 0x10))
        {
            LIST_DeleteFunc(&level->spotLightList[i].node);
        }
    }

    for (i = 0; i < level->numPointLights; i++)
    {
        if ((level->pointLightList[i].flags & 0x10))
        {
            LIST_DeleteFunc(&level->pointLightList[i].node);
        }
    }
}

void STREAM_RemoveInstancesWithIDInInstanceList(InstanceList *list, long id, Level *level)
{
    Instance *instance;
    Instance *next;

    instance = list->first;

    while (instance != NULL)
    {
        next = instance->next;

        if (instance->currentStreamUnitID == id)
        {
            SAVE_Instance(instance, level);

            INSTANCE_ReallyRemoveInstance(list, instance, 0);

            instance = next;
        }
        else
        {
            if (instance->birthStreamUnitID == id)
            {
                SAVE_Instance(instance, level);

                instance->intro = NULL;
            }

            instance = next;
        }
    }
}

void STREAM_MarkUnitNeeded(long streamID)
{
    int i;

    for (i = 0; i < 16; i++)
    {
        if ((StreamTracker.StreamList[i].used != 0) && (streamID == StreamTracker.StreamList[i].StreamUnitID))
        {
            StreamTracker.StreamList[i].FrameCount = gameTrackerX.displayFrameCount;
            break;
        }
    }
}

void STREAM_DumpUnit(StreamUnit *streamUnit, long doSave)
{
    int i;
    int j;
    int numportals;

    for (i = 0; i < 16; i++)
    {
        if (StreamTracker.StreamList[i].used == 2)
        {
            StreamUnitPortal *p; // not from decls.h

            numportals = *(long *)StreamTracker.StreamList[i].level->terrain->StreamUnits;

            p = (StreamUnitPortal *)((long *)StreamTracker.StreamList[i].level->terrain->StreamUnits + 1);

            for (j = 0; j < numportals; j++, p++)
            {
                if (p->toStreamUnit == streamUnit)
                {
                    p->toStreamUnit = NULL;
                }
            }
        }
    }

    if ((streamUnit->used == 1) || (streamUnit->used == 3))
    {
        char dramName[80];

        STREAM_FillOutFileNames(streamUnit->baseAreaName, dramName, NULL, NULL);

        LOAD_AbortFileLoad(dramName, (void *)STREAM_StreamLoadLevelAbort);

        streamUnit->used = 0;

        streamUnit->flags = 0;
        return;
    }

    if (WARPGATE_IsUnitWarpRoom(streamUnit) != 0)
    {
        WARPGATE_RemoveFromArray(streamUnit);
    }

    EVENT_RemoveStreamToInstanceList(streamUnit);

    for (i = 0; i < streamUnit->level->NumberOfSFXMarkers; i++)
    {
        SFXMkr *sfxMkr;

        sfxMkr = &streamUnit->level->SFXMarkerList[i];

        SOUND_EndInstanceSounds(sfxMkr->soundData, sfxMkr->sfxTbl);
    }

    if (streamUnit->sfxFileHandle != 0)
    {
        aadFreeDynamicSfx(streamUnit->sfxFileHandle);
    }

    PLANAPI_DeleteNodeFromPoolByUnit(streamUnit->StreamUnitID);

    STREAM_RemoveInstancesWithIDInInstanceList(gameTrackerX.instanceList, streamUnit->StreamUnitID, streamUnit->level);

    if (doSave != 0)
    {
        EVENT_SaveEventsFromLevel(streamUnit->StreamUnitID, streamUnit->level);

        SAVE_CreatedSavedLevel(streamUnit->StreamUnitID, streamUnit->level);
    }

    MEMPACK_Free((char *)streamUnit->level);

    streamUnit->level = NULL;

    streamUnit->used = 0;

    streamUnit->flags = 0;
}

void STREAM_DumpAllUnitsNotNeeded()
{
    int i;

    for (i = 0; i < 16; i++)
    {
        if ((StreamTracker.StreamList[i].used != 0) && (StreamTracker.StreamList[i].FrameCount != (long)gameTrackerX.displayFrameCount))
        {
            STREAM_DumpUnit(&StreamTracker.StreamList[i], 1);
        }
    }
}

void STREAM_DumpAllLevels(long IDNoRemove, int DoSave)
{
    int i;

    for (i = 0; i < 16; i++)
    {
        if ((StreamTracker.StreamList[i].used != 0) && (StreamTracker.StreamList[i].StreamUnitID != IDNoRemove))
        {
            STREAM_DumpUnit(&StreamTracker.StreamList[i], DoSave);
        }
    }
}

void STREAM_LoadCurrentWarpRoom(StreamUnitPortal *streamPortal, StreamUnit *mainStreamUnit)
{
    if (strcmpi(mainStreamUnit->level->worldName, WarpRoomArray[CurrentWarpNumber].name) == 0)
    {
        WarpGateLoadInfo.loading = 3;

        WarpGateLoadInfo.curTime = WarpGateLoadInfo.maxTime;
    }

    WarpRoomArray[CurrentWarpNumber].streamUnit = STREAM_LoadLevel(WarpRoomArray[CurrentWarpNumber].name, streamPortal, 0);

    if (WarpRoomArray[CurrentWarpNumber].streamUnit != NULL)
    {
        streamPortal->toStreamUnit = WarpRoomArray[CurrentWarpNumber].streamUnit;

        WarpRoomArray[CurrentWarpNumber].streamUnit->flags |= 0x1;
    }
}

void WARPGATE_RelocateLoadedWarpRooms(StreamUnit *mainUnit, StreamUnitPortal *streamPortal)
{
    int i;

    for (i = 0; i < 16; i++)
    {
        if ((StreamTracker.StreamList[i].used == 2) && (&StreamTracker.StreamList[i] != mainUnit)
        && ((StreamTracker.StreamList[i].flags & 0x1)))
        {
            STREAM_LoadLevel(StreamTracker.StreamList[i].baseAreaName, streamPortal, 0);
        }
    }
}

long WARPGATE_GetWarpRoomIndex(char *name)
{
    int i;

    for (i = 0; i < 14; i++)
    {
        if (strcmpi(WarpRoomArray[i].name, name) == 0)
        {
            return i;
        }
    }

    return -1;
}

void WARPGATE_UpdateAddToArray(StreamUnit *streamUnit)
{
    int i;

    i = WARPGATE_GetWarpRoomIndex(streamUnit->baseAreaName);

    if (i != -1)
    {
        WarpRoomArray[i].streamUnit = streamUnit;
    }
}

void WARPGATE_RemoveFromArray(StreamUnit *streamUnit)
{
    if (WARPGATE_GetWarpRoomIndex(streamUnit->baseAreaName) == -1)
    {
        CurrentWarpNumber = 0;
    }
}

void WARPGATE_Init()
{
    int n;

    WarpGateLoadInfo.fadeValue = 4096;

    WarpGateLoadInfo.warpgate_in_use = 0;

    WarpGateLoadInfo.loading = 0;

    WarpGateLoadInfo.blocked = 0;

    WarpGateLoadInfo.curTime = 0;
    WarpGateLoadInfo.maxTime = 61440;

    WarpGateLoadInfo.warpFaceInstance = NULL;

    CurrentWarpNumber = 0;

    for (n = 13; n >= 0; n--)
    {
        WarpRoomArray[n].streamUnit = NULL;
    }
}

void WARPGATE_StartUsingWarpgate()
{
    if (WarpGateLoadInfo.warpgate_in_use == 0)
    {
        SndPlayVolPan(367, 127, 64, 0);
    }

    WarpGateLoadInfo.warpgate_in_use = 1;
}

void WARPGATE_EndUsingWarpgate()
{
    if (WarpGateLoadInfo.warpgate_in_use == 1)
    {
        SndPlayVolPan(386, 127, 64, 0);
    }

    WarpGateLoadInfo.warpgate_in_use = 0;
}

int WARPGATE_IsWarpgateInUse()
{
    return WarpGateLoadInfo.warpgate_in_use;
}

int WARPGATE_IsWarpgateActive()
{
    return WarpGateLoadInfo.loading != 0;
}

int WARPGATE_IsWarpgateUsable()
{
    return (WarpGateLoadInfo.loading == 4) && (WarpGateLoadInfo.blocked == 0);
}

int WARPGATE_IsWarpgateReady()
{
    return WarpGateLoadInfo.loading == 4;
}

int WARPGATE_IsWarpgateSpectral()
{
    return strcmpi(WarpRoomArray[CurrentWarpNumber].name, D_800D1954) == 0;
}

int WARPGATE_IsObjectOnWarpSide(Instance *instance)
{
    int side;
    int temp, temp2; // not from decls.h

    if (WarpGateLoadInfo.warpFaceInstance != NULL)
    {
        side = ~(WarpGateLoadInfo.warpFaceInstance->position.y - theCamera.core.position.y);

        temp = side < 0;

        temp2 = temp;

        if ((WarpGateLoadInfo.warpFaceInstance->position.y - instance->position.y) < 0)
        {
            if (temp == 1)
            {
                return 1;
            }
        }
        else if (temp2 == 0)
        {
            return 1;
        }
    }

    return 0;
}

void WARPGATE_IsItActive(StreamUnit *streamUnit)
{
    Level *level;
    int d;
    int temp; // not from decls.h

    level = streamUnit->level;

    streamUnit->flags |= 0x1;

    if (level->PuzzleInstances == NULL)
    {
        return;
    }

    for (d = 0; d < level->PuzzleInstances->numPuzzles; d++)
    {
        temp = level->PuzzleInstances->eventInstances[d]->eventNumber;

        if (temp != 1)
        {
            continue;
        }

        if ((gameTrackerX.streamFlags & 0x400000))
        {
            *level->PuzzleInstances->eventInstances[d]->eventVariables = temp;
        }

        if (*level->PuzzleInstances->eventInstances[d]->eventVariables == temp)
        {
            streamUnit->flags |= 0x8;
        }

        break;
    }
}

long WARPGATE_IsUnitWarpRoom(StreamUnit *streamUnit)
{
    Level *level;
    long isWarpRoom;
    StreamUnitPortal *streamPortal;
    long numPortals;
    long d;

    level = streamUnit->level;

    isWarpRoom = 0;

    numPortals = *(long *)level->terrain->StreamUnits;

    streamPortal = (StreamUnitPortal *)((long *)level->terrain->StreamUnits + 1);

    for (d = 0; d < numPortals; d++, streamPortal++)
    {
        if ((streamPortal->flags & 0x1))
        {
            isWarpRoom = 1;
        }
    }

    return isWarpRoom;
}

void WARPGATE_FixUnit(StreamUnit *streamUnit)
{
    if (WARPGATE_IsUnitWarpRoom(streamUnit) != 0)
    {
        WARPGATE_IsItActive(streamUnit);
    }
}

void STREAM_MarkWarpUnitsNeeded()
{
    int i;

    for (i = 0; i < 16; i++)
    {
        if ((StreamTracker.StreamList[i].flags & 0x1))
        {
            StreamTracker.StreamList[i].FrameCount = gameTrackerX.displayFrameCount;
        }
    }
}

long WARPGATE_IncrementIndex()
{
    long result;

    result = 1;

    if (WarpGateLoadInfo.loading == 4)
    {
        SndPlayVolPan(387, 127, 64, 0);

        WarpGateLoadInfo.loading = 1;

        WarpGateLoadInfo.curTime = 0;

        WarpGateLoadInfo.warpFaceInstance->fadeValue = 4096;

        WarpGateLoadInfo.warpFaceInstance = NULL;

        WarpRoomArray[CurrentWarpNumber].streamUnit = NULL;

        CurrentWarpNumber = (CurrentWarpNumber + 1) % 14;

        if (strcmpi(gameTrackerX.baseAreaName, WarpRoomArray[CurrentWarpNumber].name) == 0)
        {
            CurrentWarpNumber = (CurrentWarpNumber + 1) % 14;
        }

        hud_warp_arrow_flash = -8192;
    }

    return result;
}

void WARPGATE_CalcWarpFade(int timeInc)
{
    WarpGateLoadInfo.warpFaceInstance->fadeValue = WarpGateLoadInfo.fadeValue;

    WarpGateLoadInfo.curTime += timeInc;

    WarpGateLoadInfo.fadeValue = 4096 - (short)((WarpGateLoadInfo.curTime << 12) / WarpGateLoadInfo.maxTime);

    if (WarpGateLoadInfo.fadeValue >= 4097)
    {
        WarpGateLoadInfo.fadeValue = 4096;
    }

    if (WarpGateLoadInfo.fadeValue < 0)
    {
        WarpGateLoadInfo.fadeValue = 0;
    }
}

long WARPGATE_DecrementIndex()
{
    long result;

    result = 1;

    if (WarpGateLoadInfo.loading == 4)
    {
        SndPlayVolPan(387, 127, 64, 0);

        WarpGateLoadInfo.loading = 1;

        WarpGateLoadInfo.curTime = 0;

        WarpGateLoadInfo.warpFaceInstance->fadeValue = 4096;

        WarpGateLoadInfo.warpFaceInstance = NULL;

        WarpRoomArray[CurrentWarpNumber].streamUnit = NULL;

        CurrentWarpNumber--;

        if (CurrentWarpNumber < 0)
        {
            CurrentWarpNumber = 13;
        }

        if (strcmpi(gameTrackerX.baseAreaName, WarpRoomArray[CurrentWarpNumber].name) == 0)
        {
            CurrentWarpNumber--;

            if (CurrentWarpNumber < 0)
            {
                CurrentWarpNumber = 13;
            }
        }

        hud_warp_arrow_flash = 8192;
    }

    return result;
}

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", PreloadAllConnectedUnits);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocateLevel);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocateCameras);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocateSavedCameras);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocateLevelWithInstances);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocateTerrain);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocateVMObjects);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocateBGObjects);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocatePlanPool);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocatePlanMarkers);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocateSFXMarkers);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_AdjustMultiSpline);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_RelocateInstance);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_OffsetInstancePosition);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_SetInstancePosition);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocateInstances);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", RelocateStreamPortals);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_PackVRAMObject);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_SetupInstanceFlags);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_SetupInstanceListFlags);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_InMorphInstanceListFlags);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_InMorphDoFadeValues);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_UpdateTimeMult);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_UpdateNormals);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_BringBackNormals);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_AddOffsets);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_SubtractOffsets);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_GetComponentsForTrackingPoint);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_AveragePoint);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_UpdateTrackingPoint);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_ToggleMorph);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_DoStep);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_SetFog);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_UpdateTextures);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", MORPH_Continue);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_MORPH_Relocate);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", AddVertex);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", GetPlaneDist);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", CalcVert);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", AddClippedTri);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_GetClipRect);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", GetFogColor);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", DrawFogRectangle);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_RenderAdjacantUnit);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_GetBspTree);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", WARPGATE_BlockWarpGateEntrance);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", WARPGATE_DrawWarpGateRim);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", WARPGATE_HideAllCloudCovers);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", WARPGATE_UnHideCloudCoverInUnit);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_RenderWarpGate);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", WARPGATE_RenderWarpUnit);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_DumpNonResidentObjects);

INCLUDE_ASM("asm/nonmatchings/Game/STREAM", STREAM_TryAndDumpNonResident);
