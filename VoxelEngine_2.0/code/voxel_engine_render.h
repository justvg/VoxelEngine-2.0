#pragma once

struct shader
{
	u32 ID;
};

struct particle_emitter_info
{
	r32 MaxLifeTime;

	u32 SpawnInSecond;
	r32 Scale;
	vec3 StartPRanges;
	vec3 StartdPRanges;
	r32 dPY;
	vec3 StartddP;

	vec3 Color;
};

inline particle_emitter_info
DefaultParticleInfoWithColor(vec3 Color)
{
	particle_emitter_info Result = {};

	Result.MaxLifeTime = 1.5f;
	Result.Scale = 1.2f;
	Result.StartPRanges = vec3(1.0f, 0.0f, 1.0f);
	Result.StartdPRanges = vec3(1.0f, 0.0f, 1.0f);
	Result.dPY = 5.0f;
	Result.StartddP = vec3(0.0f, -9.8f, 0.0f);
	Result.Color = Color;

	return(Result);
}

struct particle
{
	world_position BaseP;
	vec3 P, dP, ddP;
	vec3 Color;
	r32 Scale;

	r32 LifeTime;
};

#define MAX_PARTICLES_COUNT 1024
struct particle_generator
{
	particle Particles[MAX_PARTICLES_COUNT];
	u32 NextParticle;

	shader Shader;
	GLuint VAO, VBO, SimPVBO, ColorVBO, ScaleVBO;
};

// 
// 
// 

#if VOXEL_ENGINE_INTERNAL

struct debug_draw_info
{
	bool32 IsInitialized;

	shader Shader;
	GLuint CubeVAO, CubeVBO;
	GLuint LineVAO, LineVBO;

	GLuint SphereVAO, SphereVBO, SphereEBO;
	dynamic_array_u32 SphereIndices;
};

#endif