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

inline void
AddParticlesToEntity(game_state *GameState, stored_entity *Entity, 
					 r32 MaxLifeTime, u32 SpawnInSecond, r32 Scale,
					 vec3 StartPRanges, vec3 StartdPRanges, r32 dPY, vec3 StartddP, 
					 vec3 Color)
{
	// TODO(georgy): Allow multiple particle systems for an entity
	if(!Entity->Sim.ParticlesInfo)
	{
		Entity->Sim.ParticlesInfo = PushStruct(&GameState->FundamentalTypesAllocator, particle_emitter_info);
		Entity->Sim.ParticlesInfo->MaxLifeTime = MaxLifeTime;
		Entity->Sim.ParticlesInfo->SpawnInSecond = SpawnInSecond;
		Entity->Sim.ParticlesInfo->Scale = Scale;
		Entity->Sim.ParticlesInfo->StartPRanges = StartPRanges;
		Entity->Sim.ParticlesInfo->StartdPRanges = StartdPRanges;
		Entity->Sim.ParticlesInfo->dPY = dPY;
		Entity->Sim.ParticlesInfo->StartddP = StartddP;
		Entity->Sim.ParticlesInfo->Color = Color;
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

	Entity.StoredEntity->Sim.CanBeGravityAffected = true;
	AddFlags(&Entity.StoredEntity->Sim, EntityFlag_Moveable | EntityFlag_Collides | EntityFlag_GravityAffected);
	Entity.StoredEntity->Sim.HitPoints = 20;
	Entity.StoredEntity->Sim.MaxHitPoints = 100;
	Entity.StoredEntity->Sim.ManaPoints = 87;
	Entity.StoredEntity->Sim.MaxManaPoints = 100;
	Entity.StoredEntity->Sim.Rotation = 0.0f;
	Entity.StoredEntity->Sim.Collision = GameState->HeroCollision;
	Entity.StoredEntity->Sim.Fireball.StorageIndex = AddFireball(GameState);

	AddParticlesToEntity(GameState, &GameState->StoredEntities[Entity.StoredEntity->Sim.Fireball.StorageIndex], 
						 1.0f, 40, 1.0f, vec3(1.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 1.0f), 4.0f, vec3(0.0f, -9.8f, 0.0f),
						 vec3(1.0f, 0.078f, 0.098f));

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
InitializeAudioState(audio_state *AudioState, stack_allocator *Allocator)
{
	AudioState->Allocator = Allocator;
	AudioState->FirstPlayingSound = 0;
	AudioState->FirstFreePlayingSound = 0;
	
	AudioState->GlobalVolume = 1.0f;
}

internal playing_sound *
PlaySound(audio_state *AudioState, sound_id SoundID)
{
	if(!AudioState->FirstFreePlayingSound)
	{
		// TODO(georgy): Add audio allocator??
		AudioState->FirstFreePlayingSound = PushStruct(AudioState->Allocator, playing_sound);
	}

	playing_sound *PlayingSound = AudioState->FirstFreePlayingSound;
	AudioState->FirstFreePlayingSound = AudioState->FirstFreePlayingSound->NextFree;

	PlayingSound->Next = AudioState->FirstPlayingSound;
	AudioState->FirstPlayingSound = PlayingSound;

	PlayingSound->ID = SoundID;
	PlayingSound->Volume = { 1.0f, 1.0f };
	PlayingSound->SamplesPlayed = 0;

	return(PlayingSound);
}

internal void
ChangeVolume(playing_sound *Sound, vec2 Volume, r32 VolumeChangeInSeconds)
{
	if(VolumeChangeInSeconds <= 0.0f)
	{
		Sound->Volume = Sound->TargetVolume = Volume;
	}
	else
	{
		Sound->dVolume = (Sound->TargetVolume - Sound->Volume) * (1.0f / VolumeChangeInSeconds);
		Sound->TargetVolume = Volume;
	}
}

internal void
GameUpdate(game_memory *Memory, game_input *Input, int BufferWidth, int BufferHeight,  
		   bool32 GameProfilingPause, bool32 DebugCamera, game_input *DebugCameraInput)
{
	Platform = Memory->PlatformAPI;

	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!GameState->IsInitialized)
	{
		InitializeStackAllocator(&GameState->WorldAllocator, Memory->PermanentStorageSize - sizeof(game_state),
															 (u8 *)Memory->PermanentStorage + sizeof(game_state));
		SubAllocator(&GameState->FundamentalTypesAllocator, &GameState->WorldAllocator, Megabytes(64));

		GameState->Camera = {};
		GameState->Camera.DistanceFromHero = 9.0f;
		GameState->Camera.RotSensetivity = 0.1f;
		GameState->Camera.NearDistance = 0.1f;
		GameState->Camera.FarDistance = 160.0f;
		GameState->Camera.FoV = 45.0f;
		// TODO(georgy): Get this from hero head model or smth
		// NOTE(georgy): This is hero head centre offset from hero feet pos. 
		// 				 We use this to offset all object in view matrix. So Camera->OffsetFromHero is offset from hero head centre  
		GameState->Camera.TargetOffset = vec3(0.0f, 0.680000007f + 0.1f, 0.0f);
		GameState->Camera.DEBUGFront = vec3(0.0f, 0.0f, -1.0f);

		InitializeWorld(&GameState->World);

		InitializeAudioState(&GameState->AudioState, &GameState->FundamentalTypesAllocator);

		GameState->HeroCollision = MakeSimpleCollision(GameState, vec3(0.54f, 0.54f, 0.48f));
		GameState->FireballCollision = MakeSimpleCollision(GameState, vec3(0.25f, 0.25f, 0.25f));
		GameState->TreeCollision = MakeSimpleCollision(GameState, vec3(0.5f, 0.5f, 0.5f));
		GameState->TESTCubeCollision = MakeSimpleCollision(GameState, vec3(1.0f, 1.0f, 1.0f));

		CompileShader(&GameState->CharacterShader, "data/shaders/CharacterVS.glsl", "data/shaders/CharacterFS.glsl");
		CompileShader(&GameState->WorldShader, "data/shaders/WorldVS.glsl", "data/shaders/WorldFS.glsl");
		CompileShader(&GameState->WaterShader, "data/shaders/WaterVS.glsl", "data/shaders/WaterFS.glsl");
		CompileShader(&GameState->HitpointsShader, "data/shaders/HitpointsVS.glsl", "data/shaders/HitpointsFS.glsl");
		CompileShader(&GameState->BillboardShader, "data/shaders/BillboardVS.glsl", "data/shaders/BillboardFS.glsl");
		CompileShader(&GameState->BlockParticleShader, "data/shaders/BlockParticleVS.glsl", "data/shaders/BlockParticleFS.glsl");
		CompileShader(&GameState->CharacterDepthShader, "data/shaders/CharacterDepthVS.glsl", "data/shaders/EmptyFS.glsl");
		CompileShader(&GameState->WorldDepthShader, "data/shaders/WorldDepthVS.glsl", "data/shaders/EmptyFS.glsl");
		CompileShader(&GameState->BlockParticleDepthShader, "data/shaders/BlockParticleDepthVS.glsl", "data/shaders/EmptyFS.glsl");
		CompileShader(&GameState->UIQuadShader, "data/shaders/UI_VS.glsl", "data/shaders/UI_FS.glsl");
		CompileShader(&GameState->UIGlyphShader, "data/shaders/GlyphVS.glsl", "data/shaders/GlyphFS.glsl");
		CompileShader(&GameState->FramebufferScreenShader, "data/shaders/FramebufferScreenVS.glsl", "data/shaders/FramebufferScreenFS.glsl");

		particle_generator *ParticleGenerator = &GameState->ParticleGenerator;
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
		glGenVertexArrays(1, &ParticleGenerator->VAO);
		glGenBuffers(1, &ParticleGenerator->VBO);
		glGenBuffers(1, &ParticleGenerator->SimPVBO);
		glGenBuffers(1, &ParticleGenerator->ColorVBO);
		glGenBuffers(1, &ParticleGenerator->ScaleVBO);
		glBindVertexArray(ParticleGenerator->VAO);
		glBindBuffer(GL_ARRAY_BUFFER, ParticleGenerator->VBO);
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

		vec2 UIQuadVertices[] =
		{
			vec2(-0.5f, 0.5f),
			vec2(-0.5f, -0.5f),
			vec2(0.5f, 0.5f),
			vec2(0.5f, -0.5f)
		};
		glGenVertexArrays(1, &GameState->UIQuadVAO);
		glGenBuffers(1, &GameState->UIQuadVBO);
		glBindVertexArray(GameState->UIQuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, GameState->UIQuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(UIQuadVertices), UIQuadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void *)0);
		glBindVertexArray(0);

		vec2 UIGlyphVertices[] = 
		{
			vec2(0.0f, 1.0f),
			vec2(0.0f, 0.0f),
			vec2(1.0f, 1.0f),
			vec2(1.0f, 0.0f)
		};
		glGenVertexArrays(1, &GameState->UIGlyphVAO);
		glGenBuffers(1, &GameState->UIGlyphVBO);
		glBindVertexArray(GameState->UIGlyphVAO);
		glBindBuffer(GL_ARRAY_BUFFER, GameState->UIGlyphVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(UIGlyphVertices), UIGlyphVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void *)0);
		glBindVertexArray(0);

		
		// NOTE(georgy): Reserve slot 0
		AddStoredEntity(GameState, EntityType_Null, InvalidPosition());

		world_position TestP = {};
		TestP.Offset = vec3(0.0f, 8.0f, 3.0f);
		// AddParticlesToEntity(GameState, TESTAddCube(GameState, TestP, vec3(1.0f, 1.0f, 1.0f), 36, CubeVertices),
		// 					 2.0f, 10, 0.3f, vec3(0.5f, 0.0f, 0.5f), vec3(0.5f, 0.0f, 0.5f), 4.0f, vec3(0.0f, -9.8f, 0.0f),
		// 					 vec3(0.0f, 0.0f, 0.7f));

		world_position HeroP = {};
		HeroP.ChunkY = MAX_CHUNKS_Y;
		HeroP.Offset = vec3(0.3f, 5.0f, 3.0f);
		GameState->Hero.Entity = AddHero(GameState, HeroP);
		GameState->LastHeroWorldP = HeroP;
		// AddParticlesToEntity(GameState, GameState->Hero.Entity,
		// 					 1.5f, 30, 0.2f, vec3(0.0f, 0.0f, 0.0f), vec3(0.65f, 0.0f, 0.65f), 2.0f, vec3(0.0f, 0.0f, 0.0f),
		// 					 vec3(0.25f, 0.25f, 0.25f));

		world_position TestTreeP = {};
		TestTreeP.ChunkY = MAX_CHUNKS_Y;
		TestTreeP.Offset = vec3(0.0f, 2.0f, 3.0f);
		AddTree(GameState, TestTreeP);

		glGenFramebuffers(1, &GameState->ShadowMapFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, GameState->ShadowMapFBO);
		GameState->ShadowMapsWidth = GameState->ShadowMapsHeight = 2048;

		glGenTextures(1, &GameState->ShadowMapsArray);
		glBindTexture(GL_TEXTURE_2D_ARRAY, GameState->ShadowMapsArray);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, GameState->ShadowMapsWidth, GameState->ShadowMapsHeight, 
					 CASCADES_COUNT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GameState->ShadowMapsArray, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		srand(1234);
		for(u32 Y = 0;
			Y < 4;
			Y++)
		{
			r32 StrataY1 = (Y/ 4.0f);
			r32 StrataY2 = ((Y + 1) / 4.0f);

			for(u32 X = 0;
				X < 4;
				X++)
			{
				r32 StrataX1 = (X / 4.0f);
				r32 StrataX2 = ((X + 1) / 4.0f);

				r32 U = ((rand() % 100) / 99.0f)*(StrataX2 - StrataX1) + StrataX1;
				r32 V = ((rand() % 100) / 99.0f)*(StrataY2 - StrataY1) + StrataY1;

				vec2 DiskP = vec2(SquareRoot(V)*Cos(2.0f*PI*U), SquareRoot(V)*Sin(2.0f*PI*U));

				GameState->ShadowSamplesOffsets[Y*4 + X] = DiskP;
			}
		}

		vec2 ShadowNoise[64];
		for(u32 NoiseIndex = 0;
			NoiseIndex < 64;
			NoiseIndex++)
		{
			r32 X = ((rand() % 100) / 99.0f)*2.0f - 1.0f;
			r32 Y = ((rand() % 100) / 99.0f)*2.0f - 1.0f;
			ShadowNoise[NoiseIndex] = Normalize(vec2(X, Y));
		}
		glGenTextures(1, &GameState->ShadowNoiseTexture);
		glBindTexture(GL_TEXTURE_2D, GameState->ShadowNoiseTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 8, 8, 0, GL_RG, GL_FLOAT, ShadowNoise);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		InitializeDefaultAnimations(GameState->CharacterAnimations);

		GameState->IsInitialized = true;
	}

	Assert(sizeof(temp_state) <= Memory->TemporaryStorageSize);
	temp_state *TempState = (temp_state *)Memory->TemporaryStorage;
	if(!TempState->IsInitialized)
	{
		InitializeStackAllocator(&TempState->Allocator, Memory->TemporaryStorageSize - sizeof(temp_state),
														(u8 *)Memory->TemporaryStorage + sizeof(temp_state));

		TempState->JobSystemQueue = Memory->JobSystemQueue;
		// TempState->GameAssets = AllocateGameAssets(TempState, &TempState->Allocator, 60000);
		TempState->GameAssets = AllocateGameAssets(TempState, &TempState->Allocator, Megabytes(64));

		playing_sound *Music = PlaySound(&GameState->AudioState, GetFirstSoundFromType(TempState->GameAssets, AssetType_Music));
		// ChangeVolume(Music, vec2(0.5f, 0.5f), 10.0f);

		TempState->IsInitialized = true;
	}

	DEBUG_VARIABLE(r32, GlobalVolume, DebugTools);
	GameState->AudioState.GlobalVolume = GlobalVolume;

	camera *Camera = &GameState->Camera;
	Camera->AspectRatio = (r32)BufferWidth/(r32)BufferHeight;

#if VOXEL_ENGINE_INTERNAL
	if(!DebugCamera || DEBUGGlobalPlaybackInfo.PlaybackPhase)
#endif
	{
		Camera->Pitch -= Input->MouseYDisplacement*Camera->RotSensetivity;
		Camera->Head += Input->MouseXDisplacement*Camera->RotSensetivity;

		Camera->Pitch = Camera->Pitch > 89.0f ? 89.0f : Camera->Pitch;
		Camera->Pitch = Camera->Pitch < -89.0f ? -89.0f : Camera->Pitch;

		r32 PitchRadians = DEG2RAD(Camera->Pitch);
		r32 HeadRadians = DEG2RAD(Camera->Head);
		r32 HorizontalDistanceFromHero = Camera->DistanceFromHero*Cos(-PitchRadians);
		r32 XOffsetFromHero = -HorizontalDistanceFromHero * Sin(HeadRadians);
		r32 YOffsetFromHero = Camera->DistanceFromHero * Sin(-PitchRadians);
		r32 ZOffsetFromHero = HorizontalDistanceFromHero * Cos(HeadRadians);
		vec3 NewTargetOffsetFromHero = vec3(XOffsetFromHero, YOffsetFromHero, ZOffsetFromHero);
		Camera->OffsetFromHero = Camera->DistanceFromHero * 
								 Normalize(Lerp(Camera->LastOffsetFromHero, NewTargetOffsetFromHero, 7.0f*Input->dt));
		Camera->LastOffsetFromHero = Camera->OffsetFromHero;
	}

	vec3 Forward = Normalize(vec3(-Camera->OffsetFromHero.x(), 0.0f, -Camera->OffsetFromHero.z()));
	vec3 Right = Normalize(Cross(Forward, vec3(0.0f, 1.0f, 0.0f)));
	r32 Theta = -RAD2DEG(ATan2(Forward.z(), Forward.x())) + 90.0f;
	GameState->Hero.ddP = vec3(0.0f, 0.0f, 0.0f);

	// NOTE(georgy): Hero input stuff
#if VOXEL_ENGINE_INTERNAl
	if(DEBUGGlobalPlaybackInfo.PlaybackPhase || (!GameProfilingPause && !DebugCamera))
#endif
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

	// NOTE(georgy): Debug camera input stuff
	if(DebugCamera)
	{
		{
			Camera->DEBUGPitch -= DebugCameraInput->MouseYDisplacement*Camera->RotSensetivity;
			Camera->DEBUGHead += DebugCameraInput->MouseXDisplacement*Camera->RotSensetivity;

			Camera->DEBUGPitch = Camera->DEBUGPitch > 89.0f ? 89.0f : Camera->DEBUGPitch;
			Camera->DEBUGPitch = Camera->DEBUGPitch < -89.0f ? -89.0f : Camera->DEBUGPitch;

			r32 CameraTargetDirX = Sin(DEG2RAD(Camera->DEBUGHead))*Cos(DEG2RAD(Camera->DEBUGPitch));
			r32 CameraTargetDirY = Sin(DEG2RAD(Camera->DEBUGPitch));
			r32 CameraTargetDirZ = -Cos(DEG2RAD(Camera->DEBUGHead))*Cos(DEG2RAD(Camera->DEBUGPitch));
			Camera->DEBUGFront = Normalize(vec3(CameraTargetDirX, CameraTargetDirY, CameraTargetDirZ));
		}

		vec3 Forward = Camera->DEBUGFront;
		vec3 Right = Normalize(Cross(Forward, vec3(0.0f, 1.0f, 0.0f)));
		if (DebugCameraInput->MoveForward.EndedDown)
		{
			Camera->DEBUGP += Forward;
		}
		if (DebugCameraInput->MoveBack.EndedDown)
		{
			Camera->DEBUGP  -= Forward;
		}
		if (DebugCameraInput->MoveRight.EndedDown)
		{
			Camera->DEBUGP  += Right;
		}
		if (DebugCameraInput->MoveLeft.EndedDown)
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

						InitializeDynamicArray(&Chunk->WaterVerticesP);
						InitializeDynamicArray(&Chunk->WaterVerticesColors);

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


	temporary_memory SimulationAndRenderMemory = BeginTemporaryMemory(&TempState->Allocator);

	DEBUG_VARIABLE(r32, SimBoundsRadius, World);
	rect3 SimRegionUpdatableBounds = RectMinMax(vec3(-SimBoundsRadius, -SimBoundsRadius, -SimBoundsRadius), 
												vec3(SimBoundsRadius, SimBoundsRadius, SimBoundsRadius));

	vec3 HeroPosDifferenceBetweenFrames = Substract(&GameState->World, &GameState->Hero.Entity->P, &GameState->LastHeroWorldP);
	HeroPosDifferenceBetweenFrames = Lerp(vec3(0.0f, 0.0f, 0.0f), HeroPosDifferenceBetweenFrames, 8.0f*Input->dt);
	world_position HeroWorldP = MapIntoChunkSpace(&GameState->World, &GameState->LastHeroWorldP, HeroPosDifferenceBetweenFrames);
	sim_region *SimRegion = BeginSimulation(GameState, HeroWorldP, 
											SimRegionUpdatableBounds, &TempState->Allocator, Input->dt);
	GameState->LastHeroWorldP = HeroWorldP;

	Camera->RotationMatrix = RotationMatrixFromDirection(Camera->OffsetFromHero);
#if !defined(VOXEL_ENGINE_DEBUG_BUILD)
	CameraCollisionDetection(&GameState->World, Camera, &GameState->Hero.Entity->P);
#endif
	SetupChunksBlocks(&GameState->World, &GameState->WorldAllocator, TempState);								
	SetupChunksVertices(&GameState->World, TempState);
	LoadChunks(&GameState->World);
	CorrectChunksWaterLevel(&GameState->World);
	
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
					ClearFlags(Entity, EntityFlag_GravityAffected);

					MoveSpec.ddP = GameState->Hero.ddP;
					MoveSpec.Speed = 8.0f;
					if(IsSet(Entity, EntityFlag_InWater))
					{
						MoveSpec.Speed *= 0.5f;
					}
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
							Fireball->dP = vec3(Entity->dP.x(), 0.0f, Entity->dP.z()) + 3.0f*Forward;

							PlaySound(&GameState->AudioState, GetFirstSoundFromType(TempState->GameAssets, AssetType_Fireball));
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
						DesiredAnimation = CharacterAnimation_Run;
						// DesiredAnimation = CharacterAnimation_Jump;
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
							AnimationTimeStep = 1.5f*Length(vec3(Entity->dP.x(), 0.0f, Entity->dP.z()))*dt;
						}
					}

					Entity->AnimationState.Time += AnimationTimeStep;
					DEBUG_VALUE(r32, AnimationStateTime, World, Entity->AnimationState.Time);
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

			if(Entity->ParticlesInfo)
			{
				particle_emitter_info *EntityParticlesInfo = Entity->ParticlesInfo;
				world_position EntityWorldPosition = GameState->StoredEntities[Entity->StorageIndex].P;
				SpawnParticles(&GameState->ParticleGenerator, EntityWorldPosition, *Entity->ParticlesInfo, dt);
			}

			if(IsSet(Entity, EntityFlag_Moveable))
			{
				MoveEntity(GameState, SimRegion, Entity, MoveSpec, dt, false);
				MoveEntity(GameState, SimRegion, Entity, MoveSpec, dt, true);
			}
		}
	}
	ParticlesUpdate(&GameState->ParticleGenerator, Input->dt);

	DEBUG_VARIABLE(bool32, ShowDebugDrawings, Rendering);

	r32 OneDayCycleInSeconds = 1.0f*60.0f;
	r32 DayCycleInCircle = (GameState->t / OneDayCycleInSeconds) * 1.5f*PI;
	// GameState->DirectionalLightDir = -vec3(Cos(DayCycleInCircle), Sin(DayCycleInCircle), 0.0f);
	// GameState->DirectionalLightDir = Normalize(vec3(-0.123566f, -0.9712314f, 0.0f));
	GameState->DirectionalLightDir = Normalize(vec3(-0.5f, -0.3f, 0.0f));
	GameState->t += Input->dt;
	if(GameState->t > OneDayCycleInSeconds)
	{
		GameState->t = 0.0f;
	} 
	if (ShowDebugDrawings)
	{
		DEBUGRenderLine(vec3(0.0f, 0.0f, 0.0f), -GameState->DirectionalLightDir, vec3(1.0f, 1.0f, 0.0f));
	}


	// NOTE(georgy): Directional shadow map rendering
	r32 CascadesDistances[CASCADES_COUNT + 1] = {Camera->NearDistance, 25.0f, 50.0f, 75.0f};
	mat4 LightSpaceMatrices[CASCADES_COUNT];
	DEBUG_IF(RenderShadows, Rendering)
	{
		BEGIN_BLOCK(ShadowStuff);

		glViewport(0, 0, GameState->ShadowMapsWidth, GameState->ShadowMapsHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, GameState->ShadowMapFBO);
		for(u32 CascadeIndex = 0;
			CascadeIndex < CASCADES_COUNT;
			CascadeIndex++)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GameState->ShadowMapsArray, 0, CascadeIndex);
			glClear(GL_DEPTH_BUFFER_BIT);

			vec3 CameraRight = vec3(Camera->RotationMatrix.FirstColumn.x(), Camera->RotationMatrix.SecondColumn.x(), Camera->RotationMatrix.ThirdColumn.x());
			vec3 CameraUp = vec3(Camera->RotationMatrix.FirstColumn.y(), Camera->RotationMatrix.SecondColumn.y(), Camera->RotationMatrix.ThirdColumn.y());
			vec3 CameraOut = -vec3(Camera->RotationMatrix.FirstColumn.z(), Camera->RotationMatrix.SecondColumn.z(), Camera->RotationMatrix.ThirdColumn.z());

			r32 NearPlaneHalfHeight = Tan(0.5f*DEG2RAD(Camera->FoV))*CascadesDistances[CascadeIndex];
			r32 NearPlaneHalfWidth = NearPlaneHalfHeight*Camera->AspectRatio;
			r32 FarPlaneHalfHeight = Tan(0.5f*DEG2RAD(Camera->FoV))*CascadesDistances[CascadeIndex + 1];
			r32 FarPlaneHalfWidth = FarPlaneHalfHeight*Camera->AspectRatio;

			vec4 Cascade[8];
			Cascade[0] = vec4(CameraRight*NearPlaneHalfWidth + CameraUp*NearPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex], 1.0f);
			Cascade[1] = vec4(CameraRight*NearPlaneHalfWidth - CameraUp*NearPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex], 1.0f);
			Cascade[2] = vec4(-CameraRight*NearPlaneHalfWidth + CameraUp*NearPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex], 1.0f);
			Cascade[3] = vec4(-CameraRight*NearPlaneHalfWidth - CameraUp*NearPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex], 1.0f);
			Cascade[4] = vec4(CameraRight*FarPlaneHalfWidth + CameraUp*FarPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex + 1], 1.0f);
			Cascade[5] = vec4(CameraRight*FarPlaneHalfWidth - CameraUp*FarPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex + 1], 1.0f);
			Cascade[6] = vec4(-CameraRight*FarPlaneHalfWidth + CameraUp*FarPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex + 1], 1.0f);
			Cascade[7] = vec4(-CameraRight*FarPlaneHalfWidth - CameraUp*FarPlaneHalfHeight + CameraOut*CascadesDistances[CascadeIndex + 1], 1.0f);

			for(u32 PointIndex = 0;
				PointIndex < ArrayCount(Cascade);
				PointIndex++)
			{
				Cascade[PointIndex] = Cascade[PointIndex] + vec4(Camera->OffsetFromHero + Camera->TargetOffset, 0.0f);
			}
			
			vec3 SphereP;
			r32 Radius;
			r32 MinX = FLT_MAX, MinY = FLT_MAX, MinZ = FLT_MAX, MaxX = -FLT_MAX, MaxY = -FLT_MAX, MaxZ = -FLT_MAX;
			u32 MinXIndex = ArrayCount(Cascade) - 1, MinYIndex = ArrayCount(Cascade) - 1, MinZIndex = ArrayCount(Cascade) - 1, MaxXIndex = 0, MaxYIndex = 0, MaxZIndex = 0;
			for(u32 PointIndex = 0;
				PointIndex < ArrayCount(Cascade);
				PointIndex++)
			{
				if(MinX > Cascade[PointIndex].x()) { MinX = Cascade[PointIndex].x(); MinXIndex = PointIndex; }
				if(MinY > Cascade[PointIndex].y()) { MinY = Cascade[PointIndex].y(); MinYIndex = PointIndex; }
				if(MinZ > Cascade[PointIndex].z()) { MinZ = Cascade[PointIndex].z(); MinZIndex = PointIndex; }
				if(MaxX < Cascade[PointIndex].x()) { MaxX = Cascade[PointIndex].x(); MaxXIndex = PointIndex; }
				if(MaxY < Cascade[PointIndex].y()) { MaxY = Cascade[PointIndex].y(); MaxYIndex = PointIndex; }
				if(MaxZ < Cascade[PointIndex].z()) { MaxZ = Cascade[PointIndex].z(); MaxZIndex = PointIndex; }
			}
			r32 XDiff = MaxX - MinX;
			r32 YDiff = MaxY - MinY;
			r32 ZDiff = MaxZ - MinZ;
			if(XDiff > YDiff)
			{
				if(XDiff > ZDiff)
				{
					SphereP = Lerp(vec3(Cascade[MinXIndex].m), vec3(Cascade[MaxXIndex].m), 0.5f);
					Radius = MaxX - SphereP.x();
				}
				else
				{
					SphereP = Lerp(vec3(Cascade[MinZIndex].m), vec3(Cascade[MaxZIndex].m), 0.5f);
					Radius = MaxZ - SphereP.z();
				}
			}
			else
			{
				if(YDiff > ZDiff)
				{
					SphereP = Lerp(vec3(Cascade[MinYIndex].m), vec3(Cascade[MaxYIndex].m), 0.5f);
					Radius = MaxY - SphereP.y();
				}
				else
				{
					SphereP = Lerp(vec3(Cascade[MinZIndex].m), vec3(Cascade[MaxZIndex].m), 0.5f);
					Radius = MaxZ - SphereP.z();
				}
			}
			
			for(u32 PointIndex = 0;
				PointIndex < ArrayCount(Cascade);
				PointIndex++)
			{
				vec3 FromCenter = vec3(Cascade[PointIndex].m) - SphereP;
				r32 DistanceToPoint = Length(FromCenter);
				if(DistanceToPoint > Radius)
				{
					SphereP += (0.5f*(DistanceToPoint - Radius))*Normalize(FromCenter);
					Radius = 0.5f*(DistanceToPoint + Radius);
				}
			}

			Radius = Round(Radius);
			
			r32 ExtraBackup = 60.0f;
			r32 NearClip = 1.0f;
			r32 FarClip = ExtraBackup + 2.0f*Radius;
			r32 BackupDistance = ExtraBackup + NearClip + Radius;
			vec3 LightP = SphereP - GameState->DirectionalLightDir*BackupDistance;
			
			mat4 LightView = LookAt(LightP, SphereP);
			mat4 LightProjection = Ortho(-Radius, Radius, -Radius, Radius,
										 NearClip, FarClip);

			LightSpaceMatrices[CascadeIndex] = LightProjection * LightView;

			world_position HeroWorldP = GameState->Hero.Entity->P;
			chunk *HeroChunk = GetChunk(&GameState->World, HeroWorldP.ChunkX, HeroWorldP.ChunkY, HeroWorldP.ChunkZ);
			vec4 HeroChunkP = vec4(HeroChunk->Translation, 1.0f);
			vec3 ShadowOrigin = vec3((LightSpaceMatrices[CascadeIndex] * HeroChunkP).m);
			ShadowOrigin *= 0.5f*GameState->ShadowMapsWidth;
			vec2 RoundedOrigin = vec2(Round(ShadowOrigin.x()), Round(ShadowOrigin.y()));
			vec2 Rounding = RoundedOrigin - vec2(ShadowOrigin.x(), ShadowOrigin.y());
			Rounding *= 2.0f / GameState->ShadowMapsWidth;
			mat4 RoundMatrix = Translate(vec3(Rounding.x, Rounding.y, 0.0f));
			LightSpaceMatrices[CascadeIndex] = RoundMatrix*LightSpaceMatrices[CascadeIndex];

			UseShader(GameState->WorldDepthShader);
			SetMat4(GameState->WorldDepthShader, "ViewProjection", LightSpaceMatrices[CascadeIndex]);
			UseShader(GameState->CharacterDepthShader);
			SetMat4(GameState->CharacterDepthShader, "ViewProjection", LightSpaceMatrices[CascadeIndex]);
			UseShader(GameState->BlockParticleDepthShader);
			SetMat4(GameState->BlockParticleDepthShader, "ViewProjection", LightSpaceMatrices[CascadeIndex]);

			RenderEntities(GameState, TempState, SimRegion, GameState->WorldDepthShader, 
						   GameState->CharacterDepthShader, GameState->HitpointsShader, Right);
			RenderChunks(&GameState->World, GameState->WorldDepthShader, GameState->WaterShader, LightSpaceMatrices[CascadeIndex], Camera->OffsetFromHero);
			RenderParticles(&GameState->ParticleGenerator, &GameState->World, &TempState->Allocator, 
						    GameState->BlockParticleDepthShader, GameState->Hero.Entity->P);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		END_BLOCK(ShadowStuff);
	}

	// NOTE(georgy): World and entities rendering
	glViewport(0, 0, BufferWidth, BufferHeight);

	mat4 View = Camera->RotationMatrix * 
				Translate(-Camera->OffsetFromHero - Camera->TargetOffset);
	mat4 Projection = Perspective(Camera->FoV, Camera->AspectRatio, Camera->NearDistance, Camera->FarDistance);
	mat4 ViewProjection = Projection * View;
	shader Shaders3D[] = { GameState->WorldShader, GameState->WaterShader, GameState->CharacterShader,
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
	

	DEBUG_VARIABLE(r32, Bias, Rendering);
	UseShader(GameState->WorldShader);
	// NOTE(georgy): This is for the situation when we use debug camera. Even if we use debug camera,
	//  			 we want Output.ClipSpacePosZ to be as it is from our default camera, not debug one
	SetMat4(GameState->WorldShader, "ViewProjectionForClipSpacePosZ", ViewProjection);
	SetMat4Array(GameState->WorldShader, "LightSpaceMatrices", CASCADES_COUNT, LightSpaceMatrices);
	SetFloatArray(GameState->WorldShader, "CascadesDistances", CASCADES_COUNT + 1, CascadesDistances);
	SetVec2Array(GameState->WorldShader, "SampleOffsets", 64, GameState->ShadowSamplesOffsets);
	SetVec3(GameState->WorldShader, "DirectionalLightDir", GameState->DirectionalLightDir);
	SetInt(GameState->WorldShader, "ShadowsEnabled", RenderShadows);
	SetInt(GameState->WorldShader, "ShadowMaps", 0);
	SetFloat(GameState->WorldShader, "Bias", Bias);
	SetInt(GameState->WorldShader, "ShadowNoiseTexture", 1);
	SetInt(GameState->WorldShader, "Width", BufferWidth);
	SetInt(GameState->WorldShader, "Height", BufferHeight);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, GameState->ShadowMapsArray);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, GameState->ShadowNoiseTexture);

	UseShader(GameState->WaterShader);
	// NOTE(georgy): This is for the situation when we use debug camera. Even if we use debug camera,
	//  			 we want Output.ClipSpacePosZ to be as it is from our default camera, not debug one
	SetMat4(GameState->WaterShader, "View", View);
	SetMat4(GameState->WaterShader, "ViewProjectionForClipSpacePosZ", ViewProjection);
	SetMat4Array(GameState->WaterShader, "LightSpaceMatrices", CASCADES_COUNT, LightSpaceMatrices);
	SetFloatArray(GameState->WaterShader, "CascadesDistances", CASCADES_COUNT + 1, CascadesDistances);
	SetVec3(GameState->WaterShader, "DirectionalLightDir", GameState->DirectionalLightDir);
	SetInt(GameState->WaterShader, "ShadowsEnabled", RenderShadows);
	SetInt(GameState->WaterShader, "ShadowMaps", 0);
	SetFloat(GameState->WaterShader, "Bias", Bias);
	SetInt(GameState->WaterShader, "ShadowNoiseTexture", 1);
	SetInt(GameState->WaterShader, "Width", BufferWidth);
	SetInt(GameState->WaterShader, "Height", BufferHeight);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, GameState->ShadowMapsArray);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, GameState->ShadowNoiseTexture);

	UseShader(GameState->CharacterShader);
	SetMat4(GameState->CharacterShader, "ViewProjectionForClipSpacePosZ", ViewProjection);
	SetMat4Array(GameState->CharacterShader, "LightSpaceMatrices", CASCADES_COUNT, LightSpaceMatrices);
	SetFloatArray(GameState->CharacterShader, "CascadesDistances", CASCADES_COUNT + 1, CascadesDistances);
	SetVec2Array(GameState->CharacterShader, "SampleOffsets", 64, GameState->ShadowSamplesOffsets);
	SetVec3(GameState->CharacterShader, "DirectionalLightDir", GameState->DirectionalLightDir);
	SetInt(GameState->CharacterShader, "ShadowsEnabled", RenderShadows);
	SetInt(GameState->CharacterShader, "ShadowMaps", 0);
	SetFloat(GameState->CharacterShader, "Bias", Bias);
	SetInt(GameState->CharacterShader, "ShadowNoiseTexture", 1);
	SetInt(GameState->CharacterShader, "Width", BufferWidth);
	SetInt(GameState->CharacterShader, "Height", BufferHeight);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, GameState->ShadowMapsArray);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, GameState->ShadowNoiseTexture);

	UseShader(GameState->BlockParticleShader);
	SetMat4(GameState->BlockParticleShader, "ViewProjectionForClipSpacePosZ", ViewProjection);
	SetMat4Array(GameState->BlockParticleShader, "LightSpaceMatrices", CASCADES_COUNT, LightSpaceMatrices);
	SetFloatArray(GameState->BlockParticleShader, "CascadesDistances", CASCADES_COUNT + 1, CascadesDistances);
	SetVec2Array(GameState->BlockParticleShader, "SampleOffsets", 64, GameState->ShadowSamplesOffsets);
	SetVec3(GameState->BlockParticleShader, "DirectionalLightDir", GameState->DirectionalLightDir);
	SetInt(GameState->BlockParticleShader, "ShadowsEnabled", RenderShadows);
	SetInt(GameState->BlockParticleShader, "ShadowMaps", 0);
	SetFloat(GameState->BlockParticleShader, "Bias", Bias);
	SetInt(GameState->BlockParticleShader, "ShadowNoiseTexture", 1);
	SetInt(GameState->BlockParticleShader, "Width", BufferWidth);
	SetInt(GameState->BlockParticleShader, "Height", BufferHeight);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, GameState->ShadowMapsArray);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, GameState->ShadowNoiseTexture);

	UseShader(GameState->BillboardShader);
	SetVec3(GameState->BillboardShader, "DirectionalLightDir", GameState->DirectionalLightDir);

	RenderEntities(GameState, TempState, SimRegion, 
				   GameState->WorldShader, GameState->CharacterShader, GameState->HitpointsShader,
				   Right, ShowDebugDrawings);
	RenderChunks(&GameState->World, GameState->WorldShader, GameState->WaterShader, ViewProjection, Camera->OffsetFromHero);
	RenderParticles(&GameState->ParticleGenerator, &GameState->World, &TempState->Allocator, 
					GameState->BlockParticleShader, GameState->Hero.Entity->P);

	// NOTE(georgy): Render UI
	mat4 Orthographic = Ortho(-0.5f*BufferHeight, 0.5f*BufferHeight, 
							  -0.5f*BufferWidth, 0.5f*BufferWidth,
							  0.1f, 100.0f);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	UseShader(GameState->UIQuadShader);
	SetMat4(GameState->UIQuadShader, "Projection", Orthographic);
	SetInt(GameState->UIQuadShader, "Texture", 0);
	glActiveTexture(GL_TEXTURE0);
	
	UseShader(GameState->UIGlyphShader);
	SetMat4(GameState->UIGlyphShader, "Projection", Orthographic);
	SetInt(GameState->UIGlyphShader, "Texture", 0);
	glActiveTexture(GL_TEXTURE0);
	
	UseShader(GameState->UIQuadShader);
	asset_tag_vector MatchVector = {};
	MatchVector.E[Tag_Color] = TagColor_Red;
	texture_id HPBarTextureIndex = GetBestMatchTextureFromType(TempState->GameAssets, AssetType_UIBar, &MatchVector);
	vec2 HPBarTextureDim = vec2(150.0f, 25.0f);
	vec2 HPBarTextureP = vec2(-0.5f*HPBarTextureDim.x - 5.0f, (-0.5f*BufferHeight + 10.0f) + 0.5f*HPBarTextureDim.y);
	RenderQuad(GameState->UIQuadShader, GameState->UIQuadVAO, TempState->GameAssets, HPBarTextureIndex, 
		   	   HPBarTextureP, HPBarTextureDim, 0.5f);

	vec2 HPBarMaxDim = HPBarTextureDim - vec2(10.0f, 10.0f);
	r32 HPBarScale = ((r32)GameState->Hero.Entity->Sim.HitPoints / GameState->Hero.Entity->Sim.MaxHitPoints);
	vec2 HPBarDim = vec2(HPBarMaxDim.x * HPBarScale, HPBarMaxDim.y);
	r32 HPDiff = 0.5f*(HPBarMaxDim.x - HPBarDim.x);
	vec2 HPBarPos = HPBarTextureP + vec2(-HPDiff, 1.0f);
	RenderQuad(GameState->UIQuadShader, GameState->UIQuadVAO,  HPBarPos, 
			   HPBarDim, vec4(1.0f, 0.0f, 0.0f, 1.0f));

	MatchVector = {};
	MatchVector.E[Tag_Color] = TagColor_Blue;
	texture_id MPBarTextureIndex = GetBestMatchTextureFromType(TempState->GameAssets, AssetType_UIBar, &MatchVector);
	vec2 MPBarTextureDim = vec2(150.0f, 25.0f);
	vec2 MPBarTextureP = vec2(0.5f*MPBarTextureDim.x + 5.0f, (-0.5f*BufferHeight + 10.0f) + 0.5f*MPBarTextureDim.y);
	RenderQuad(GameState->UIQuadShader, GameState->UIQuadVAO, TempState->GameAssets, MPBarTextureIndex, 
		   	   MPBarTextureP, MPBarTextureDim, 0.5f);

	vec2 MPBarMaxDim = MPBarTextureDim - vec2(10.0f, 10.0f);
	r32 MPBarScale = ((r32)GameState->Hero.Entity->Sim.ManaPoints / GameState->Hero.Entity->Sim.MaxManaPoints);
	vec2 MPBarDim = vec2(MPBarMaxDim.x * MPBarScale, MPBarMaxDim.y);
	r32 MPDiff = 0.5f*(MPBarMaxDim.x - MPBarDim.x);
	vec2 MPBarPos = MPBarTextureP + vec2(-MPDiff, 1.0f);
	RenderQuad(GameState->UIQuadShader, GameState->UIQuadVAO,  MPBarPos, 
			   MPBarDim, vec4(0.0f, 0.0f, 1.0f, 1.0f));

	asset_tag_vector GameFontMatchVector = { };
	GameFontMatchVector.E[Tag_FontType] = FontType_GameFont;
	GameState->FontID = GetBestMatchFont(TempState->GameAssets, &GameFontMatchVector);
	GameState->Font = GetFont(TempState->GameAssets, GameState->FontID);
	if(GameState->Font)
	{
		vec2 HitPointsP = HPBarTextureP + vec2(0.0f, 1.5f);
		rect2 SlashRect = GetTextLineRect(GameState->Font, vec2(0.0f, 0.0f), "/", 0.2f);
		vec2 SlashDim = GetDim(SlashRect);
		vec2 SlashScreenP = HitPointsP - 0.5f*SlashDim;
		RenderTextLine(GameState->Font, GameState->UIGlyphShader, GameState->UIGlyphVAO, 
					   SlashScreenP, "/", 0.2f);

		char Buffer[256];
		_snprintf_s(Buffer, sizeof(Buffer), "%i", GameState->Hero.Entity->Sim.MaxHitPoints);
		vec2 MaxHPScreenP = SlashScreenP + vec2(SlashDim.x, 0.0f);
		RenderTextLine(GameState->Font, GameState->UIGlyphShader, GameState->UIGlyphVAO, 
					   MaxHPScreenP, Buffer, 0.2f);

		_snprintf_s(Buffer, sizeof(Buffer), "%i", GameState->Hero.Entity->Sim.HitPoints);
		rect2 HPRect = GetTextLineRect(GameState->Font, vec2(0.0f, 0.0f), Buffer, 0.2f);
		vec2 HPDim = GetDim(HPRect);
		vec2 HPScreenP = SlashScreenP - vec2(HPDim.x, 0.0f);
		RenderTextLine(GameState->Font, GameState->UIGlyphShader, GameState->UIGlyphVAO, 
					   HPScreenP, Buffer, 0.2f);

		vec2 ManaPointsP = MPBarTextureP + vec2(0.0f, 1.5f);
		SlashScreenP = ManaPointsP - 0.5f*SlashDim;
		RenderTextLine(GameState->Font, GameState->UIGlyphShader, GameState->UIGlyphVAO, 
					   SlashScreenP, "/", 0.2f);

		_snprintf_s(Buffer, sizeof(Buffer), "%i", GameState->Hero.Entity->Sim.MaxManaPoints);
		vec2 MaxMPScreenP = SlashScreenP + vec2(SlashDim.x, 0.0f);
		RenderTextLine(GameState->Font, GameState->UIGlyphShader, GameState->UIGlyphVAO, 
					   MaxMPScreenP, Buffer, 0.2f);

		_snprintf_s(Buffer, sizeof(Buffer), "%i", GameState->Hero.Entity->Sim.ManaPoints);
		rect2 MPRect = GetTextLineRect(GameState->Font, vec2(0.0f, 0.0f), Buffer, 0.2f);
		vec2 MPDim = GetDim(MPRect);
		vec2 MPScreenP = SlashScreenP - vec2(MPDim.x, 0.0f);
		RenderTextLine(GameState->Font, GameState->UIGlyphShader, GameState->UIGlyphVAO, 
					   MPScreenP, Buffer, 0.2f);
	}
	else
	{
		LoadFont(TempState->GameAssets, GameState->FontID);
	}
	
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);


	EndSimulation(GameState, SimRegion, &GameState->WorldAllocator);
	EndTemporaryMemory(SimulationAndRenderMemory);

	UnloadAssetsIfNecessary(TempState->GameAssets);
}

internal void
GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
{
	TIME_BLOCK;

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	audio_state *AudioState = &GameState->AudioState;
	temp_state *TempState = (temp_state *)Memory->TemporaryStorage;

	temporary_memory SoundMixerMemory = BeginTemporaryMemory(&TempState->Allocator);
	r32 *Real32Channel0 = PushArray(&TempState->Allocator, SoundBuffer->SampleCount, r32);
	r32 *Real32Channel1 = PushArray(&TempState->Allocator, SoundBuffer->SampleCount, r32);

#define ChannelsCount 2

	// NOTE(georgy): Clear the mixer channels
	{
		r32 *Dest0 = Real32Channel0;
		r32 *Dest1 = Real32Channel1;
		for(u32 SampleIndex = 0;
			SampleIndex < SoundBuffer->SampleCount;
			SampleIndex++)
		{
			*Dest0++ = 0.0f;
			*Dest1++ = 0.0f;
		}
	}

	for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound;
		*PlayingSoundPtr;
		)
	{
		playing_sound *PlayingSound = *PlayingSoundPtr;
		bool32 SoundFinished = false;

		loaded_sound *LoadedSound = GetSound(TempState->GameAssets, PlayingSound->ID);
		if(LoadedSound)
		{
			r32 *Dest0 = Real32Channel0;
			r32 *Dest1 = Real32Channel1;

			u32 TotalSamplesToMix = SoundBuffer->SampleCount;
			u32 SamplesRemainingInSound = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
			if(TotalSamplesToMix > SamplesRemainingInSound)
			{
				TotalSamplesToMix = SamplesRemainingInSound;
			}

			while(TotalSamplesToMix > 0)
			{
				u32 SamplesToMix = TotalSamplesToMix;
				vec2 Volume = PlayingSound->Volume;
				vec2 dVolume = (1.0f / SoundBuffer->SamplesPerSecond)*PlayingSound->dVolume;

				bool32 VolumeEnded[ChannelsCount] = {};
				u32 VolumeSampleCount[ChannelsCount] = {U32_MAX, U32_MAX};
				for(u32 ChannelIndex = 0;
					ChannelIndex < 2;
					ChannelIndex++)
				{
					if(dVolume.E[ChannelIndex] != 0.0f)
					{
						r32 VolumeDelta = PlayingSound->TargetVolume.E[ChannelIndex] - PlayingSound->Volume.E[ChannelIndex];
						VolumeSampleCount[ChannelIndex] = (u32)((VolumeDelta / dVolume.E[ChannelIndex]) + 0.5f);
					}
				}

				if((SamplesToMix >= VolumeSampleCount[0]) || (SamplesToMix >= VolumeSampleCount[1]))
				{
					if(VolumeSampleCount[0] == VolumeSampleCount[1])
					{
						VolumeEnded[0] = VolumeEnded[1] = true;
						SamplesToMix = VolumeSampleCount[0];
					}
					else
					{
						if(VolumeSampleCount[0] < VolumeSampleCount[1])
						{
							VolumeEnded[0] = true;
							SamplesToMix = VolumeSampleCount[0];
						}
						else
						{
							VolumeEnded[1] = true;
							SamplesToMix = VolumeSampleCount[1];
						}
					}
				}

				for(u32 SampleIndex = 0;
					SampleIndex < SamplesToMix;
					SampleIndex++)
				{
					r32 SampleValue = LoadedSound->Samples[0][PlayingSound->SamplesPlayed + SampleIndex];
					*Dest0++ += AudioState->GlobalVolume*Volume.E[0]*SampleValue;
					*Dest1++ += AudioState->GlobalVolume*Volume.E[1]*SampleValue;

					Volume += dVolume;
				}

				PlayingSound->Volume = Volume;

				for(u32 ChannelIndex = 0;
					ChannelIndex < 2;
					ChannelIndex++)
				{
					if(VolumeEnded[ChannelIndex])
					{
						PlayingSound->Volume.E[ChannelIndex] = PlayingSound->TargetVolume.E[ChannelIndex];
						PlayingSound->dVolume.E[ChannelIndex] = 0.0f;
					}
				}

				PlayingSound->SamplesPlayed += SamplesToMix;

				TotalSamplesToMix -= SamplesToMix;
			}

			SoundFinished = (PlayingSound->SamplesPlayed == LoadedSound->SampleCount);
		}
		else
		{
			LoadSound(TempState->GameAssets, PlayingSound->ID);
		}

		if(SoundFinished)
		{
			*PlayingSoundPtr = PlayingSound->Next;

			PlayingSound->NextFree = AudioState->FirstFreePlayingSound;
			AudioState->FirstFreePlayingSound = PlayingSound;
		}
		else
		{
			PlayingSoundPtr = &PlayingSound->Next;
		}
	}

	// NOTE(georgy): Convert r32 buffers to i16
	{
		i16 *SampleDest = (i16 *)SoundBuffer->Samples;
		for(u32 SampleIndex = 0;
			SampleIndex < SoundBuffer->SampleCount;
			SampleIndex++)
		{
			*SampleDest++ = (i16)(Round(*Real32Channel0++));
			*SampleDest++ = (i16)(Round(*Real32Channel1++));
		}
	}

	EndTemporaryMemory(SoundMixerMemory);
}