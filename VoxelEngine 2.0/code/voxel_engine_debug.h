#pragma once

enum debug_event_type
{
    DebugEvent_FrameMarker,
    DebugEvent_BeginBlock,
    DebugEvent_EndBlock,
};
struct debug_event
{
    u64 Clock;
    u16 DebugRecordIndex;
    u8 Type;

    union 
    {
        r32 MSElapsed;
        u16 ThreadID;
    };
};

struct debug_record
{
    char *FileName;
    char *BlockName;
    u32 LineNumber;
};

struct open_debug_block
{
    debug_record *Record;
    debug_event *Event;

    open_debug_block *Parent;
};

struct debug_region
{
    debug_record *Record;
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

#define DEBUG_EVENTS_ARRAYS_COUNT 8
#define MAX_FRAME_COUNT DEBUG_EVENTS_ARRAYS_COUNT*8
#define MAX_DEBUG_EVENT_COUNT 1024
#define MAX_REGIONS_PER_FRAME 1024
global_variable volatile u64 GlobalEventArrayIndex_EventIndex;
global_variable u32 GlobalDebugEventsCounts[DEBUG_EVENTS_ARRAYS_COUNT];
global_variable debug_event GlobalDebugEventsArrays[DEBUG_EVENTS_ARRAYS_COUNT][MAX_DEBUG_EVENT_COUNT];
debug_record GlobalDebugRecords[]; 

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

	GLuint GlyphVAO, GlyphVBO;

    bool32 ProfilePause;

    u32 FrameCount;
    debug_frame Frames[MAX_FRAME_COUNT];
    debug_frame *CollationFrame;

    u32 LaneCount;
    debug_thread *FirstThread;

    debug_record *ProfileBlockRecord;

    u32 DebugRegionsCount;
    debug_region *DebugRegions;
};

inline debug_event *
RecordDebugEvent(u16 DebugRecordIndex, debug_event_type Type)
{
    u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalEventArrayIndex_EventIndex, 1);
    u32 ArrayIndex = ArrayIndex_EventIndex >> 32;
    u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;
    Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);
    debug_event *Event = GlobalDebugEventsArrays[ArrayIndex] + EventIndex;
    Event->Clock = __rdtsc();
    Event->DebugRecordIndex = DebugRecordIndex;
    Event->ThreadID = GetThreadID();
    Event->Type = Type;

    return(Event);
}

#define TIME_BLOCK__(Number) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __FUNCTION__, __LINE__); 
#define TIME_BLOCK_(Number) TIME_BLOCK__(Number)
#define TIME_BLOCK TIME_BLOCK_(__LINE__)

#define FRAME_MARKER(MSElapsedInit) \
   {int Counter = __COUNTER__; \
    debug_record *Record = GlobalDebugRecords + Counter; \
    Record->FileName = __FILE__; \
    Record->BlockName = "Frame Marker"; \
    Record->LineNumber = __LINE__; \
    debug_event *Event = RecordDebugEvent(Counter, DebugEvent_FrameMarker); \
    Event->MSElapsed = MSElapsedInit;}

#define BEGIN_BLOCK_(CounterInit, FileNameInit, BlockNameInit, LineNumberInit) \
   {debug_record *Record = GlobalDebugRecords + CounterInit; \
    Record->FileName = FileNameInit; \
    Record->BlockName = BlockNameInit; \
    Record->LineNumber = LineNumberInit; \
    RecordDebugEvent(CounterInit, DebugEvent_BeginBlock);}

#define END_BLOCK_(CounterInit, ...) \
   {debug_record *Record = GlobalDebugRecords + CounterInit; \
    RecordDebugEvent(CounterInit, DebugEvent_EndBlock);}

#define BEGIN_BLOCK(Name) \
    u32 Counter##Name = __COUNTER__; \
    BEGIN_BLOCK_(Counter##Name, __FILE__, #Name, __LINE__)

#define END_BLOCK(Name) \
    END_BLOCK_(Counter##Name, Name)

struct timed_block
{
    u32 Counter;
    
    timed_block(u32 CounterInit, char *FileName, char *BlockName, u32 LineNumber)
    {
        Counter = CounterInit;

        BEGIN_BLOCK_(Counter, FileName, BlockName, LineNumber);
    };

    ~timed_block()
    {
        END_BLOCK_(Counter);
    }
};