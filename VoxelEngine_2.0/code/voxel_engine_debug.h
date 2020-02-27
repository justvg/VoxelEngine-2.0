#pragma once

#if VOXEL_ENGINE_INTERNAL


enum debug_value_event_group
{
    DebugValueEventGroup_Rendering,
    DebugValueEventGroup_World,
    DebugValueEventGroup_DebugTools,
    
    DebugValueEventGroup_Count
};

enum debug_event_type
{
    DebugEvent_FrameMarker,
    DebugEvent_BeginBlock,
    DebugEvent_EndBlock,

    DebugEvent_SaveDebugVariable,

    DebugEvent_Group,
    DebugEvent_bool32,
    DebugEvent_r32,
    DebugEvent_u32,
    DebugEvent_vec3,
};
struct debug_event
{
    // STUPID C++ WANTS ME TO DO THIS CONSTRUCTOR FOR NO REASON IF I INTRODUCE vec3 IN HERE 
    debug_event() {}

    char *FileName;
    char *Name;
    u32 LineNumber;

    u64 Clock;
    
    u16 ThreadID;
    u8 Type;
    u8 Group;

    union 
    {
        debug_event *Value_debug_event;
        bool32 Value_bool32;
        r32 Value_r32;
        u32 Value_u32;
        vec3 Value_vec3;
    };
};

struct debug_stored_event
{
    union
    {
        debug_stored_event *Next;
        debug_stored_event *NextFree;
    };

    debug_event Event;
    u32 FrameIndex;
};

struct open_debug_block
{
    debug_event *Event;

	union
	{
		open_debug_block* Parent;
		open_debug_block* NextFree;
	};
};

struct debug_region
{
    char *ParentRegionName;

    debug_event *Event;
    r32 StartCyclesInFrame;
    r32 EndCyclesInFrame;

    u32 LaneIndex;
    u32 ColorIndex;
};

struct debug_thread
{
    u32 ID;
    u32 LaneIndex;
    open_debug_block *OpenDebugBlocks;

    debug_thread *Next;
};

struct debug_frame
{
    union 
    {
        struct 
        {
            debug_frame *Next;
            debug_frame *Prev;
        };
        debug_frame *NextFree;
    };

    u64 BeginClock;
    u64 EndClock;
    r32 MSElapsed;

    u32 RegionsCount;
    debug_region *Regions;

    u32 Index;

    // NOTE(georgy): Saved value events for this frame. 
    u32 ValuesCount;
    debug_event *ValuesEvents[64];
}; 

#define MAX_DEBUG_EVENT_COUNT 1024
#define MAX_REGIONS_PER_FRAME 1024
struct debug_table
{
    bool32 ProfilePause;
    u32 CurrentEventArrayIndex;
    volatile u64 EventArrayIndex_EventIndex;
    u32 EventCount;
    debug_event EventsArrays[2][MAX_DEBUG_EVENT_COUNT];

    char *ProfileBlockName;
};

global_variable debug_table GlobalDebugTable;

struct debug_state
{
	bool32 IsInitialized;

    stack_allocator Allocator;
    stack_allocator FrameAllocator;

    mat4 Orthographic;

	vec2 BufferDim;
	vec2 TextP;
	r32 FontScale;

	font_id FontID;
    loaded_font *Font;

	shader GlyphShader;
    shader QuadShader;

    vec2 GlyphVertices[4];
	GLuint GlyphVAO, GlyphVBO;

    GLuint OrthoUBO;

    u32 VariablesCount;
    debug_event *VariablesEvents[64];

    debug_event Groups[DebugValueEventGroup_Count];

    debug_stored_event *OldestStoredEvent;
    debug_stored_event *MostRecentStoredEvent;
    debug_stored_event *FirstFreeStoredEvent;

    debug_event *NextHotInteraction;
    debug_event *HotInteraction;
    debug_event *ActiveInteraction;

    bool32 ProfilePause;

    u32 TotalFrameCount;

    debug_frame FramesSentinel;
    debug_frame *CollationFrame;
    debug_frame *FirstFreeFrame;

    u32 LaneCount;
    debug_thread *FirstThread;

    open_debug_block *FirstFreeDebugBlock;

    u32 DebugRegionsCount;
    debug_region *DebugRegions;

    vec2 LastMouseP;
};

inline debug_event *
RecordDebugEvent(debug_event_type Type, char *FileName, char *Name, u32 LineNumber)
{
	debug_event *Event = 0;
    if(!GlobalDebugTable.ProfilePause)
    {
        u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable.EventArrayIndex_EventIndex, 1);
        u32 ArrayIndex = ArrayIndex_EventIndex >> 32;
        u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;
        Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);
        Event = GlobalDebugTable.EventsArrays[ArrayIndex] + EventIndex;
        Event->FileName = FileName;
        Event->Name = Name;
        Event->LineNumber = LineNumber;
        Event->Clock = __rdtsc();
        Event->ThreadID = (u16)GetThreadID();
        Event->Type = (u8)Type;
	}

	return(Event);
}

#define TIME_BLOCK__(Number) timed_block TimedBlock_##Number(__FILE__, __FUNCTION__, __LINE__); 
#define TIME_BLOCK_(Number) TIME_BLOCK__(Number)
#define TIME_BLOCK TIME_BLOCK_(__LINE__)

#define FRAME_MARKER(MSElapsedInit) \
   {debug_event *Event = RecordDebugEvent(DebugEvent_FrameMarker, __FILE__, "Frame Marker", __LINE__); \
    if(Event) Event->Value_r32 = MSElapsedInit;}

#define BEGIN_BLOCK_(FileName, BlockName, LineNumber) \
   {RecordDebugEvent(DebugEvent_BeginBlock, FileName, BlockName, LineNumber);}

#define END_BLOCK_(...) \
   {RecordDebugEvent(DebugEvent_EndBlock, __FILE__, "End Block", __LINE__);}

#define BEGIN_BLOCK(Name) BEGIN_BLOCK_(__FILE__, #Name, __LINE__)

#define END_BLOCK(Name) END_BLOCK_(Name)

struct timed_block
{
    timed_block(char *FileName, char *BlockName, u32 LineNumber)
    {
        BEGIN_BLOCK_(FileName, BlockName, LineNumber);
    };

    ~timed_block()
    {
        END_BLOCK_();
    }
};

struct record_playback_info
{
    bool32 RecordPhaseStarted;
    // NOTE(georgy): These are sim bounds when we start recording phase. 
    // They are needed to know what chunks to update when we finish recording phase
    world_position MinChunkP;
    world_position MaxChunkP;

    bool32 RecordPhase;
    bool32 RefreshNow;
    bool32 PlaybackPhase;

    u32 ChunksModifiedDuringRecordPhaseCount;
    chunk *ChunksModifiedDuringRecordPhase[256];

    u32 ChunksUnloadedDuringRecordPhaseCount;
    chunk *ChunksUnloadedDuringRecordPhase[4096];
};

global_variable record_playback_info DEBUGGlobalPlaybackInfo;

inline debug_event DEBUGInitializeValue(debug_event *SubEvent, debug_event_type Type, debug_value_event_group Group,
                                        char *FileName, char *Name, u32 LineNumber)
{
    debug_event *Event = RecordDebugEvent(DebugEvent_SaveDebugVariable, 0, "", 0);
    Event->Value_debug_event = SubEvent;

    SubEvent->FileName = FileName;
    SubEvent->Name = Name;
    SubEvent->LineNumber = LineNumber;
    SubEvent->Clock = 0;
    SubEvent->Type = (u8)Type;
    SubEvent->Group = (u8)Group;

	return(*SubEvent);
}

#define DEBUG_IF(Name, Group) \
local_persist debug_event DebugEvent##Name = DEBUGInitializeValue(&DebugEvent##Name, (DebugEvent##Name.Value_bool32 = GlobalConstants_##Name, DebugEvent_bool32), DebugValueEventGroup_##Group, __FILE__, #Name, __LINE__); \
bool32 Name = DebugEvent##Name.Value_bool32; \
if(Name)

#define DEBUG_VARIABLE(type, Name, Group) \
local_persist debug_event DebugEvent##Name = DEBUGInitializeValue(&DebugEvent##Name, (DebugEvent##Name.Value_##type = GlobalConstants_##Name, DebugEvent_##type), DebugValueEventGroup_##Group, __FILE__, #Name, __LINE__); \
type Name = DebugEvent##Name.Value_##type; 

#define DEBUG_VALUE(type, Name, GroupInit, Value) \
debug_event *DebugEvent##Name = RecordDebugEvent(DebugEvent_##type, __FILE__, #Name, __LINE__); \
if(DebugEvent##Name) \
{ \
    DebugEvent##Name->Group = DebugValueEventGroup_##GroupInit; \
    DebugEvent##Name->Value_##type = Value; \
}

#else 

#define TIME_BLOCK__(...)
#define TIME_BLOCK_(...)
#define TIME_BLOCK
#define FRAME_MARKER(...)
#define BEGIN_BLOCK_(...)
#define END_BLOCK_(...)
#define BEGIN_BLOCK(...)
#define END_BLOCK(...)

#define DEBUG_IF(Name, ...) \
bool32 Name = GlobalConstants_##Name; \
if(Name)

#define DEBUG_VARIABLE(type, Name, ...) \
type Name = GlobalConstants_##Name; 

#define DEBUG_VALUE(...)

#endif