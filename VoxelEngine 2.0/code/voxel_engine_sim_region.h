#pragma once

struct sim_entity
{
	u32 StorageIndex;

	vec3 P;
};

struct sim_region
{
	world *World;
	
	world_position Origin;

	u32 MaxEntityCount;
	u32 EntityCount;
	sim_entity *Entities;
};