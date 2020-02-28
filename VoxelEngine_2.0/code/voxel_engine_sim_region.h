#pragma once

struct move_spec
{
	vec3 ddP;
	r32 Speed;
	r32 Drag;
};

enum entity_type
{
	EntityType_Null,

	EntityType_Water, // NOTE(georgy): This is for handling collisions with chunk. 
	EntityType_Chunk, //			   Should I make chunk a real entity?
					  

	EntityType_Hero,
	EntityType_Fireball,
	EntityType_Tree,
};

struct sim_entity;
struct entity_reference 
{
	u32 StorageIndex;
	sim_entity *SimPtr;
};

struct sim_entity_collision_volume
{
	vec3 OffsetP;
	vec3 Dim;

	dynamic_array_vec3 VerticesP;
};

enum sim_entity_flags
{
	EntityFlag_Moveable = 1,
	EntityFlag_OnGround = (1 << 1),
	EntityFlag_NonSpatial = (1 << 2),
	EntityFlag_Collides = (1 << 3),
	EntityFlag_GravityAffected = (1 << 4),
	EntityFlag_InWater = (1 << 5),
};

struct sim_entity
{
	u32 StorageIndex;

	entity_type Type;
	bool32 Updatable;

	bool32 CanBeGravityAffected;
	u32 Flags;

	entity_animation_state AnimationState;

	i32 MaxHitPoints, MaxManaPoints;
	i32 HitPoints, ManaPoints;

	r32 Rotation;

	sim_entity_collision_volume *Collision;

	r32 DistanceLimit;
	vec3 P;
	vec3 dP;

	entity_reference Fireball;
	
	particle_emitter_info *ParticlesInfo;
	point_light *PointLight;
};

inline bool32 
IsSet(sim_entity *Entity, u32 Flag)
{
	bool32 Result = Entity->Flags & Flag;

	return(Result);
}

inline void
AddFlags(sim_entity *Entity, u32 Flag)
{
	Entity->Flags |= Flag;
}

inline void
ClearFlags(sim_entity *Entity, u32 Flag)
{
	Entity->Flags &= ~Flag;
}

struct sim_region
{
	world *World;
	
	world_position Origin;
	
	rect3 UpdatableBounds;
	rect3 Bounds;

	u32 MaxEntityCount;
	u32 EntityCount;
	sim_entity *Entities;

	// NOTE(georgy): Must be a power of 2!
	entity_reference Hash[4096];
};