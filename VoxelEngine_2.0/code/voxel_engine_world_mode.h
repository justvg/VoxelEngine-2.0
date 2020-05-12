#pragma once

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

struct sim_entity;
struct stored_entity
{
	world_position P;
	sim_entity Sim;
};

struct hero
{
	stored_entity *Entity;

	bool32 Attack;
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

enum ubo_binding_point
{
	BindingPoint_Matrices,
	BindingPoint_UIMatrices,
	BindingPoint_DirectionalLightInfo,
	BindingPoint_ShadowsInfo,

	BindingPoint_Counts
};

struct game_mode_world
{
	world World;	

	camera Camera;

	stack_allocator WorldAllocator;
	stack_allocator FundamentalTypesAllocator;

	u32 LastStoredCollisionRule;
	pairwise_collision_rule CollisionRules[256];

	sim_entity_collision_volume *HeroCollision;
	sim_entity_collision_volume *FireballCollision;
	sim_entity_collision_volume *HeroSwordCollision;

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

	vec3 DirectionalLightDir, DirectionalLightColor;
#define CASCADES_COUNT 3
	GLuint ShadowMapFBO, ShadowMapsArray;
	u32 ShadowMapsWidth, ShadowMapsHeight;
	GLuint ShadowNoiseTexture;
	vec2 ShadowSamplesOffsets[16];

	u32 StoredEntityCount;
	stored_entity StoredEntities[10000];

	particle_generator ParticleGenerator;

	// TODO(georgy): Move this to temp state?
	font_id FontID;
	loaded_font *Font;

	GLuint UIQuadVAO, UIQuadVBO;
	GLuint UIGlyphVAO, UIGlyphVBO;

	GLuint CubeVAO, CubeVBO;
	GLuint QuadVAO, QuadVBO;
};

internal void AddCollisionRule(game_mode_world *GameState, u32 StorageIndexA, u32 StorageIndexB, bool32 CanCollide);