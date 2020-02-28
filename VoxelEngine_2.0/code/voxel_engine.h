#pragma once

#include "voxel_engine_preprocessor_generated.h"

global_variable platform_api Platform;

#define PLATFORM_FREE_MEMORY_AND_ZERO_POINTER(Memory) \
		Platform.FreeMemory(Memory); \
		(Memory) = 0;

#define DEFAULT_ALIGNMENT 16
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
GetEffectiveSizeFor(stack_allocator *Allocator, u64 SizeInit, u64 Alignment)
{
	u64 AlignmentOffset = GetAlignmentOffset(Allocator, Alignment);
	u64 Size = SizeInit + AlignmentOffset;

	return(Size);
}

#define PushStruct(Allocator, type, ...) (type *)PushSize(Allocator, sizeof(type), ## __VA_ARGS__)
#define PushArray(Allocator, Count, type, ...) (type *)PushSize(Allocator, (Count)*sizeof(type), ## __VA_ARGS__)
inline void *
PushSize(stack_allocator *Allocator, u64 SizeInit, u64 Alignment = DEFAULT_ALIGNMENT)
{
	u64 Size = GetEffectiveSizeFor(Allocator, SizeInit, Alignment);

   	Assert((Allocator->Used + Size) <= Allocator->Size);
	u64 AlignmentOffset = GetAlignmentOffset(Allocator, Alignment);
	void *Result = Allocator->Base + Allocator->Used + AlignmentOffset;
	Allocator->Used += Size;

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
SubAllocator(stack_allocator *Result, stack_allocator *Allocator, u64 Size, u64 Alignment = DEFAULT_ALIGNMENT)
{
	Result->Size = Size;
	Result->Base = (u8 *)PushSize(Allocator, Size, Alignment);
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

#include "voxel_engine_intrinsics.h"
#include "voxel_engine_math.h"
#include "voxel_engine_dynamic_array.h"
#include "voxel_engine_asset.h"
#include "voxel_engine_world.h"
#include "voxel_engine_render.h"
#include "voxel_engine_config.h"
#include "voxel_engine_debug.h"

global_variable mat4 GlobalViewProjection;

struct camera
{
	r32 DistanceFromHero;
	r32 Pitch, Head;

	r32 RotSensetivity;

	r32 NearDistance;
	r32 FarDistance;
	r32 FoV;
	r32 AspectRatio;

	mat4 RotationMatrix;

	vec3 OffsetFromHero;
	vec3 TargetOffset;
	vec3 LastOffsetFromHero;

	vec3 DEBUGP;
	vec3 DEBUGFront;
	r32 DEBUGPitch, DEBUGHead;
};

struct point_light
{
	vec3 P;
	vec3 Color;
};

struct point_lights_info
{
	u32 Count;
	point_light PointLights[64];
};

#include "voxel_engine_animation.h"
#include "voxel_engine_sim_region.h"

struct stored_entity
{
	world_position P;
	sim_entity Sim;
};

struct hero
{
	stored_entity *Entity;

	bool32 Fireball;

	vec3 ddP;
	r32 dY;
	r32 AdditionalRotation;
};

struct pairwise_collision_rule
{
	u32 StorageIndexA;
	u32 StorageIndexB;
	bool32 CanCollide;
};

struct playing_sound
{
	sound_id ID;
	vec2 Volume;
	vec2 dVolume;
	vec2 TargetVolume;
	u32 SamplesPlayed;

	world_position P;

	union
	{
		playing_sound *Next;
		playing_sound *NextFree;
	};
};

struct audio_state
{
	stack_allocator *Allocator;
	playing_sound *FirstPlayingSound;
	playing_sound *FirstFreePlayingSound;
	
	r32 GlobalVolume;
};

enum ubo_binding_point
{
	BindingPoint_Matrices,
	BindingPoint_UIMatrices,
	BindingPoint_DirectionalLightInfo,
	BindingPoint_PointLightInfo,
	BindingPoint_ShadowsInfo,

	BindingPoint_Counts
};

struct game_state
{
	bool32 IsInitialized;

	camera Camera;

	stack_allocator WorldAllocator;
	world World;	

	stack_allocator FundamentalTypesAllocator;

	u32 LastStoredCollisionRule;
	pairwise_collision_rule CollisionRules[256];

	sim_entity_collision_volume *HeroCollision;
	sim_entity_collision_volume *FireballCollision;
	sim_entity_collision_volume *TreeCollision;
	sim_entity_collision_volume *TESTCubeCollision;

	shader CharacterShader;
	shader WorldShader;
	shader WaterShader;
	shader HitpointsShader;
	shader BlockParticleShader;
	shader WorldDepthShader;
	shader CharacterDepthShader;
	shader BlockParticleDepthShader;
	shader UIQuadShader;
	shader UIGlyphShader;
	shader FramebufferScreenShader;

	GLuint UBOs[BindingPoint_Counts];

	animation CharacterAnimations[CharacterAnimation_Count];

	hero Hero;
	world_position LastHeroWorldP;

	r32 t;

	vec3 DirectionalLightDir, DirectionalLightColor;
#define CASCADES_COUNT 3
	GLuint ShadowMapFBO, ShadowMapsArray;
	u32 ShadowMapsWidth, ShadowMapsHeight;
	GLuint ShadowNoiseTexture;
	vec2 ShadowSamplesOffsets[16];

	audio_state AudioState;

	u32 StoredEntityCount;
	stored_entity StoredEntities[10000];

	GLuint ParticleVAO, ParticleVBO, ParticlePVBO, ParticleOffsetVBO, ParticleScaleVBO; 

	particle_generator ParticleGenerator;

	// TODO(georgy): Move this to temp state?
	font_id FontID;
	loaded_font *Font;

	GLuint UIQuadVAO, UIQuadVBO;
	GLuint UIGlyphVAO, UIGlyphVBO;

	GLuint CubeVAO, CubeVBO;
	GLuint QuadVAO, QuadVBO;
};

internal void AddCollisionRule(game_state *GameState, u32 StorageIndexA, u32 StorageIndexB, bool32 CanCollide);

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