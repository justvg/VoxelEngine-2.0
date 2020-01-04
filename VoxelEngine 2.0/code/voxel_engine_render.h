#pragma once

struct shader
{
	u32 ID;
};


struct particle
{
	vec3 P, dP, ddP;
	r32 Scale;
	r32 LifeTime;
	r32 DistanceFromCameraSq;
};

struct particle_emitter_info
{
	r32 MaxLifeTime;
	bool32 Additive;
	u32 RowsInTextureAtlas;

	u32 SpawnInSecond;
	r32 Scale;
	vec3 StartPRanges;
	vec3 StartdPRanges;
	r32 dPY;
	vec3 StartddP;
};

#define MAX_PARTICLES_COUNT 512
struct particle_emitter
{
	particle Particles[MAX_PARTICLES_COUNT];
	u32 NextParticle;

	particle_emitter_info Info;

	asset_type_id TextureType;
};

struct block_particle
{
	world_position BaseP;
	vec3 P;
	vec3 dP;
	vec3 ddP;

	vec3 Color;

	r32 LifeTime;
};

#define MAX_BLOCK_PARTICLES_COUNT 512
struct block_particle_generator
{
	block_particle Particles[MAX_BLOCK_PARTICLES_COUNT];
	u32 NextParticle;

	shader Shader;
	GLuint VAO, VBO, SimPVBO, ColorVBO;
};

// 
// 
// 

struct debug_draw_info
{
	bool32 IsInitialized;

	shader Shader, AxesShader;
	GLuint CubeVAO, CubeVBO;
	GLuint AxesVAO, AxesVBO;
	GLuint LineVAO, LineVBO;

	GLuint SphereVAO, SphereVBO, SphereEBO;
	dynamic_array_u32 SphereIndices;
};