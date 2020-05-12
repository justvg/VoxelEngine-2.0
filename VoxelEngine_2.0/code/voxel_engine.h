#pragma once

#include "voxel_engine_preprocessor_generated.h"

global_variable platform_api Platform;

#define PLATFORM_FREE_MEMORY_AND_ZERO_POINTER(Memory) \
		Platform.FreeMemory(Memory); \
		(Memory) = 0;

#define DEFAULT_ALIGNMENT 4
struct stack_allocator
{
	u64 Size;
	u8 *Base;
	u64 Used;
};

struct temporary_memory
{
	stack_allocator *Allocator;
	u64 Used;
};

inline void
InitializeStackAllocator(stack_allocator *Allocator, u64 Size, void *Base)
{
	Allocator->Size = Size;
	Allocator->Base = (u8 *)Base;
	Allocator->Used = 0;
}

inline u64
GetAlignmentOffset(stack_allocator *Allocator, u64 Alignment)
{
	u64 ResultPointer = (u64)Allocator->Base + Allocator->Used;
	u64 AlignmentOffset = 0;

	u64 AlignmentMask = Alignment - 1;
	if(ResultPointer & AlignmentMask)
	{
		AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
	}

	return(AlignmentOffset);
}

inline u64
GetAllocatorSizeRemaining(stack_allocator *Allocator, u64 Alignment = DEFAULT_ALIGNMENT)
{
	u64 Result = Allocator->Size - (Allocator->Used + GetAlignmentOffset(Allocator, Alignment));

	return(Result);
}

inline u64
GetEffectiveSizeFor(stack_allocator *Allocator, u64 SizeInit, u64 Alignment)
{
	u64 AlignmentOffset = GetAlignmentOffset(Allocator, Alignment);
	u64 Size = SizeInit + AlignmentOffset;

	return(Size);
}

#define ZeroStruct(Struct) ZeroSize(&(Struct), sizeof(Struct));
#define ZeroArray(Pointer, Count) ZeroSize((Pointer), Count*sizeof((Pointer)[0]));
inline void
ZeroSize(void *Ptr, u64 Size)
{
	// TODO(georgy): Performance!
	u8 *Byte = (u8 *)Ptr;
	while(Size--)
	{
		*Byte++ = 0;
	}
}

#define PushStruct(Allocator, type, ...) (type *)PushSize(Allocator, sizeof(type), ## __VA_ARGS__)
#define PushArray(Allocator, Count, type, ...) (type *)PushSize(Allocator, (Count)*sizeof(type), ## __VA_ARGS__)
inline void *
PushSize(stack_allocator *Allocator, u64 SizeInit, bool32 ZeroMemory = false, u64 Alignment = DEFAULT_ALIGNMENT)
{
	u64 Size = GetEffectiveSizeFor(Allocator, SizeInit, Alignment);

   	Assert((Allocator->Used + Size) <= Allocator->Size);
	u64 AlignmentOffset = GetAlignmentOffset(Allocator, Alignment);
	void *Result = Allocator->Base + Allocator->Used + AlignmentOffset;
	Allocator->Used += Size;

	if(ZeroMemory)
	{
		ZeroSize(Result, SizeInit);
	}

	return(Result);
}

inline bool32
AllocatorHasRoomFor(stack_allocator *Allocator, u64 SizeInit, u64 Alignment = DEFAULT_ALIGNMENT)
{
	u64 Size = GetEffectiveSizeFor(Allocator, SizeInit, Alignment);

	bool32 Result = ((Allocator->Used + Size) <= Allocator->Size);
	return(Result);
}

inline void
SubAllocator(stack_allocator *Result, stack_allocator *Allocator, u64 Size, bool32 ZeroMemory = false, u64 Alignment = DEFAULT_ALIGNMENT)
{
	Result->Size = Size;
	Result->Base = (u8 *)PushSize(Allocator, Size, ZeroMemory, Alignment);
	Result->Used = 0;
}

inline temporary_memory
BeginTemporaryMemory(stack_allocator *Allocator)
{
	temporary_memory Result;
	Result.Allocator = Allocator;
	Result.Used = Allocator->Used;

	return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMemory)
{
	stack_allocator *Allocator = TempMemory.Allocator;
	Assert(Allocator->Used >= TempMemory.Used);
	Allocator->Used = TempMemory.Used;
}

struct game_state;
#include "voxel_engine_intrinsics.h"
#include "voxel_engine_math.h"
#include "voxel_engine_dynamic_array.h"
#include "voxel_engine_asset.h"
#include "voxel_engine_world.h"
#include "voxel_engine_render.h"
#include "voxel_engine_config.h"
#include "voxel_engine_debug.h"
#include "voxel_engine_animation.h"
#include "voxel_engine_audio.h"
#include "voxel_engine_sim_region.h"
#include "voxel_engine_world_mode.h"
#include "voxel_engine_title_screen.h"

global_variable mat4 GlobalViewProjection;

enum game_mode
{
	GameMode_None,

	GameMode_TitleScreen,
	GameMode_World
};

struct game_state
{
	bool32 IsInitialized;

	stack_allocator ModeAllocator;
	stack_allocator AudioAllocator;

	audio_state AudioState;

	game_mode GameMode;
	union
	{
		game_mode_title_screen *TitleScreen;
		game_mode_world *WorldMode;
	};
};

struct temp_state
{
	bool32 IsInitialized;

	stack_allocator Allocator;

	game_assets *GameAssets;

	platform_job_system_queue *JobSystemQueue;
};

#define INVALID_POSITION INT32_MAX
inline void
MakeEntityNonSpatial(sim_entity *Entity)
{
	Entity->P = vec3((r32)INVALID_POSITION, (r32)INVALID_POSITION, (r32)INVALID_POSITION);
	AddFlags(Entity, EntityFlag_NonSpatial);

	// TODO(georgy): Think about this!
#if 0
	if(Entity->Particles)
	{
		for(u32 ParticleIndex = 0;
			ParticleIndex < MAX_PARTICLES_COUNT;
			ParticleIndex++)
		{
			Entity->Particles->Particles[ParticleIndex].LifeTime = 0.0f;
		}
	}
#endif
}

internal void SetGameMode(game_state *GameState, game_mode GameMode);
