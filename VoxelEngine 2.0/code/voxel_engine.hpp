#include "voxel_engine.h"
#include "voxel_engine_asset.hpp"
#include "voxel_engine_render.hpp"
#include "voxel_engine_world.hpp"
#include "voxel_engine_sim_region.hpp"

internal sim_entity_collision_volume *
MakeSimpleCollision(game_state *GameState, vec3 Dim)
{
	sim_entity_collision_volume *Result = PushStruct(&GameState->FundamentalTypesAllocator, sim_entity_collision_volume);

	Result->Dim = Dim;
	Result->OffsetP = vec3(0.0f, 0.5f*Dim.y(), 0.0f);

	InitializeDynamicArray(&Result->VerticesP);

	box Box = ConstructBoxDim(Dim);
	vec3 VolumeVertices[] = 
	{
			// Back face
			Box.Points[0],
			Box.Points[5],
			Box.Points[4],
			Box.Points[5],
			Box.Points[0],
			Box.Points[1],

			// Front face
			Box.Points[2],
			Box.Points[6],
			Box.Points[7],
			Box.Points[7],			
			Box.Points[3],
			Box.Points[2],

			// Left face
			Box.Points[3],
			Box.Points[1],
			Box.Points[0],
			Box.Points[0],			
			Box.Points[2],
			Box.Points[3],

			// Right face
			Box.Points[7],
			Box.Points[4],
			Box.Points[5],
			Box.Points[4],
			Box.Points[7],
			Box.Points[6],

			// Bottom face
			Box.Points[0],
			Box.Points[4],
			Box.Points[6],
			Box.Points[6],
			Box.Points[2],
			Box.Points[0],

			// Top face
			Box.Points[1],
			Box.Points[7],
			Box.Points[5],
			Box.Points[7],
			Box.Points[1],
			Box.Points[3],			
	};

	for(u32 VertexIndex = 0;
		VertexIndex < 36;
		VertexIndex++)
	{
		PushEntry(&Result->VerticesP, VolumeVertices[VertexIndex]);
	}

	return(Result);
}

internal void
AddCollisionRule(game_state *GameState, u32 StorageIndexA, u32 StorageIndexB, bool32 CanCollide)
{
	if(StorageIndexA > StorageIndexB)
	{
		u32 Temp = StorageIndexA;
		StorageIndexA = StorageIndexB;
		StorageIndexB = Temp;
	}

	for(u32 CollisionRuleIndex = GameState->LastStoredCollisionRule;
		CollisionRuleIndex < ArrayCount(GameState->CollisionRules);
		CollisionRuleIndex++)
	{
		pairwise_collision_rule *CollisionRule = GameState->CollisionRules + CollisionRuleIndex;
		if((CollisionRule->StorageIndexA == 0) &&
		   (CollisionRule->StorageIndexB == 0))
		{
			CollisionRule->StorageIndexA = StorageIndexA;
			CollisionRule->StorageIndexB = StorageIndexB;
			CollisionRule->CanCollide = CanCollide;
			GameState->LastStoredCollisionRule = CollisionRuleIndex;
			return;
		}
	}

	for(u32 CollisionRuleIndex = 0;
		CollisionRuleIndex < GameState->LastStoredCollisionRule;
		CollisionRuleIndex++)
	{
		pairwise_collision_rule *CollisionRule = GameState->CollisionRules + CollisionRuleIndex;
		if((CollisionRule->StorageIndexA == 0) &&
		   (CollisionRule->StorageIndexB == 0))
		{
			CollisionRule->StorageIndexA = StorageIndexA;
			CollisionRule->StorageIndexB = StorageIndexB;
			CollisionRule->CanCollide = CanCollide;
			GameState->LastStoredCollisionRule = CollisionRuleIndex;
			return;
		}
	}

	Assert(!"NO SPACE FOR NEW COLLISION RULE!");
}

internal void
ClearCollisionRulesFor(game_state *GameState, u32 StorageIndex)
{
	for(u32 CollisionRuleIndex = 0;
		CollisionRuleIndex < ArrayCount(GameState->CollisionRules);
		CollisionRuleIndex++)
	{
		pairwise_collision_rule *CollisionRule = GameState->CollisionRules + CollisionRuleIndex;
		if((CollisionRule->StorageIndexA == StorageIndex) ||
		   (CollisionRule->StorageIndexB == StorageIndex))
		{
			CollisionRule->StorageIndexA = CollisionRule->StorageIndexB = 0;
		}
	}
}

internal stored_entity *
TESTAddCube(game_state *GameState, world_position P, vec3 Dim, u32 VerticesCount, r32 *Vertices)
{
	Assert(GameState->StoredEntityCount < ArrayCount(GameState->StoredEntities));
	u32 EntityIndex = GameState->StoredEntityCount++;

	stored_entity *StoredEntity = GameState->StoredEntities + EntityIndex;
	*StoredEntity = {};
	StoredEntity->Sim.StorageIndex = EntityIndex;
	StoredEntity->Sim.Type = (entity_type)10000;
	StoredEntity->Sim.Collision = GameState->TESTCubeCollision;
	StoredEntity->P = InvalidPosition();
	AddFlags(&StoredEntity->Sim, EntityFlag_Moveable | EntityFlag_Collides);

	StoredEntity->Sim.MaxHitPoints = 30;
	StoredEntity->Sim.HitPoints = 30;

	ChangeEntityLocation(&GameState->World, &GameState->WorldAllocator, EntityIndex, StoredEntity, P);

	return(StoredEntity);
}

internal void
AddParticlesToEntity(game_state *GameState, stored_entity *Entity, particle_emitter_info Info, asset_type_id TextureType)
{
	// TODO(georgy): Allow multiple particle systems for an entity
	if(!Entity->Sim.Particles)
	{
		// TODO(georgy): Do I want to use _world_ allocator here?
		Entity->Sim.Particles = PushStruct(&GameState->FundamentalTypesAllocator, particle_emitter);
		particle_emitter *EntityParticles = Entity->Sim.Particles;
		EntityParticles->Info = Info;
		EntityParticles->TextureType = TextureType;
	}
}

struct add_stored_entity_result
{
	stored_entity *StoredEntity;
	u32 StorageIndex;
};
inline add_stored_entity_result
AddStoredEntity(game_state *GameState, entity_type Type, world_position P)
{
	add_stored_entity_result Result;
	
	Assert(GameState->StoredEntityCount < ArrayCount(GameState->StoredEntities));
	Result.StorageIndex = GameState->StoredEntityCount++;

	Result.StoredEntity = GameState->StoredEntities + Result.StorageIndex;
	*Result.StoredEntity = {};
	Result.StoredEntity->Sim.StorageIndex = Result.StorageIndex;
	Result.StoredEntity->Sim.Type = Type;
	Result.StoredEntity->P = InvalidPosition();

	ChangeEntityLocation(&GameState->World, &GameState->WorldAllocator, Result.StorageIndex, Result.StoredEntity, P);

	return(Result);
}

internal u32
AddFireball(game_state *GameState)
{
	add_stored_entity_result Entity = AddStoredEntity(GameState, EntityType_Fireball, InvalidPosition());

	AddFlags(&Entity.StoredEntity->Sim, EntityFlag_Moveable | EntityFlag_NonSpatial | EntityFlag_Collides);
	Entity.StoredEntity->Sim.Rotation = 0.0f;
	Entity.StoredEntity->Sim.Collision = GameState->FireballCollision;

	return(Entity.StorageIndex);
}

internal stored_entity *
AddHero(game_state *GameState, world_position P)
{
	add_stored_entity_result Entity = AddStoredEntity(GameState, EntityType_Hero, P);

	AddFlags(&Entity.StoredEntity->Sim, EntityFlag_Moveable | EntityFlag_Collides | EntityFlag_GravityAffected);
	Entity.StoredEntity->Sim.Rotation = 0.0f;
	Entity.StoredEntity->Sim.Collision = GameState->HeroCollision;
	Entity.StoredEntity->Sim.Fireball.StorageIndex = AddFireball(GameState);

	particle_emitter_info FireballParticleEmitterInfo = {};
	FireballParticleEmitterInfo.MaxLifeTime = 2.0f;
	FireballParticleEmitterInfo.RowsInTextureAtlas = 4;
	FireballParticleEmitterInfo.SpawnInSecond = 60;
	FireballParticleEmitterInfo.Scale = 0.25f;
	FireballParticleEmitterInfo.StartPRanges = vec3(0.5f, 0.0f, 0.5f);
	FireballParticleEmitterInfo.StartdPRanges = vec3(0.5f, 0.0f, 0.5f);
	FireballParticleEmitterInfo.dPY = 7.0f;
	FireballParticleEmitterInfo.StartddP = vec3(0.0f, -9.8f, 0.0f);
	AddParticlesToEntity(GameState, &GameState->StoredEntities[Entity.StoredEntity->Sim.Fireball.StorageIndex], 
						 FireballParticleEmitterInfo, AssetType_Fire);

	return(Entity.StoredEntity);
}

internal void
AddTree(game_state *GameState, world_position P)
{
	add_stored_entity_result Entity = AddStoredEntity(GameState, EntityType_Tree, P);

	AddFlags(&Entity.StoredEntity->Sim, EntityFlag_Collides);
	Entity.StoredEntity->Sim.Collision = GameState->TreeCollision;
}

internal void
GameUpdate(game_memory *Memory, game_input *Input, bool32 GameProfilingPause, int BufferWidth, int BufferHeight)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!GameState->IsInitialized)
	{
		PlatformAddEntry = Memory->PlatformAddEntry;
		PlatformCompleteAllWork = Memory->PlatformCompleteAllWork;
		PlatformReadEntireFile = Memory->PlatformReadEntireFile;
		PlatformAllocateMemory = Memory->PlatformAllocateMemory;
		PlatformFreeMemory = Memory->PlatformFreeMemory;
		PlatformOutputDebugString = Memory->PlatformOutputDebugString;
		PlatformLoadCodepointBitmap = Memory->PlatformLoadCodepointBitmap;
		PlatformBeginFont = Memory->PlatformBeginFont;
		PlatformLoadCodepointBitmap = Memory->PlatformLoadCodepointBitmap;
		PlatformEndFont = Memory->PlatformEndFont;

		InitializeStackAllocator(&GameState->WorldAllocator, Memory->PermanentStorageSize - sizeof(game_state),
															 (u8 *)Memory->PermanentStorage + sizeof(game_state));
		SubAllocator(&GameState->FundamentalTypesAllocator, &GameState->WorldAllocator, Megabytes(64));

		GameState->StoredEntityCount = 0;

		GameState->Camera = {};
		GameState->Camera.DistanceFromHero = 9.0f;
		GameState->Camera.RotSensetivity = 0.1f;
		GameState->Camera.NearDistance = 0.1f;
		GameState->Camera.FarDistance = 120.0f;
		GameState->Camera.FoV = 45.0f;
		// TODO(georgy): Get this from hero head model or smth
		// NOTE(georgy): This is hero head centre offset from hero feet pos. 
		// 				 We use this to offset all object in view matrix. So Camera->OffsetFromHero is offset from hero head centre  
		GameState->Camera.TargetOffset = vec3(0.0f, 0.680000007f + 0.1f, 0.0f);
		GameState->Camera.DEBUGFront = vec3(0.0f, 0.0f, -1.0f);

		InitializeWorld(&GameState->World);

		GameState->HeroCollision = MakeSimpleCollision(GameState, vec3(0.54f, 0.54f, 0.48f));
		GameState->FireballCollision = MakeSimpleCollision(GameState, vec3(0.25f, 0.25f, 0.25f));
		GameState->TreeCollision = MakeSimpleCollision(GameState, vec3(0.5f, 0.5f, 0.5f));
		GameState->TESTCubeCollision = MakeSimpleCollision(GameState, vec3(1.0f, 1.0f, 1.0f));

		CompileShader(&GameState->CharacterShader, "data/shaders/CharacterVS.glsl", "data/shaders/CharacterFS.glsl");
		CompileShader(&GameState->WorldShader, "data/shaders/WorldVS.glsl", "data/shaders/WorldFS.glsl");
		CompileShader(&GameState->HitpointsShader, "data/shaders/HitpointsVS.glsl", "data/shaders/HitpointsFS.glsl");
		CompileShader(&GameState->BillboardShader, "data/shaders/BillboardVS.glsl", "data/shaders/BillboardFS.glsl");
		CompileShader(&GameState->BlockParticleShader, "data/shaders/BlockParticleVS.glsl", "data/shaders/BlockParticleFS.glsl");
		CompileShader(&GameState->CharacterDepthShader, "data/shaders/CharacterDepthVS.glsl", "data/shaders/EmptyFS.glsl");
		CompileShader(&GameState->WorldDepthShader, "data/shaders/WorldDepthVS.glsl", "data/shaders/EmptyFS.glsl");
		CompileShader(&GameState->BlockParticleDepthShader, "data/shaders/BlockParticleDepthVS.glsl", "data/shaders/EmptyFS.glsl");
		CompileShader(&GameState->FramebufferScreenShader, "data/shaders/FramebufferScreenVS.glsl", "data/shaders/FramebufferScreenFS.glsl");

		block_particle_generator *BlockParticleGenerator = &GameState->BlockParticleGenerator;
		r32 BlockParticleVertices[] = 
		{
			0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
			-0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
			0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 
			-0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
			0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
			-0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 

			-0.1f, -0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  
			0.1f, -0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  
			0.1f,  0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  
			0.1f,  0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  
			-0.1f,  0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  
			-0.1f, -0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  

			-0.1f,  0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 
			-0.1f,  0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 
			-0.1f, -0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 
			-0.1f, -0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 
			-0.1f, -0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 
			-0.1f,  0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 

			0.1f,  0.1f, -0.1f,  1.0f,  0.0f,  0.0f,
			0.1f,  0.1f,  0.1f,  1.0f,  0.0f,  0.0f,
			0.1f, -0.1f, -0.1f,  1.0f,  0.0f,  0.0f, 
			0.1f, -0.1f,  0.1f,  1.0f,  0.0f,  0.0f,
			0.1f, -0.1f, -0.1f,  1.0f,  0.0f,  0.0f,
			0.1f,  0.1f,  0.1f,  1.0f,  0.0f,  0.0f, 

			-0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 
			0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 
			0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 
			0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 
			-0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 
			-0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 

			0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f,
			-0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f,
			0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f, 
			-0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f,
			0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f,
			-0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f
		};
		glGenVertexArrays(1, &BlockParticleGenerator->VAO);
		glGenBuffers(1, &BlockParticleGenerator->VBO);
		glGenBuffers(1, &BlockParticleGenerator->SimPVBO);
		glGenBuffers(1, &BlockParticleGenerator->ColorVBO);
		glBindVertexArray(BlockParticleGenerator->VAO);
		glBindBuffer(GL_ARRAY_BUFFER, BlockParticleGenerator->VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(BlockParticleVertices), BlockParticleVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(r32), (void *)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(r32), (void *)(3*sizeof(r32)));
		glBindVertexArray(0);

		r32 CubeVertices[] = {
			// Back face
			-0.5f, -0.5f, -0.5f,
			0.5f,  0.5f, -0.5f,
			0.5f, -0.5f, -0.5f,
			0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			// Front face
			-0.5f, -0.5f,  0.5f,
			0.5f, -0.5f,  0.5f,
			0.5f,  0.5f,  0.5f,
			0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			// Left face
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			// Right face
			0.5f,  0.5f,  0.5f,
			0.5f, -0.5f, -0.5f,
			0.5f,  0.5f, -0.5f,
			0.5f, -0.5f, -0.5f,
			0.5f,  0.5f,  0.5f,
			0.5f, -0.5f,  0.5f,
			// Bottom face
			-0.5f, -0.5f, -0.5f,
			0.5f, -0.5f, -0.5f,
			0.5f, -0.5f,  0.5f,
			0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f, -0.5f,
			// Top face
			-0.5f,  0.5f, -0.5f,
			0.5f,  0.5f , 0.5f,
			0.5f,  0.5f, -0.5f,
			0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f,  0.5f,
		};
		glGenVertexArrays(1, &GameState->CubeVAO);
		glGenBuffers(1, &GameState->CubeVBO);
		glBindVertexArray(GameState->CubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, GameState->CubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(CubeVertices), CubeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(r32), (void *)0);
		glBindVertexArray(0);

		r32 QuadVertices[] = 
		{
			-0.5f, 0.5f, 0.0f,
			-0.5f, -0.5f, 0.0f,
			0.5f, 0.5f, 0.0f,
			0.5f, -0.5f, 0.0f
		};
		glGenVertexArrays(1, &GameState->QuadVAO);
		glGenBuffers(1, &GameState->QuadVBO);
		glBindVertexArray(GameState->QuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, GameState->QuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), QuadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(r32), (void *)0);
		glBindVertexArray(0);

		glGenVertexArrays(1, &GameState->ParticleVAO);
		glGenBuffers(1, &GameState->ParticleVBO);
		glGenBuffers(1, &GameState->ParticlePVBO);
		glGenBuffers(1, &GameState->ParticleOffsetVBO);
		glGenBuffers(1, &GameState->ParticleScaleVBO);
		glBindVertexArray(GameState->ParticleVAO);
		glBindBuffer(GL_ARRAY_BUFFER, GameState->ParticleVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), QuadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(r32), (void *)0);
		glBindVertexArray(0);

		// NOTE(georgy): Reserve slot 0
		AddStoredEntity(GameState, EntityType_Null, InvalidPosition());

		world_position TestP = {};
		TestP.ChunkX = 0;
		TestP.ChunkY = 0;
		TestP.ChunkZ = 0;
		TestP.Offset = vec3(0.0f, 4.0f, 3.0f);
		particle_emitter_info CubeParticleEmitterInfo = {};
		CubeParticleEmitterInfo.Additive = true;
		CubeParticleEmitterInfo.MaxLifeTime = 2.0f;
		CubeParticleEmitterInfo.RowsInTextureAtlas = 4;
		CubeParticleEmitterInfo.SpawnInSecond = 120;
		CubeParticleEmitterInfo.Scale = 0.3f;
		CubeParticleEmitterInfo.StartPRanges = vec3(0.12f, 0.0f, 0.12f);
		CubeParticleEmitterInfo.StartdPRanges = vec3(0.5f, 0.0f, 0.5f);
		CubeParticleEmitterInfo.dPY = 7.0f;
		CubeParticleEmitterInfo.StartddP = vec3(0.0f, -9.8f, 0.0f);
		AddParticlesToEntity(GameState, TESTAddCube(GameState, TestP, vec3(1.0f, 1.0f, 1.0f), 36, CubeVertices),
							 CubeParticleEmitterInfo, AssetType_Cosmic);

		world_position HeroP = {};
		HeroP.ChunkX = 0;
		HeroP.ChunkY = MAX_CHUNKS_Y;
		HeroP.ChunkZ = 0;
		HeroP.Offset = vec3(0.3f, 5.0f, 3.0f);
		GameState->Hero.Entity = AddHero(GameState, HeroP);
		particle_emitter_info ParticleEmitterInfo = {};
		ParticleEmitterInfo.MaxLifeTime = 2.0f;
		ParticleEmitterInfo.RowsInTextureAtlas = 8;
		ParticleEmitterInfo.SpawnInSecond = 120;
		ParticleEmitterInfo.Scale = 1.0f;
		ParticleEmitterInfo.StartPRanges = vec3(0.0f, 0.0f, 0.0f);
		ParticleEmitterInfo.StartdPRanges = vec3(0.5f, 0.0f, 0.5f);
		ParticleEmitterInfo.dPY = 2.0f;
		// AddParticlesToEntity(GameState, GameState->Hero.Entity, ParticleEmitterInfo, AssetType_Smoke);

		world_position TestTreeP = {};
		TestTreeP.ChunkX = 0;
		TestTreeP.ChunkY = MAX_CHUNKS_Y;
		TestTreeP.ChunkZ = 0;
		TestTreeP.Offset = vec3(0.0f, 2.0f, 3.0f);
		AddTree(GameState, TestTreeP);

		GameState->DirectionalLightDir = -vec3(3.0f, 5.0f, 4.0f);

		glGenFramebuffers(1, &GameState->ShadowMapFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, GameState->ShadowMapFBO);
		GameState->ShadowMapsWidth = GameState->ShadowMapsHeight = 2000;

		glGenTextures(1, &GameState->ShadowMapsArray);
		glBindTexture(GL_TEXTURE_2D_ARRAY, GameState->ShadowMapsArray);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, GameState->ShadowMapsWidth, GameState->ShadowMapsHeight, 
					 CASCADES_COUNT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GameState->ShadowMapsArray, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		InitializeDefaultAnimations(GameState->CharacterAnimations);

		GameState->IsInitialized = true;
	}

	Assert(sizeof(temp_state) <= Memory->TemporaryStorageSize);
	temp_state *TempState = (temp_state *)Memory->TemporaryStorage;
	if (!TempState->IsInitialized)
	{
		InitializeStackAllocator(&TempState->Allocator, Memory->TemporaryStorageSize - sizeof(temp_state),
														(u8 *)Memory->TemporaryStorage + sizeof(temp_state));

		TempState->JobSystemQueue = Memory->JobSystemQueue;
		// TempState->GameAssets = AllocateGameAssets(TempState, &TempState->Allocator, 60000);
		TempState->GameAssets = AllocateGameAssets(TempState, &TempState->Allocator, Megabytes(64));

		TempState->IsInitialized = true;
	}

	camera *Camera = &GameState->Camera;
	Camera->Pitch -= Input->MouseYDisplacement*Camera->RotSensetivity;
	Camera->Head += Input->MouseXDisplacement*Camera->RotSensetivity;

	Camera->Pitch = Camera->Pitch > 89.0f ? 89.0f : Camera->Pitch;
	Camera->Pitch = Camera->Pitch < -89.0f ? -89.0f : Camera->Pitch;

	DEBUG_IF(DebugCamera)
	{
		r32 CameraTargetDirX = Sin(DEG2RAD(Camera->Head))*Cos(DEG2RAD(Camera->Pitch));
		r32 CameraTargetDirY = Sin(DEG2RAD(Camera->Pitch));
		r32 CameraTargetDirZ = -Cos(DEG2RAD(Camera->Head))*Cos(DEG2RAD(Camera->Pitch));
		Camera->DEBUGFront = Normalize(vec3(CameraTargetDirX, CameraTargetDirY, CameraTargetDirZ));
	}
	else
	{
		r32 PitchRadians = DEG2RAD(Camera->Pitch);
		r32 HeadRadians = DEG2RAD(Camera->Head);
		r32 HorizontalDistanceFromHero = Camera->DistanceFromHero*Cos(-PitchRadians);
		r32 XOffsetFromHero = -HorizontalDistanceFromHero * Sin(HeadRadians);
		r32 YOffsetFromHero = Camera->DistanceFromHero * Sin(-PitchRadians);
		r32 ZOffsetFromHero = HorizontalDistanceFromHero * Cos(HeadRadians);
		vec3 NewTargetOffsetFromHero = vec3(XOffsetFromHero, YOffsetFromHero, ZOffsetFromHero);
		Camera->OffsetFromHero = Camera->DistanceFromHero * Normalize(Lerp(Camera->LastOffsetFromHero, NewTargetOffsetFromHero, 8.0f*Input->dt));
		Camera->LastOffsetFromHero = Camera->OffsetFromHero;
	}

	Camera->AspectRatio = (r32)BufferWidth/(r32)BufferHeight;

	vec3 Forward;
	if(!DebugCamera)
	{
		Forward = Normalize(vec3(-Camera->OffsetFromHero.x(), 0.0f, -Camera->OffsetFromHero.z()));
	}
	else
	{
		Forward = Camera->DEBUGFront;
	}
	vec3 Right = Normalize(Cross(Forward, vec3(0.0f, 1.0f, 0.0f)));
	r32 Theta = -RAD2DEG(ATan2(Forward.z(), Forward.x())) + 90.0f;
	GameState->Hero.ddP = vec3(0.0f, 0.0f, 0.0f);

	if(!DebugCamera)
	{
		if(!GameProfilingPause)
		{
			if (Input->MoveForward.EndedDown)
			{
				GameState->Hero.ddP += Forward;
				GameState->Hero.AdditionalRotation = Theta;
			}
			if (Input->MoveBack.EndedDown)
			{
				GameState->Hero.ddP -= Forward;
				GameState->Hero.AdditionalRotation = Theta - 180.0f;
			}
			if (Input->MoveRight.EndedDown)
			{
				GameState->Hero.ddP += Right;
				GameState->Hero.AdditionalRotation = Theta - 90.0f;
			}
			if (Input->MoveLeft.EndedDown)
			{
				GameState->Hero.ddP -= Right;
				GameState->Hero.AdditionalRotation = Theta + 90.0f;
			}

			GameState->Hero.dY = 0.0f;
			if(Input->MoveUp.EndedDown)
			{
				GameState->Hero.dY = 3.0f;
			}

			if (Input->MoveForward.EndedDown && Input->MoveRight.EndedDown)
			{
				GameState->Hero.AdditionalRotation = Theta - 45.0f;
			}
			if (Input->MoveForward.EndedDown && Input->MoveLeft.EndedDown)
			{
				GameState->Hero.AdditionalRotation = Theta + 45.0f;
			}
			if (Input->MoveBack.EndedDown && Input->MoveRight.EndedDown)
			{
				GameState->Hero.AdditionalRotation = Theta - 135.0f;
			}
			if (Input->MoveBack.EndedDown && Input->MoveLeft.EndedDown)
			{
				GameState->Hero.AdditionalRotation = Theta + 135.0f;
			}

			GameState->Hero.Fireball = WasDown(&Input->MouseRight);
			if(GameState->Hero.Fireball)
			{
				GameState->Hero.AdditionalRotation = Theta;
			}
		}
	}
	else
	{
		if (Input->MoveForward.EndedDown)
		{
			Camera->DEBUGP += Forward;
		}
		if (Input->MoveBack.EndedDown)
		{
			Camera->DEBUGP  -= Forward;
		}
		if (Input->MoveRight.EndedDown)
		{
			Camera->DEBUGP  += Right;
		}
		if (Input->MoveLeft.EndedDown)
		{
			Camera->DEBUGP  -= Right;
		}
	}


#if VOXEL_ENGINE_INTERNAL
	if(DEBUGGlobalPlaybackInfo.RefreshNow)
	{
		for(u32 ChunkIndex = 0;
			ChunkIndex < DEBUGGlobalPlaybackInfo.ChunksUnloadedDuringRecordPhaseCount;
			ChunkIndex++)
		{
			chunk *Chunk = DEBUGGlobalPlaybackInfo.ChunksUnloadedDuringRecordPhase[ChunkIndex];
			if(Chunk->IsSetupBlocks)
			{
				Chunk->IsNotEmpty = CheckChunkEmptiness(Chunk);

				if(Chunk->IsNotEmpty)
				{
					if(Chunk->IsFullySetup)
					{
						InitializeDynamicArray(&Chunk->VerticesP);
						InitializeDynamicArray(&Chunk->VerticesNormals);
						InitializeDynamicArray(&Chunk->VerticesColors);

						GenerateChunkVertices(&GameState->World, Chunk);

						if(Chunk->IsLoaded)
						{
							LoadChunk(Chunk);
						}
					}
				}
			}
		}

		for(u32 ChunkIndex = 0;
			ChunkIndex < DEBUGGlobalPlaybackInfo.ChunksModifiedDuringRecordPhaseCount;
			ChunkIndex++)
		{
			chunk *Chunk = DEBUGGlobalPlaybackInfo.ChunksModifiedDuringRecordPhase[ChunkIndex];
			UpdateChunk(&GameState->World, Chunk);
		}

		DEBUGGlobalPlaybackInfo.RefreshNow = false;
	}
#endif


	temporary_memory WorldConstructionAndRenderMemory = BeginTemporaryMemory(&TempState->Allocator);

	DEBUG_VARIABLE(r32, SimBoundsRadius);
	rect3 SimRegionUpdatableBounds = RectMinMax(vec3(-SimBoundsRadius, -20.0f, -SimBoundsRadius), 
												vec3(SimBoundsRadius, 20.0f, SimBoundsRadius));
	sim_region *SimRegion = BeginSimulation(GameState, GameState->Hero.Entity->P, 
											SimRegionUpdatableBounds, &TempState->Allocator, Input->dt);	

	Camera->RotationMatrix = RotationMatrixFromDirection(Camera->OffsetFromHero);
#if !defined(VOXEL_ENGINE_DEBUG_BUILD)
	CameraCollisionDetection(&GameState->World, Camera, &GameState->Hero.Entity->P);
#endif
	SetupChunksBlocks(&GameState->World, &GameState->WorldAllocator, TempState);								
	SetupChunksVertices(&GameState->World, TempState);
	LoadChunks(&GameState->World);

	// NOTE(georgy): Entity simulations
	for(u32 EntityIndex = 0;
		EntityIndex < SimRegion->EntityCount;
		EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;
		r32 dt = Input->dt;
		if(Entity->Updatable)
		{
			move_spec MoveSpec = {};
			switch(Entity->Type)
			{
				case EntityType_Hero:
				{
					MoveSpec.ddP = GameState->Hero.ddP;
					MoveSpec.Speed = 8.0f;
					MoveSpec.Drag = 2.0f;
					if(GameState->Hero.dY && IsSet(Entity, EntityFlag_OnGround))
					{
						Entity->dP.SetY(GameState->Hero.dY);
					}

					Entity->Rotation = GameState->Hero.AdditionalRotation;

					if(GameState->Hero.Fireball)
					{
						sim_entity *Fireball = Entity->Fireball.SimPtr;
						if(Fireball && IsSet(Fireball, EntityFlag_NonSpatial))
						{
							ClearFlags(Fireball, EntityFlag_NonSpatial);
							Fireball->DistanceLimit = 8.0f;
							Fireball->P = Entity->P + vec3(0.0f, 0.5f, 0.0f);
							Fireball->dP = vec3(Entity->dP.x(), 0.0f, Entity->dP.z()) + 5.0f*Forward;
						}
					}

					r32 AnimationTimeStep = 0.0f;
					character_animation_type DesiredAnimation = CharacterAnimation_Idle;
					if(Length(vec3(Entity->dP.x(), 0.0f, Entity->dP.z())) > 0.12f)
					{
						DesiredAnimation = CharacterAnimation_Run;
					}
					if(!IsSet(Entity, EntityFlag_OnGround))
					{
						DesiredAnimation = CharacterAnimation_Jump;
					}

					if(Entity->AnimationState.Type != DesiredAnimation)
					{
						Entity->AnimationState.Type = DesiredAnimation;
						Entity->AnimationState.Time = 0.0f;
					}
					else
					{
						AnimationTimeStep = dt;
						if(Entity->AnimationState.Type == CharacterAnimation_Run)
						{
							AnimationTimeStep = 2.0f*Length(vec3(Entity->dP.x(), 0.0f, Entity->dP.z()))*dt;
						}
					}

					Entity->AnimationState.Time += AnimationTimeStep;
				} break;

				case EntityType_Fireball:
				{
					if(Entity->DistanceLimit == 0.0f)
					{
						MakeEntityNonSpatial(Entity);
						ClearCollisionRulesFor(GameState, Entity->StorageIndex);
					}
				} break;

				case 10000:
				{
					//Drag = 1.0f;

					vec3 DisplacementToHero = -Entity->P;
					r32 Distance = Length(DisplacementToHero);
					if(Distance > 2.0f)
					{
						//ddP = Normalize(DisplacementToHero);
					}
				} break;
			}

			if(Entity->Particles)
			{
				particle_emitter *EntityParticles = Entity->Particles;
				UpdateParticles(EntityParticles, Camera, Entity->P, dt);
				SpawnParticles(EntityParticles, Camera, Entity->P, dt);
				if(!EntityParticles->Info.Additive)
				{
					SortParticles(EntityParticles->Particles, ArrayCount(EntityParticles->Particles));
				}
			}

			if(IsSet(Entity, EntityFlag_Moveable) && !IsSet(Entity, EntityFlag_NonSpatial))
			{
				MoveEntity(GameState, SimRegion, Entity, MoveSpec, dt, false);
				if(IsSet(Entity, EntityFlag_GravityAffected))
				{
					MoveEntity(GameState, SimRegion, Entity, MoveSpec, dt, true);
				}
			}
		}
	}
	BlockParticlesUpdate(&GameState->BlockParticleGenerator, Input->dt);
	
	
	// NOTE(georgy): Directional shadow map rendering
	r32 CascadesDistances[CASCADES_COUNT + 1] = {Camera->NearDistance, 25.0f, 65.0f, Camera->FarDistance};
	mat4 LightSpaceMatrices[CASCADES_COUNT];
	DEBUG_IF(DebugRenderShadows)
	{
		glViewport(0, 0, GameState->ShadowMapsWidth, GameState->ShadowMapsHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, GameState->ShadowMapFBO);
		for(u32 CascadeIndex = 0;
			CascadeIndex < CASCADES_COUNT;
			CascadeIndex++)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GameState->ShadowMapsArray, 0, CascadeIndex);
			glClear(GL_DEPTH_BUFFER_BIT);

			vec3 CameraRight = vec3(Camera->RotationMatrix.FirstColumn.x(), Camera->RotationMatrix.SecondColumn.x(), Camera->RotationMatrix.ThirdColumn.x());;
			vec3 CameraUp = vec3(Camera->RotationMatrix.FirstColumn.y(), Camera->RotationMatrix.SecondColumn.y(), Camera->RotationMatrix.ThirdColumn.y());;
			vec3 CameraOut = -vec3(Camera->RotationMatrix.FirstColumn.z(), Camera->RotationMatrix.SecondColumn.z(), Camera->RotationMatrix.ThirdColumn.z());

			r32 NearPlaneHalfHeight = Tan(0.5f*DEG2RAD(Camera->FoV))*CascadesDistances[CascadeIndex];
			r32 NearPlaneHalfWidth = NearPlaneHalfHeight*Camera->AspectRatio;
			r32 FarPlaneHalfHeight = Tan(0.5f*DEG2RAD(Camera->FoV))*CascadesDistances[CascadeIndex + 1];
			r32 FarPlaneHalfWidth = FarPlaneHalfHeight*Camera->AspectRatio;

			vec4 Cascade1[8];
			Cascade1[0] = vec4(CameraRight*NearPlaneHalfWidth + CameraUp*NearPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex], 1.0f);
			Cascade1[1] = vec4(CameraRight*NearPlaneHalfWidth - CameraUp*NearPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex], 1.0f);
			Cascade1[2] = vec4(-CameraRight*NearPlaneHalfWidth + CameraUp*NearPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex], 1.0f);
			Cascade1[3] = vec4(-CameraRight*NearPlaneHalfWidth - CameraUp*NearPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex], 1.0f);
			Cascade1[4] = vec4(CameraRight*FarPlaneHalfWidth + CameraUp*FarPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex + 1], 1.0f);
			Cascade1[5] = vec4(CameraRight*FarPlaneHalfWidth - CameraUp*FarPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex + 1], 1.0f);
			Cascade1[6] = vec4(-CameraRight*FarPlaneHalfWidth + CameraUp*FarPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex + 1], 1.0f);
			Cascade1[7] = vec4(-CameraRight*FarPlaneHalfWidth - CameraUp*FarPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex + 1], 1.0f);

			for(u32 PointIndex = 0;
				PointIndex < ArrayCount(Cascade1);
				PointIndex++)
			{
				Cascade1[PointIndex] = Cascade1[PointIndex] + vec4(Camera->OffsetFromHero + Camera->TargetOffset, 0.0f);
			}

			// mat4 LightView = LookAt(vec3(0.0f, 0.0f, 0.0f), -vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f));
			mat4 LightView = LookAt(vec3(0.0f, 0.0f, 0.0f), GameState->DirectionalLightDir);
			for(u32 PointIndex = 0;
				PointIndex < ArrayCount(Cascade1);
				PointIndex++)
			{
				Cascade1[PointIndex] = LightView * Cascade1[PointIndex];
			}
			
			rect3 Cascade1AABB;
			Cascade1AABB.Min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
			Cascade1AABB.Max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			for(u32 PointIndex = 0;
				PointIndex < ArrayCount(Cascade1);
				PointIndex++)
			{
				Cascade1AABB.Min = Min(Cascade1AABB.Min, vec3(Cascade1[PointIndex].m));
				Cascade1AABB.Max = Max(Cascade1AABB.Max, vec3(Cascade1[PointIndex].m));
			}
			Cascade1AABB = AddRadiusTo(Cascade1AABB, vec3(1.5f, 1.5f, 1.5f));
			mat4 LightProjection = Ortho(Cascade1AABB.Min.y(), Cascade1AABB.Max.y(), 
										Cascade1AABB.Min.x(), Cascade1AABB.Max.x(),
										-Cascade1AABB.Max.z() - 70.0f, -Cascade1AABB.Min.z());

			LightSpaceMatrices[CascadeIndex] = LightProjection * LightView;
			UseShader(GameState->WorldDepthShader);
			SetMat4(GameState->WorldDepthShader, "ViewProjection", LightSpaceMatrices[CascadeIndex]);
			UseShader(GameState->CharacterDepthShader);
			SetMat4(GameState->CharacterDepthShader, "ViewProjection", LightSpaceMatrices[CascadeIndex]);
			UseShader(GameState->BlockParticleDepthShader);
			SetMat4(GameState->BlockParticleDepthShader, "ViewProjection", LightSpaceMatrices[CascadeIndex]);

			RenderChunks(&GameState->World, GameState->WorldDepthShader, LightSpaceMatrices[CascadeIndex]);

			RenderEntities(GameState, TempState, SimRegion, GameState->WorldDepthShader, 
						GameState->CharacterDepthShader, GameState->BillboardShader, 
						Right);
			RenderBlockParticles(&GameState->BlockParticleGenerator, &GameState->World, &TempState->Allocator, 
								GameState->BlockParticleDepthShader, GameState->Hero.Entity->P);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// NOTE(georgy): World and entities rendering
	glViewport(0, 0, BufferWidth, BufferHeight);

	mat4 View = Camera->RotationMatrix * 
				Translate(-Camera->OffsetFromHero - Camera->TargetOffset);
	mat4 Projection = Perspective(Camera->FoV, Camera->AspectRatio, Camera->NearDistance, Camera->FarDistance);
	mat4 ViewProjection = Projection * View;
	shader Shaders3D[] = { GameState->WorldShader, GameState->CharacterShader,
						   GameState->HitpointsShader, GameState->BillboardShader, GameState->BlockParticleShader};
	if(!DebugCamera)
	{
		Initialize3DTransforms(Shaders3D, ArrayCount(Shaders3D), ViewProjection);
		GlobalViewProjection = ViewProjection;
	}
	else
	{
		mat4 DEBUGCameraView = LookAt(Camera->DEBUGP, Camera->DEBUGP + Camera->DEBUGFront);
		mat4 DEBUGCameraViewProjection = Projection * DEBUGCameraView;
		Initialize3DTransforms(Shaders3D, ArrayCount(Shaders3D), DEBUGCameraViewProjection);
		GlobalViewProjection = DEBUGCameraViewProjection;
	}
	

	UseShader(GameState->WorldShader);
	// NOTE(georgy): This is for the situation when we use debug camera. Even if we use debug camera,
	//  			 we want Output.ClipSpacePosZ to be as it is from our default camera, not debug one
	SetMat4(GameState->WorldShader, "ViewProjectionForClipSpacePosZ", ViewProjection);
	SetMat4Array(GameState->WorldShader, "LightSpaceMatrices", CASCADES_COUNT, LightSpaceMatrices);
	SetFloatArray(GameState->WorldShader, "CascadesDistances", CASCADES_COUNT + 1, CascadesDistances);
	SetVec3(GameState->WorldShader, "DirectionalLightDir", GameState->DirectionalLightDir);
	SetInt(GameState->WorldShader, "ShadowsEnabled", DebugRenderShadows);
	SetInt(GameState->WorldShader, "ShadowMaps", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, GameState->ShadowMapsArray);
	
	UseShader(GameState->CharacterShader);
	SetMat4(GameState->CharacterShader, "ViewProjectionForClipSpacePosZ", ViewProjection);
	SetMat4Array(GameState->CharacterShader, "LightSpaceMatrices", CASCADES_COUNT, LightSpaceMatrices);
	SetFloatArray(GameState->CharacterShader, "CascadesDistances", CASCADES_COUNT + 1, CascadesDistances);
	SetVec3(GameState->CharacterShader, "DirectionalLightDir", GameState->DirectionalLightDir);
	SetInt(GameState->CharacterShader, "ShadowsEnabled", DebugRenderShadows);
	SetInt(GameState->CharacterShader, "ShadowMaps", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, GameState->ShadowMapsArray);

	UseShader(GameState->BlockParticleShader);
	SetMat4(GameState->BlockParticleShader, "ViewProjectionForClipSpacePosZ", ViewProjection);
	SetMat4Array(GameState->BlockParticleShader, "LightSpaceMatrices", CASCADES_COUNT, LightSpaceMatrices);
	SetFloatArray(GameState->BlockParticleShader, "CascadesDistances", CASCADES_COUNT + 1, CascadesDistances);
	SetVec3(GameState->BlockParticleShader, "DirectionalLightDir", GameState->DirectionalLightDir);
	SetInt(GameState->BlockParticleShader, "ShadowsEnabled", DebugRenderShadows);
	SetInt(GameState->BlockParticleShader, "ShadowMaps", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, GameState->ShadowMapsArray);

	RenderChunks(&GameState->World, GameState->WorldShader, ViewProjection);
	DEBUG_VARIABLE(bool32, ShowDebugDrawings);
	RenderEntities(GameState, TempState, SimRegion, GameState->WorldShader, GameState->CharacterShader,
				   GameState->BillboardShader, Right, ShowDebugDrawings);
	RenderBlockParticles(&GameState->BlockParticleGenerator, &GameState->World, &TempState->Allocator, 
						 GameState->BlockParticleShader, GameState->Hero.Entity->P);
	RenderParticleEffects(GameState, TempState, SimRegion, GameState->BillboardShader, Right);

	EndSimulation(GameState, SimRegion, &GameState->WorldAllocator);
	EndTemporaryMemory(WorldConstructionAndRenderMemory);

	UnloadAssetsIfNecessary(TempState->GameAssets);
}