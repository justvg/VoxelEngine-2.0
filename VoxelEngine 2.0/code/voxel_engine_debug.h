#pragma once

#if VOXEL_ENGINE_INTERNAL

enum debug_event_type
{
    DebugEvent_FrameMarker,
    DebugEvent_BeginBlock,
    DebugEvent_EndBlock,

    DebugEvent_SaveDebugValue,

    DebugEvent_bool32,
    DebugEvent_r32,
};
struct debug_event
{
    char *FileName;
    char *Name;
    u32 LineNumber;

    u64 Clock;
    u8 Type;

    union 
    {
        debug_event *Value_debug_event;
        r32 Value_r32;
        bool32 Value_bool32;

        u16 ThreadID;
    };
};

struct open_debug_block
{
    debug_event *Event;

    open_debug_block *Parent;
};

struct debug_region
{
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
    u64 BeginClock;
    u64 EndClock;
    r32 MSElapsed;

    u32 RegionsCount;
    debug_region *Regions;
}; 

#define DEBUG_EVENTS_ARRAYS_COUNT 10
#define MAX_FRAME_COUNT DEBUG_EVENTS_ARRAYS_COUNT*8
#define MAX_DEBUG_EVENT_COUNT 1024
#define MAX_REGIONS_PER_FRAME 1024
struct debug_table
{
    bool32 ProfilePause;
    u32 CurrentEventArrayIndex;
    volatile u64 EventArrayIndex_EventIndex;
    u32 EventsCounts[DEBUG_EVENTS_ARRAYS_COUNT];
    debug_event EventsArrays[DEBUG_EVENTS_ARRAYS_COUNT][MAX_DEBUG_EVENT_COUNT];

    char *ProfileBlockName;
};

global_variable debug_table GlobalDebugTable;

struct debug_state
{
	bool32 IsInitialized;

    stack_allocator Allocator;
    temporary_memory CollateTemp;

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

    u32 ValuesCount;
    debug_event *ValueEvents[64];

    debug_event *NextHotInteraction;
    debug_event *HotInteraction;
    debug_event *ActiveInteraction;

    bool32 ProfilePause;

    u32 FrameCount;
    debug_frame Frames[MAX_FRAME_COUNT];
    debug_frame *CollationFrame;

    u32 LaneCount;
    debug_thread *FirstThread;

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
        Event->ThreadID = GetThreadID();
        Event->Type = Type;
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

    u32 ChunksModifiedDuringRecordPhaseCount;
    chunk *ChunksModifiedDuringRecordPhase[256];

    u32 ChunksUnloadedDuringRecordPhaseCount;
    chunk *ChunksUnloadedDuringRecordPhase[2048];
};

global_variable record_playback_info DEBUGGlobalPlaybackInfo;

inline debug_event DEBUGInitializeValue(debug_event *SubEvent, debug_event_type Type, 
                                        char *FileName, char *Name, u32 LineNumber)
{
    debug_event *Event = RecordDebugEvent(DebugEvent_SaveDebugValue, 0, "", 0);
    Event->Value_debug_event = SubEvent;

    SubEvent->FileName = FileName;
    SubEvent->Name = Name;
    SubEvent->LineNumber = LineNumber;
    SubEvent->Clock = 0;
    SubEvent->Type = Type;

	return(*SubEvent);
}

#define DEBUG_IF(Name) \
local_persist debug_event DebugEvent##Name = DEBUGInitializeValue(&DebugEvent##Name, (DebugEvent##Name.Value_bool32 = GlobalConstants_##Name, DebugEvent_bool32), __FILE__, #Name, __LINE__); \
bool32 Name = DebugEvent##Name.Value_bool32; \
if(Name)

#define DEBUG_VARIABLE(type, Name) \
local_persist debug_event DebugEvent##Name = DEBUGInitializeValue(&DebugEvent##Name, (DebugEvent##Name.Value_##type = GlobalConstants_##Name, DebugEvent_##type), __FILE__, #Name, __LINE__); \
type Name = DebugEvent##Name.Value_##type; 

#else 

#define TIME_BLOCK__(...)
#define TIME_BLOCK_(...)
#define TIME_BLOCK
#define FRAME_MARKER(...)
#define BEGIN_BLOCK_(...)
#define END_BLOCK_(...)
#define BEGIN_BLOCK(...)
#define END_BLOCK(...)

#define DEBUG_IF(Name) \
bool32 Name = GlobalConstants_##Name; \
if(Name)

#define DEBUG_VARIABLE(type, Name) \
type Name = GlobalConstants_##Name; 

#endif