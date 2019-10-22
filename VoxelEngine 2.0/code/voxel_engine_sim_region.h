#pragma once

enum entity_type
{
	EntityType_Null,

	EntityType_Chunk, // NOTE(georgy): This is for handling collisions with chunk. 
					  //			   Should I make chunk a real entity?

	EntityType_Hero,
	EntityType_Fireball,
};

struct sim_entity;
struct entity_reference 
{
	u32 StorageIndex;
	sim_entity *SimPtr;
};

struct sim_entity
{
	u32 StorageIndex;

	entity_type Type;
	bool32 Updatable;
	// TODO(georgy): Change these to bit flags!
	bool32 Moveable;
	bool32 OnGround;
	bool32 NonSpatial;
	bool32 Collides;
	bool32 GravityAffected;

	i32 MaxHitPoints;
	i32 HitPoints;

	r32 Rotation;

	vec3 Dim;

	r32 DistanceLimit;
	vec3 P;
	vec3 dP;

	entity_reference Fireball;
};

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