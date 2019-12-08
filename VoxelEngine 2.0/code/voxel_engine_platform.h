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
typedef OUTPUT_DEBUG_STRING(output_debug_string);

struct read_entire_file_result
{
	u32 Size;
	void *Memory;
};
#define READ_ENTIRE_FILE(name) read_entire_file_result name(char *Filename)
typedef READ_ENTIRE_FILE(read_entire_file);

#define FREE_FILE_MEMORY(name) void name(void *Memory)
typedef FREE_FILE_MEMORY(free_file_memory);


#define ALLOCATE_MEMORY(name) void *name(u64 Size)
typedef ALLOCATE_MEMORY(allocate_memory);

#define FREE_MEMORY(name) void name(void *Memory)
typedef FREE_MEMORY(free_memory);

struct platform_job_system_queue;
#define PLATFORM_JOB_SYSTEM_CALLBACK(name) void name(platform_job_system_queue *JobSystem, void *Data)
typedef PLATFORM_JOB_SYSTEM_CALLBACK(platform_job_system_callback);

typedef void platform_add_entry(platform_job_system_queue *JobSystem, platform_job_system_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_job_system_queue *JobSystem);

struct game_input
{
	r32 dt;
	i32 MouseX, MouseY;
	i32 MouseXDisplacement, MouseYDisplacement;
	bool8 MouseLeft, MouseRight;

	union
	{
		bool8 Buttons[5];
		struct
		{
			bool8 MoveForward;
			bool8 MoveBack;
			bool8 MoveRight;
			bool8 MoveLeft;
			bool8 MoveUp;
		};
	};
};

struct game_memory
{
	u64 PermanentStorageSize;
	void *PermanentStorage;

	u64 TemporaryStorageSize;
	void *TemporaryStorage;

	platform_job_system_queue *JobSystemQueue;

	platform_add_entry *PlatformAddEntry;
	platform_complete_all_work *PlatformCompleteAllWork;
	read_entire_file *PlatformReadEntireFile;
	free_file_memory *PlatformFreeFileMemory;
	allocate_memory *PlatformAllocateMemory;
	free_memory *PlatformFreeMemory;
	output_debug_string *PlatformOutputDebugString;
};