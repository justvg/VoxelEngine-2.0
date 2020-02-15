#pragma once

#if !defined(COMPILER_MSVC)
	#define COMPILER_MSVC 0
#endif

#if !COMPILER_MSVC
	#if _MSC_VER
		#undef COMPILER_MSVC
		#define COMPILER_MSVC 1
	#endif
#endif

#if COMPILER_MSVC
	#include <intrin.h>
#endif

#include <stdint.h>

#define internal static
#define global_variable static
#define local_persist static

#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0; }
#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase Assert(!"InvalidDefaultCase")

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)

typedef  uint8_t u8;
typedef  uint8_t bool8;
typedef  uint16_t u16;
typedef  uint32_t u32;
typedef  uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef int32_t bool32;

typedef float r32;
typedef double r64;

inline u32
CheckTruncationUInt64(u64 Value)
{
	Assert(Value <= 0xFFFFFFFF);
	u32 Result = (u32)Value;
	return(Result);
}

#define OUTPUT_DEBUG_STRING(name) void name(char *String)
typedef OUTPUT_DEBUG_STRING(platform_output_debug_string);

struct read_entire_file_result
{
	u32 Size;
	void *Memory;
};
#define READ_ENTIRE_FILE(name) read_entire_file_result name(char *Filename)
typedef READ_ENTIRE_FILE(platform_read_entire_file);


#define ALLOCATE_MEMORY(name) void *name(u64 Size)
typedef ALLOCATE_MEMORY(platform_allocate_memory);

#define FREE_MEMORY(name) void name(void *Memory)
typedef FREE_MEMORY(platform_free_memory);

struct platform_job_system_queue;
#define PLATFORM_JOB_SYSTEM_CALLBACK(name) void name(platform_job_system_queue *JobSystem, void *Data)
typedef PLATFORM_JOB_SYSTEM_CALLBACK(platform_job_system_callback);

#define PLATFORM_ADD_ENTRY(name) void name(platform_job_system_queue *JobSystem, platform_job_system_callback *Callback, void *Data)
typedef PLATFORM_ADD_ENTRY(platform_add_entry);

#define PLATFORM_COMPLETE_ALL_WORK(name) void name(platform_job_system_queue *JobSystem)
typedef PLATFORM_COMPLETE_ALL_WORK(platform_complete_all_work);

struct loaded_texture;
struct loaded_font
{
	loaded_texture *Glyphs;
	r32 *HorizontalAdvances;
	
	u32 LineAdvance;
	u32 AscenderHeight;

	u32 GlyphsCount;
	u32 FirstCodepoint;
	u32 LastCodepoint;
};

#define PLATFORM_BEGIN_FONT(name) void name(loaded_font *Font, char *Filename, char *FontName)
typedef PLATFORM_BEGIN_FONT(platform_begin_font);

#define PLATFORM_END_FONT(name) void name(loaded_font *Font)
typedef PLATFORM_END_FONT(platform_end_font);

#define PLATFORM_LOAD_CODEPOINT_BITMAP(name) u8 *name(loaded_font *Font, u32 Codepoint, u32 *Width, u32 *Height, r32 *AlignPercentageY)
typedef PLATFORM_LOAD_CODEPOINT_BITMAP(platform_load_codepoint_bitmap);


struct button
{
	bool32 EndedDown;
	u32 HalfTransitionCount;
};

struct game_input
{
	r32 dt;
	r32 MouseX, MouseY;
	i32 MouseXDisplacement, MouseYDisplacement;
	button MouseLeft, MouseRight;

	union
	{
		button Buttons[8];
		struct
		{
			button MoveForward;
			button MoveBack;
			button MoveRight;
			button MoveLeft;
			button MoveUp;

			button Pause;
		};
	};
};

inline bool32
WasDown(button *Button)
{
	bool32 Result = (Button->HalfTransitionCount > 1) ||
					((Button->HalfTransitionCount == 1) && Button->EndedDown);

	return(Result);
}

inline bool32
WasUp(button *Button)
{
	bool32 Result = (Button->HalfTransitionCount > 1) ||
					((Button->HalfTransitionCount == 1) && !Button->EndedDown);

	return(Result);
}

struct game_sound_output_buffer
{
	i16 *Samples;
	u32 SamplesPerSecond;
	u32 SampleCount;
};

struct platform_api
{
	platform_add_entry *AddEntry;
	platform_complete_all_work *CompleteAllWork;
	platform_read_entire_file *ReadEntireFile;
	platform_allocate_memory *AllocateMemory;
	platform_free_memory *FreeMemory;
	platform_output_debug_string *OutputDebugString;

	platform_begin_font *BeginFont;
	platform_load_codepoint_bitmap *LoadCodepointBitmap;
	platform_end_font *EndFont;
};

struct game_memory
{
	u64 PermanentStorageSize;
	void *PermanentStorage;

	u64 TemporaryStorageSize;
	void *TemporaryStorage;

	u64 DebugStorageSize;
	void *DebugStorage;

	platform_job_system_queue *JobSystemQueue;

	platform_api PlatformAPI;
};