#pragma once

#include <stdint.h>

#define internal static
#define global_variable static

#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0; }
#define InvalidCodePath Assert(!"InvalidCodePath")

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

#define Kilobytes(Value) ((Value) * 1024L)
#define Megabytes(Value) (Kilobytes(Value) * 1024L)
#define Gigabytes(Value) (Megabytes(Value) * 1024L)

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


struct game_input
{
	r32 dt;
	i32 MouseX, MouseY;
	i32 MouseXDisplacement, MouseYDisplacement;
	bool8 MouseLeft, MouseRight;

	union
	{
		bool8 Buttons[4];
		struct
		{
			bool8 MoveForward;
			bool8 MoveBack;
			bool8 MoveRight;
			bool8 MoveLeft;
		};
	};
};

struct game_memory
{
	u64 PermanentStorageSize;
	void *PermanentStorage;

	u64 TemporaryStorageSize;
	void *TemporaryStorage;

	read_entire_file *PlatformReadEntireFile;
	free_file_memory *PlatformFreeFileMemory;
	allocate_memory *PlatformAllocateMemory;
	free_memory *PlatformFreeMemory;
	output_debug_string *PlatformOutputDebugString;
};