#include "voxel_engine_world_mode.h"

internal sim_entity_collision_volume *
MakeSimpleCollision(game_mode_world *WorldMode, vec3 Dim)
{
	sim_entity_collision_volume *Result = PushStruct(&WorldMode->FundamentalTypesAllocator, sim_entity_collision_volume);

	Result->Dim = Dim;
	Result->OffsetP = vec3(0.0f, 0.5f*Dim.y(), 0.0f);

	InitializeDynamicArray(&Result->VerticesP);

	vec3 Min = -0.5f*Dim;
	vec3 Points[8];
	Points[0] = Min;
	Points[1] = Points[0] + vec3(0.0f, Dim.y(), 0.0f);
	Points[2] = Points[0] + vec3(0.0f, 0.0f, Dim.z());
	Points[3] = Points[0] + vec3(0.0f, Dim.y(), Dim.z());
	Points[4] = Points[0] + vec3(Dim.x(), 0.0f, 0.0f);
	Points[5] = Points[0] + vec3(Dim.x(), Dim.y(), 0.0f);
	Points[6] = Points[0] + vec3(Dim.x(), 0.0f, Dim.z());
	Points[7] = Points[0] + vec3(Dim.x(), Dim.y(), Dim.z());

	vec3 VolumeVertices[] = 
	{
			// Back face
			Points[0],
			Points[5],
			Points[4],
			Points[5],
			Points[0],
			Points[1],

			// Front face
			Points[2],
			Points[6],
			Points[7],
			Points[7],			
			Points[3],
			Points[2],

			// Left face
			Points[3],
			Points[1],
			Points[0],
			Points[0],			
			Points[2],
			Points[3],

			// Right face
			Points[7],
			Points[4],
			Points[5],
			Points[4],
			Points[7],
			Points[6],

			// Bottom face
			Points[0],
			Points[4],
			Points[6],
			Points[6],
			Points[2],
			Points[0],

			// Top face
			Points[1],
			Points[7],
			Points[5],
			Points[7],
			Points[1],
			Points[3],			
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
AddCollisionRule(game_mode_world *WorldMode, u32 StorageIndexA, u32 StorageIndexB, bool32 CanCollide)
{
	if(StorageIndexA > StorageIndexB)
	{
		u32 Temp = StorageIndexA;
		StorageIndexA = StorageIndexB;
		StorageIndexB = Temp;
	}

	for(u32 CollisionRuleIndex = WorldMode->LastStoredCollisionRule;
		CollisionRuleIndex < ArrayCount(WorldMode->CollisionRules);
		CollisionRuleIndex++)
	{
		pairwise_collision_rule *CollisionRule = WorldMode->CollisionRules + CollisionRuleIndex;
		if((CollisionRule->StorageIndexA == 0) &&
		   (CollisionRule->StorageIndexB == 0))
		{
			CollisionRule->StorageIndexA = StorageIndexA;
			CollisionRule->StorageIndexB = StorageIndexB;
			CollisionRule->CanCollide = CanCollide;
			WorldMode->LastStoredCollisionRule = CollisionRuleIndex;
			return;
		}
	}

	for(u32 CollisionRuleIndex = 0;
		CollisionRuleIndex < WorldMode->LastStoredCollisionRule;
		CollisionRuleIndex++)
	{
		pairwise_collision_rule *CollisionRule = WorldMode->CollisionRules + CollisionRuleIndex;
		if((CollisionRule->StorageIndexA == 0) &&
		   (CollisionRule->StorageIndexB == 0))
		{
			CollisionRule->StorageIndexA = StorageIndexA;
			CollisionRule->StorageIndexB = StorageIndexB;
			CollisionRule->CanCollide = CanCollide;
			WorldMode->LastStoredCollisionRule = CollisionRuleIndex;
			return;
		}
	}

	Assert(!"NO SPACE FOR NEW COLLISION RULE!");
}

internal void
ClearCollisionRulesFor(game_mode_world *WorldMode, u32 StorageIndex)
{
	for(u32 CollisionRuleIndex = 0;
		CollisionRuleIndex < ArrayCount(WorldMode->CollisionRules);
		CollisionRuleIndex++)
	{
		pairwise_collision_rule *CollisionRule = WorldMode->CollisionRules + CollisionRuleIndex;
		if((CollisionRule->StorageIndexA == StorageIndex) ||
		   (CollisionRule->StorageIndexB == StorageIndex))
		{
			CollisionRule->StorageIndexA = CollisionRule->StorageIndexB = 0;
		}
	}
}

internal void
AddParticlesToEntity(game_mode_world *WorldMode, stored_entity *Entity, 
					 r32 MaxLifeTime, u32 SpawnInSecond, r32 Scale,
					 vec3 StartPRanges, vec3 StartdPRanges, r32 dPY, vec3 StartddP, 
					 vec3 Color)
{
	// TODO(georgy): Allow multiple particle systems per an entity
	if(!Entity->Sim.ParticlesInfo)
	{
		// TODO(georgy): Think about unloading this
		Entity->Sim.ParticlesInfo = PushStruct(&WorldMode->FundamentalTypesAllocator, particle_emitter_info);
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

internal void
AddPointLightToEntity(game_mode_world *WorldMode, stored_entity *Entity,
					  vec3 OffsetFromEntity, vec3 Color)
{
	// TODO(georgy): Allow multiple point lights per an entity
	if(!Entity->Sim.PointLight)
	{
		// TODO(georgy): Think about unloading this
		Entity->Sim.PointLight = PushStruct(&WorldMode->FundamentalTypesAllocator, point_light);
		Entity->Sim.PointLight->P = OffsetFromEntity;
		Entity->Sim.PointLight->Color = Color;
	}
}

struct add_stored_entity_result
{
	stored_entity *StoredEntity;
	u32 StorageIndex;
};
inline add_stored_entity_result
AddStoredEntity(game_mode_world *WorldMode, entity_type Type, world_position P)
{
	add_stored_entity_result Result;
	
	Assert(WorldMode->StoredEntityCount < ArrayCount(WorldMode->StoredEntities));
	Result.StorageIndex = WorldMode->StoredEntityCount++;

	Result.StoredEntity = WorldMode->StoredEntities + Result.StorageIndex;
	*Result.StoredEntity = {};
	Result.StoredEntity->Sim.StorageIndex = Result.StorageIndex;
	Result.StoredEntity->Sim.Type = Type;
	Result.StoredEntity->P = InvalidPosition();

	ChangeEntityLocation(&WorldMode->World, &WorldMode->WorldAllocator, Result.StorageIndex, Result.StoredEntity, P);

	return(Result);
}

internal u32
AddFireball(game_mode_world *WorldMode)
{
	add_stored_entity_result Entity = AddStoredEntity(WorldMode, EntityType_Fireball, InvalidPosition());

	AddFlags(&Entity.StoredEntity->Sim, EntityFlag_Moveable | EntityFlag_NonSpatial | EntityFlag_Collides);
	Entity.StoredEntity->Sim.Collision = WorldMode->FireballCollision;

	return(Entity.StorageIndex);
}

internal u32
AddSword(game_mode_world *WorldMode)
{
	add_stored_entity_result Entity = AddStoredEntity(WorldMode, EntityType_Sword, InvalidPosition());

	AddFlags(&Entity.StoredEntity->Sim, EntityFlag_NonSpatial | EntityFlag_Collides);
	Entity.StoredEntity->Sim.Collision = WorldMode->HeroSwordCollision;

	return(Entity.StorageIndex);
}

internal stored_entity *
AddHero(game_mode_world *WorldMode, world_position P)
{
	add_stored_entity_result Entity = AddStoredEntity(WorldMode, EntityType_Hero, P);

	Entity.StoredEntity->Sim.CanBeGravityAffected = true;
	AddFlags(&Entity.StoredEntity->Sim, EntityFlag_Moveable | EntityFlag_Collides | EntityFlag_GravityAffected);
	Entity.StoredEntity->Sim.HitPoints = 100;
	Entity.StoredEntity->Sim.MaxHitPoints = 100;
	Entity.StoredEntity->Sim.ManaPoints = 87;
	Entity.StoredEntity->Sim.MaxManaPoints = 100;
	Entity.StoredEntity->Sim.Collision = WorldMode->HeroCollision;
	Entity.StoredEntity->Sim.Fireball.StorageIndex = AddFireball(WorldMode);
	Entity.StoredEntity->Sim.Sword.StorageIndex = AddSword(WorldMode);

	AddParticlesToEntity(WorldMode, &WorldMode->StoredEntities[Entity.StoredEntity->Sim.Fireball.StorageIndex], 
						 1.0f, 40, 1.0f, vec3(1.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 1.0f), 4.0f, vec3(0.0f, -9.8f, 0.0f),
						 vec3(1.0f, 0.078f, 0.098f));
	AddPointLightToEntity(WorldMode, &WorldMode->StoredEntities[Entity.StoredEntity->Sim.Fireball.StorageIndex], 
						  vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f));

	return(Entity.StoredEntity);
}

internal stored_entity *
AddMonster(game_mode_world *WorldMode, world_position P)
{
	add_stored_entity_result Entity = AddStoredEntity(WorldMode, EntityType_Monster, P);

	Entity.StoredEntity->Sim.CanBeGravityAffected = true;
	AddFlags(&Entity.StoredEntity->Sim, EntityFlag_Moveable | EntityFlag_Collides | EntityFlag_GravityAffected);
	Entity.StoredEntity->Sim.HitPoints = 100;
	Entity.StoredEntity->Sim.MaxHitPoints = 100;
	Entity.StoredEntity->Sim.Collision = WorldMode->HeroCollision;
	Entity.StoredEntity->Sim.Sword.StorageIndex = AddSword(WorldMode);

	return(Entity.StoredEntity);
}

internal void
PlayWorld(game_state *GameState)
{
	SetGameMode(GameState, GameMode_World);

	game_mode_world *WorldMode = PushStruct(&GameState->ModeAllocator, game_mode_world);
	ZeroSize(WorldMode, sizeof(game_mode_world));

	SubAllocator(&WorldMode->WorldAllocator, &GameState->ModeAllocator, GetAllocatorSizeRemaining(&GameState->ModeAllocator));
	SubAllocator(&WorldMode->FundamentalTypesAllocator, &WorldMode->WorldAllocator, Megabytes(64));

	WorldMode->Camera = {};
	WorldMode->Camera.DistanceFromHero = 9.0f;
	WorldMode->Camera.RotSensetivity = 0.1f;
	WorldMode->Camera.NearDistance = 0.1f;
	WorldMode->Camera.FarDistance = 160.0f;
	WorldMode->Camera.FoV = 45.0f;
	// TODO(georgy): Get this from hero head model or smth
	// NOTE(georgy): This is hero head centre offset from hero feet pos. 
	// 				 We use this to offset all object in view matrix. So Camera->OffsetFromHero is offset from hero head centre  
	WorldMode->Camera.TargetOffset = vec3(0.0f, 0.680000007f + 0.1f, 0.0f);
	WorldMode->Camera.DEBUGFront = vec3(0.0f, 0.0f, -1.0f);

	InitializeWorld(&WorldMode->World);

	WorldMode->HeroCollision = MakeSimpleCollision(WorldMode, vec3(0.54f, 0.54f, 0.48f));
	WorldMode->FireballCollision = MakeSimpleCollision(WorldMode, vec3(0.25f, 0.25f, 0.25f));
	WorldMode->HeroSwordCollision = MakeSimpleCollision(WorldMode, vec3(1.0f, 0.5f, 0.5f));

	CompileShader(&WorldMode->CharacterShader, CharacterVS, CharacterFS);
	CompileShader(&WorldMode->WorldShader, WorldVS, WorldFS);
	CompileShader(&WorldMode->WaterShader, WaterVS, WaterFS);
	CompileShader(&WorldMode->HitpointsShader, HitpointsVS, HitpointsFS);
	CompileShader(&WorldMode->BlockParticleShader, BlockParticleVS, BlockParticleFS);
	CompileShader(&WorldMode->CharacterDepthShader, CharacterDepthVS, EmptyFS);
	CompileShader(&WorldMode->WorldDepthShader, WorldDepthVS, EmptyFS);
	CompileShader(&WorldMode->BlockParticleDepthShader, BlockParticleDepthVS, EmptyFS);
	CompileShader(&WorldMode->UIQuadShader, UI_VS, UI_FS);
	CompileShader(&WorldMode->UIGlyphShader, GlyphVS, GlyphFS);
	CompileShader(&WorldMode->FramebufferScreenShader, FramebufferScreenVS, FramebufferScreenFS);

	GenerateUBO(&WorldMode->UBOs[BindingPoint_Matrices], 5*sizeof(mat4), BindingPoint_Matrices);
	GenerateUBO(&WorldMode->UBOs[BindingPoint_UIMatrices], sizeof(mat4), BindingPoint_UIMatrices);
	GenerateUBO(&WorldMode->UBOs[BindingPoint_DirectionalLightInfo], 2*sizeof(vec3), BindingPoint_DirectionalLightInfo);
	GenerateUBO(&WorldMode->UBOs[BindingPoint_PointLightInfo], sizeof(point_lights_info), BindingPoint_PointLightInfo);
	GenerateUBO(&WorldMode->UBOs[BindingPoint_ShadowsInfo], 352, BindingPoint_ShadowsInfo);

	BindUniformBlockToBindingPoint(WorldMode->WorldShader, "Matrices", BindingPoint_Matrices);
	BindUniformBlockToBindingPoint(WorldMode->WaterShader, "Matrices", BindingPoint_Matrices);
	BindUniformBlockToBindingPoint(WorldMode->CharacterShader, "Matrices", BindingPoint_Matrices);
	BindUniformBlockToBindingPoint(WorldMode->BlockParticleShader, "Matrices", BindingPoint_Matrices);

	BindUniformBlockToBindingPoint(WorldMode->UIQuadShader, "Matrices", BindingPoint_UIMatrices);
	BindUniformBlockToBindingPoint(WorldMode->UIGlyphShader, "Matrices", BindingPoint_UIMatrices);

	BindUniformBlockToBindingPoint(WorldMode->WorldShader, "DirectionalLightInfo", BindingPoint_DirectionalLightInfo);
	BindUniformBlockToBindingPoint(WorldMode->WaterShader, "DirectionalLightInfo", BindingPoint_DirectionalLightInfo);
	BindUniformBlockToBindingPoint(WorldMode->CharacterShader, "DirectionalLightInfo", BindingPoint_DirectionalLightInfo);
	BindUniformBlockToBindingPoint(WorldMode->BlockParticleShader, "DirectionalLightInfo", BindingPoint_DirectionalLightInfo);

	BindUniformBlockToBindingPoint(WorldMode->WorldShader, "PointLightsInfo", BindingPoint_PointLightInfo);
	BindUniformBlockToBindingPoint(WorldMode->WaterShader, "PointLightsInfo", BindingPoint_PointLightInfo);
	BindUniformBlockToBindingPoint(WorldMode->CharacterShader, "PointLightsInfo", BindingPoint_PointLightInfo);
	BindUniformBlockToBindingPoint(WorldMode->BlockParticleShader, "PointLightsInfo", BindingPoint_PointLightInfo);

	BindUniformBlockToBindingPoint(WorldMode->WorldShader, "ShadowsInfo", BindingPoint_ShadowsInfo);
	BindUniformBlockToBindingPoint(WorldMode->WaterShader, "ShadowsInfo", BindingPoint_ShadowsInfo);
	BindUniformBlockToBindingPoint(WorldMode->CharacterShader, "ShadowsInfo", BindingPoint_ShadowsInfo);
	BindUniformBlockToBindingPoint(WorldMode->BlockParticleShader, "ShadowsInfo", BindingPoint_ShadowsInfo);

	particle_generator *ParticleGenerator = &WorldMode->ParticleGenerator;
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
	glGenVertexArrays(1, &WorldMode->CubeVAO);
	glGenBuffers(1, &WorldMode->CubeVBO);
	glBindVertexArray(WorldMode->CubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, WorldMode->CubeVBO);
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
	glGenVertexArrays(1, &WorldMode->QuadVAO);
	glGenBuffers(1, &WorldMode->QuadVBO);
	glBindVertexArray(WorldMode->QuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, WorldMode->QuadVBO);
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
	glGenVertexArrays(1, &WorldMode->UIQuadVAO);
	glGenBuffers(1, &WorldMode->UIQuadVBO);
	glBindVertexArray(WorldMode->UIQuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, WorldMode->UIQuadVBO);
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
	glGenVertexArrays(1, &WorldMode->UIGlyphVAO);
	glGenBuffers(1, &WorldMode->UIGlyphVBO);
	glBindVertexArray(WorldMode->UIGlyphVAO);
	glBindBuffer(GL_ARRAY_BUFFER, WorldMode->UIGlyphVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(UIGlyphVertices), UIGlyphVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void *)0);
	glBindVertexArray(0);

	
	// NOTE(georgy): Reserve slot 0
	AddStoredEntity(WorldMode, EntityType_Null, InvalidPosition());

	world_position HeroP = {};
	HeroP.ChunkY = MAX_CHUNKS_Y;
	HeroP.Offset = vec3(0.3f, 5.0f, 3.0f);
	WorldMode->Hero.Entity = AddHero(WorldMode, HeroP);
	WorldMode->LastHeroWorldP = HeroP;
	// AddPointLightToEntity(WorldMode, WorldMode->Hero.Entity,
	// 						vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f));
	// AddParticlesToEntity(WorldMode, WorldMode->Hero.Entity,
	// 					 1.5f, 30, 0.2f, vec3(0.0f, 0.0f, 0.0f), vec3(0.65f, 0.0f, 0.65f), 2.0f, vec3(0.0f, 0.0f, 0.0f),
	// 					 vec3(0.25f, 0.25f, 0.25f));

	world_position MonsterP = {};
	MonsterP.ChunkY = MAX_CHUNKS_Y;
	MonsterP.Offset = vec3(0.0f, 5.0f, 0.0f);
	AddMonster(WorldMode, MonsterP);

	glGenFramebuffers(1, &WorldMode->ShadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, WorldMode->ShadowMapFBO);
	WorldMode->ShadowMapsWidth = WorldMode->ShadowMapsHeight = 2048;

	glGenTextures(1, &WorldMode->ShadowMapsArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, WorldMode->ShadowMapsArray);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, WorldMode->ShadowMapsWidth, WorldMode->ShadowMapsHeight, 
					CASCADES_COUNT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, WorldMode->ShadowMapsArray, 0);
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

			WorldMode->ShadowSamplesOffsets[Y*4 + X] = DiskP;
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
	glGenTextures(1, &WorldMode->ShadowNoiseTexture);
	glBindTexture(GL_TEXTURE_2D, WorldMode->ShadowNoiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 8, 8, 0, GL_RG, GL_FLOAT, ShadowNoise);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	UseShader(WorldMode->WorldShader);
	SetInt(WorldMode->WorldShader, "ShadowMaps", 0);
	SetInt(WorldMode->WorldShader, "ShadowNoiseTexture", 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, WorldMode->ShadowMapsArray);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, WorldMode->ShadowNoiseTexture);

	UseShader(WorldMode->WaterShader);
	SetInt(WorldMode->WaterShader, "ShadowMaps", 0);
	SetInt(WorldMode->WaterShader, "ShadowNoiseTexture", 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, WorldMode->ShadowMapsArray);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, WorldMode->ShadowNoiseTexture);
	
	UseShader(WorldMode->CharacterShader);
	SetInt(WorldMode->CharacterShader, "ShadowMaps", 0);
	SetInt(WorldMode->CharacterShader, "ShadowNoiseTexture", 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, WorldMode->ShadowMapsArray);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, WorldMode->ShadowNoiseTexture);
	
	UseShader(WorldMode->BlockParticleShader);
	SetInt(WorldMode->BlockParticleShader, "ShadowMaps", 0);
	SetInt(WorldMode->BlockParticleShader, "ShadowNoiseTexture", 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, WorldMode->ShadowMapsArray);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, WorldMode->ShadowNoiseTexture);
		
	UseShader(WorldMode->UIQuadShader);
	SetInt(WorldMode->UIQuadShader, "Texture", 0);

	UseShader(WorldMode->UIGlyphShader);
	SetInt(WorldMode->UIGlyphShader, "Texture", 0);

	InitializeDefaultCharacterAnimations(WorldMode->CharacterAnimations);

	GameState->WorldMode = WorldMode;
}

internal bool32
UpdateAndRenderWorld(game_state *GameState, game_mode_world *WorldMode, temp_state *TempState, game_input *Input,
                     int BufferWidth, int BufferHeight, bool32 GameProfilingPause, bool32 DebugCamera, game_input *DebugCameraInput)
{
    world *World = &WorldMode->World;
	camera *Camera = &WorldMode->Camera;

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
	WorldMode->Hero.ddP = vec3(0.0f, 0.0f, 0.0f);

	// NOTE(georgy): Hero input stuff
#if VOXEL_ENGINE_INTERNAl
	if(DEBUGGlobalPlaybackInfo.PlaybackPhase || (!GameProfilingPause && !DebugCamera))
#endif
	{
		if (Input->MoveForward.EndedDown)
		{
			WorldMode->Hero.ddP += Forward;
			WorldMode->Hero.AdditionalRotation = Theta;
		}
		if (Input->MoveBack.EndedDown)
		{
			WorldMode->Hero.ddP -= Forward;
			WorldMode->Hero.AdditionalRotation = Theta - 180.0f;
		}
		if (Input->MoveRight.EndedDown)
		{
			WorldMode->Hero.ddP += Right;
			WorldMode->Hero.AdditionalRotation = Theta - 90.0f;
		}
		if (Input->MoveLeft.EndedDown)
		{
			WorldMode->Hero.ddP -= Right;
			WorldMode->Hero.AdditionalRotation = Theta + 90.0f;
		}

		WorldMode->Hero.dY = 0.0f;
		if(Input->MoveUp.EndedDown)
		{
			WorldMode->Hero.dY = 3.0f;
		}

		if (Input->MoveForward.EndedDown && Input->MoveRight.EndedDown)
		{
			WorldMode->Hero.AdditionalRotation = Theta - 45.0f;
		}
		if (Input->MoveForward.EndedDown && Input->MoveLeft.EndedDown)
		{
			WorldMode->Hero.AdditionalRotation = Theta + 45.0f;
		}
		if (Input->MoveBack.EndedDown && Input->MoveRight.EndedDown)
		{
			WorldMode->Hero.AdditionalRotation = Theta - 135.0f;
		}
		if (Input->MoveBack.EndedDown && Input->MoveLeft.EndedDown)
		{
			WorldMode->Hero.AdditionalRotation = Theta + 135.0f;
		}

		WorldMode->Hero.Attack = Input->MouseLeft.EndedDown;
		if(WorldMode->Hero.Attack)
		{
			WorldMode->Hero.AdditionalRotation = Theta;
		}

		WorldMode->Hero.Fireball = WasDown(&Input->MouseRight);
		if(WorldMode->Hero.Fireball)
		{
			WorldMode->Hero.AdditionalRotation = Theta;
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

						GenerateChunkVertices(World, Chunk);

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
			UpdateChunk(World, Chunk);
		}

		DEBUGGlobalPlaybackInfo.RefreshNow = false;
	}
#endif

    
	DEBUG_VARIABLE(r32, SimBoundsRadius, World);
	rect3 SimRegionUpdatableBounds = RectMinMax(vec3(-SimBoundsRadius, -SimBoundsRadius, -SimBoundsRadius), 
												vec3(SimBoundsRadius, SimBoundsRadius, SimBoundsRadius));

	vec3 HeroPosDifferenceBetweenFrames = Substract(World, &WorldMode->Hero.Entity->P, &WorldMode->LastHeroWorldP);
	HeroPosDifferenceBetweenFrames = Lerp(vec3(0.0f, 0.0f, 0.0f), HeroPosDifferenceBetweenFrames, 8.0f*Input->dt);
	world_position HeroWorldP = MapIntoChunkSpace(World, &WorldMode->LastHeroWorldP, HeroPosDifferenceBetweenFrames);
	sim_region *SimRegion = BeginSimulation(WorldMode, HeroWorldP, 
											SimRegionUpdatableBounds, &TempState->Allocator, Input->dt);
	WorldMode->LastHeroWorldP = HeroWorldP;

	Camera->RotationMatrix = RotationMatrixFromDirection(Camera->OffsetFromHero);
#if !defined(VOXEL_ENGINE_DEBUG_BUILD)
	CameraCollisionDetection(World, Camera, &WorldMode->Hero.Entity->P);
#endif
	SetupChunksBlocks(World, &WorldMode->WorldAllocator, TempState);								
	SetupChunksVertices(World, TempState);
	LoadChunks(World);
	CorrectChunksWaterLevel(World);
	
	point_lights_info PointLightsInfo = {};
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

					MoveSpec.ddP = WorldMode->Hero.ddP;
					MoveSpec.Speed = 8.0f;
					if(IsSet(Entity, EntityFlag_InWater))
					{
						MoveSpec.Speed *= 0.5f;
					}
					MoveSpec.Drag = 2.0f;
					if(WorldMode->Hero.dY && IsSet(Entity, EntityFlag_OnGround))
					{
						Entity->dP.SetY(WorldMode->Hero.dY);
					}

					Entity->Rotation = WorldMode->Hero.AdditionalRotation;

					if(WorldMode->Hero.Fireball)
					{
						sim_entity *Fireball = Entity->Fireball.SimPtr;
						if(Fireball && IsSet(Fireball, EntityFlag_NonSpatial))
						{
							ClearFlags(Fireball, EntityFlag_NonSpatial);
							Fireball->DistanceLimit = 8.0f;
							Fireball->P = Entity->P + vec3(0.0f, 0.5f, 0.0f);
							Fireball->dP = vec3(Entity->dP.x(), 0.0f, Entity->dP.z()) + 3.0f*Forward;

							Fireball->Rotation = Entity->Rotation;

							PlaySound(&GameState->AudioState, GetFirstSoundFromType(TempState->GameAssets, AssetType_Fireball));
						}
					}

					if(WorldMode->Hero.Attack)
					{
						SwordAttack(WorldMode, &GameState->AudioState, TempState->GameAssets, Entity, 0.5f*Forward);
					}

					UpdateCharacterAnimations(Entity, &MoveSpec, dt);
				} break;

				case EntityType_Monster:
				{
					ClearFlags(Entity, EntityFlag_GravityAffected);

					vec3 FromEntityToHero = WorldMode->Hero.Entity->Sim.P - Entity->P;
					r32 Theta = RAD2DEG(ATan2(-FromEntityToHero.z(), FromEntityToHero.x())) + 90.0f;
					Entity->Rotation = Theta;
					if(Length(FromEntityToHero) > 2.0f)
					{
						MoveSpec.ddP = Normalize(FromEntityToHero);
						MoveSpec.Speed = 4.0f;
						if(IsSet(Entity, EntityFlag_InWater))
						{
							MoveSpec.Speed *= 0.5f;
						}
						MoveSpec.Drag = 4.0f;
					}
					else
					{
						SwordAttack(WorldMode, &GameState->AudioState, TempState->GameAssets, Entity, 0.5f*Normalize(FromEntityToHero));
					}

					UpdateCharacterAnimations(Entity, &MoveSpec, dt);
				} break;

				case EntityType_Fireball:
				{
					if(Entity->DistanceLimit == 0.0f)
					{
						MakeEntityNonSpatial(Entity);
						ClearCollisionRulesFor(WorldMode, Entity->StorageIndex);
					}
				} break;

				case EntityType_Sword:
				{
					Entity->TimeLimit -= Input->dt;
					if(Entity->TimeLimit <= 0.0f)
					{
						MakeEntityNonSpatial(Entity);
						ClearCollisionRulesFor(WorldMode, Entity->StorageIndex);
					}
				} break;

				InvalidDefaultCase;
			}

			if(Entity->ParticlesInfo)
			{
				particle_emitter_info *EntityParticlesInfo = Entity->ParticlesInfo;
				world_position EntityWorldPosition = WorldMode->StoredEntities[Entity->StorageIndex].P;
				SpawnParticles(&WorldMode->ParticleGenerator, EntityWorldPosition, *Entity->ParticlesInfo, dt);
			}

			if(Entity->PointLight)
			{
				point_light PointLight = *Entity->PointLight;
				PointLight.P += Entity->P;
				PointLightsInfo.PointLights[PointLightsInfo.Count++] = PointLight;
			}

			if(IsSet(Entity, EntityFlag_Moveable))
			{
				MoveEntity(WorldMode, SimRegion, Entity, MoveSpec, dt, false);
				MoveEntity(WorldMode, SimRegion, Entity, MoveSpec, dt, true);
			}
		}
	}
	ParticlesUpdate(&WorldMode->ParticleGenerator, Input->dt);

	DEBUG_VARIABLE(bool32, ShowDebugDrawings, Rendering);

	WorldMode->DirectionalLightDir = Normalize(vec3(-0.123566f, -0.9712314f, 0.0f));
	if (ShowDebugDrawings)
	{
		DEBUGRenderLine(vec3(0.0f, 0.0f, 0.0f), -WorldMode->DirectionalLightDir, vec3(1.0f, 1.0f, 0.0f));
	}
	WorldMode->DirectionalLightColor = vec3(0.666666, 0.788235, 0.79215);

	// NOTE(georgy): Directional shadow map rendering
	r32 CascadesDistances[CASCADES_COUNT + 1] = {Camera->NearDistance, 25.0f, 50.0f, 75.0f};
	mat4 LightSpaceMatrices[CASCADES_COUNT];
	DEBUG_IF(RenderShadows, Rendering)
	{
		BEGIN_BLOCK(ShadowStuff);

		glViewport(0, 0, WorldMode->ShadowMapsWidth, WorldMode->ShadowMapsHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, WorldMode->ShadowMapFBO);
		for(u32 CascadeIndex = 0;
			CascadeIndex < CASCADES_COUNT;
			CascadeIndex++)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, WorldMode->ShadowMapsArray, 0, CascadeIndex);
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
			vec3 LightP = SphereP - WorldMode->DirectionalLightDir*BackupDistance;
			
			mat4 LightView = LookAt(LightP, SphereP);
			mat4 LightProjection = Ortho(-Radius, Radius, -Radius, Radius,
										 NearClip, FarClip);

			LightSpaceMatrices[CascadeIndex] = LightProjection * LightView;

			world_position HeroWorldP = WorldMode->Hero.Entity->P;
			chunk *HeroChunk = GetChunk(World, HeroWorldP.ChunkX, HeroWorldP.ChunkY, HeroWorldP.ChunkZ);
			vec4 HeroChunkP = vec4(HeroChunk->Translation, 1.0f);
			vec3 ShadowOrigin = vec3((LightSpaceMatrices[CascadeIndex] * HeroChunkP).m);
			ShadowOrigin *= 0.5f*WorldMode->ShadowMapsWidth;
			vec2 RoundedOrigin = vec2(Round(ShadowOrigin.x()), Round(ShadowOrigin.y()));
			vec2 Rounding = RoundedOrigin - vec2(ShadowOrigin.x(), ShadowOrigin.y());
			Rounding *= 2.0f / WorldMode->ShadowMapsWidth;
			mat4 RoundMatrix = Translate(vec3(Rounding.x, Rounding.y, 0.0f));
			LightSpaceMatrices[CascadeIndex] = RoundMatrix*LightSpaceMatrices[CascadeIndex];

			UseShader(WorldMode->WorldDepthShader);
			SetMat4(WorldMode->WorldDepthShader, "ViewProjection", LightSpaceMatrices[CascadeIndex]);
			UseShader(WorldMode->CharacterDepthShader);
			SetMat4(WorldMode->CharacterDepthShader, "ViewProjection", LightSpaceMatrices[CascadeIndex]);
			UseShader(WorldMode->BlockParticleDepthShader);
			SetMat4(WorldMode->BlockParticleDepthShader, "ViewProjection", LightSpaceMatrices[CascadeIndex]);

			RenderEntities(WorldMode, TempState, SimRegion, WorldMode->WorldDepthShader, 
						   WorldMode->CharacterDepthShader, WorldMode->HitpointsShader, Right);
			RenderChunks(World, WorldMode->WorldDepthShader, WorldMode->WaterShader, LightSpaceMatrices[CascadeIndex], Camera->OffsetFromHero);
			RenderParticles(&WorldMode->ParticleGenerator, World, &TempState->Allocator, 
						    WorldMode->BlockParticleDepthShader, WorldMode->Hero.Entity->P);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		END_BLOCK(ShadowStuff);
	}

	// NOTE(georgy): World and entities rendering
	glViewport(0, 0, BufferWidth, BufferHeight);
	glClearColor(0.0f, SquareRoot(0.175f), SquareRoot(0.375f), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	mat4 View = Camera->RotationMatrix * 
				Translate(-Camera->OffsetFromHero - Camera->TargetOffset);
	mat4 Projection = Perspective(Camera->FoV, Camera->AspectRatio, Camera->NearDistance, Camera->FarDistance);
	mat4 ViewProjection = Projection * View;
	if(!DebugCamera)
	{
		GlobalViewProjection = ViewProjection;
	}
	else
	{
		mat4 DEBUGCameraView = LookAt(Camera->DEBUGP, Camera->DEBUGP + Camera->DEBUGFront);
		mat4 DEBUGCameraViewProjection = Projection * DEBUGCameraView;
		GlobalViewProjection = DEBUGCameraViewProjection;
	}

	glBindBuffer(GL_UNIFORM_BUFFER, WorldMode->UBOs[BindingPoint_Matrices]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &GlobalViewProjection);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), 3*sizeof(mat4), LightSpaceMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 4*sizeof(mat4), sizeof(mat4), &ViewProjection);

	glBindBuffer(GL_UNIFORM_BUFFER, WorldMode->UBOs[BindingPoint_DirectionalLightInfo]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(vec3), &WorldMode->DirectionalLightDir);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(vec3), sizeof(vec3), &WorldMode->DirectionalLightColor);

	glBindBuffer(GL_UNIFORM_BUFFER, WorldMode->UBOs[BindingPoint_PointLightInfo]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(point_lights_info), &PointLightsInfo);

	DEBUG_VARIABLE(r32, Bias, Rendering);
	glBindBuffer(GL_UNIFORM_BUFFER, WorldMode->UBOs[BindingPoint_ShadowsInfo]);
	u32 OffsetInShadowsUBO = 0;
	glBufferSubData(GL_UNIFORM_BUFFER, OffsetInShadowsUBO, sizeof(u32), &RenderShadows);
	OffsetInShadowsUBO = 16;
	for(u32 CascadeIndex = 0;
		CascadeIndex <= CASCADES_COUNT;
		CascadeIndex++, OffsetInShadowsUBO += 16)
	{
		glBufferSubData(GL_UNIFORM_BUFFER, OffsetInShadowsUBO, sizeof(r32), &CascadesDistances[CascadeIndex]);
	}
	for(u32 SampleIndex = 0;
		SampleIndex < ArrayCount(WorldMode->ShadowSamplesOffsets);
		SampleIndex++, OffsetInShadowsUBO += 16)
	{
		glBufferSubData(GL_UNIFORM_BUFFER, OffsetInShadowsUBO, sizeof(vec2), &WorldMode->ShadowSamplesOffsets[SampleIndex]);
	}
	glBufferSubData(GL_UNIFORM_BUFFER, OffsetInShadowsUBO, sizeof(r32), &Bias);
	glBufferSubData(GL_UNIFORM_BUFFER, OffsetInShadowsUBO + 4, sizeof(i32), &BufferWidth);
	glBufferSubData(GL_UNIFORM_BUFFER, OffsetInShadowsUBO + 8, sizeof(i32), &BufferHeight);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	UseShader(WorldMode->HitpointsShader);
	SetMat4(WorldMode->HitpointsShader, "ViewProjection", GlobalViewProjection);

	UseShader(WorldMode->WaterShader);
	SetMat4(WorldMode->WaterShader, "View", View);

	RenderEntities(WorldMode, TempState, SimRegion, 
				   WorldMode->WorldShader, WorldMode->CharacterShader, WorldMode->HitpointsShader,
				   Right, ShowDebugDrawings);
	RenderChunks(World, WorldMode->WorldShader, WorldMode->WaterShader, ViewProjection, Camera->OffsetFromHero);
	RenderParticles(&WorldMode->ParticleGenerator, World, &TempState->Allocator, 
					WorldMode->BlockParticleShader, WorldMode->Hero.Entity->P);

	// NOTE(georgy): Render UI
	mat4 Orthographic = Ortho(-0.5f*BufferHeight, 0.5f*BufferHeight, 
							  -0.5f*BufferWidth, 0.5f*BufferWidth,
							  0.1f, 100.0f);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer(GL_UNIFORM_BUFFER, WorldMode->UBOs[BindingPoint_UIMatrices]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &Orthographic);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	glActiveTexture(GL_TEXTURE0);
	
	UseShader(WorldMode->UIQuadShader);
	asset_tag_vector MatchVector = {};
	MatchVector.E[Tag_Color] = TagColor_Red;
	texture_id HPBarTextureIndex = GetBestMatchTextureFromType(TempState->GameAssets, AssetType_UIBar, &MatchVector);
	vec2 HPBarTextureDim = vec2(150.0f, 25.0f);
	vec2 HPBarTextureP = vec2(-0.5f*HPBarTextureDim.x - 5.0f, (-0.5f*BufferHeight + 10.0f) + 0.5f*HPBarTextureDim.y);
	RenderQuad(WorldMode->UIQuadShader, WorldMode->UIQuadVAO, TempState->GameAssets, HPBarTextureIndex, 
		   	   HPBarTextureP, HPBarTextureDim, 0.5f);

	vec2 HPBarMaxDim = HPBarTextureDim - vec2(10.0f, 10.0f);
	r32 HPBarScale = ((r32)WorldMode->Hero.Entity->Sim.HitPoints / WorldMode->Hero.Entity->Sim.MaxHitPoints);
	vec2 HPBarDim = vec2(HPBarMaxDim.x * HPBarScale, HPBarMaxDim.y);
	r32 HPDiff = 0.5f*(HPBarMaxDim.x - HPBarDim.x);
	vec2 HPBarPos = HPBarTextureP + vec2(-HPDiff, 1.0f);
	RenderQuad(WorldMode->UIQuadShader, WorldMode->UIQuadVAO,  HPBarPos, 
			   HPBarDim, vec4(1.0f, 0.0f, 0.0f, 1.0f));

	MatchVector = {};
	MatchVector.E[Tag_Color] = TagColor_Blue;
	texture_id MPBarTextureIndex = GetBestMatchTextureFromType(TempState->GameAssets, AssetType_UIBar, &MatchVector);
	vec2 MPBarTextureDim = vec2(150.0f, 25.0f);
	vec2 MPBarTextureP = vec2(0.5f*MPBarTextureDim.x + 5.0f, (-0.5f*BufferHeight + 10.0f) + 0.5f*MPBarTextureDim.y);
	RenderQuad(WorldMode->UIQuadShader, WorldMode->UIQuadVAO, TempState->GameAssets, MPBarTextureIndex, 
		   	   MPBarTextureP, MPBarTextureDim, 0.5f);

	vec2 MPBarMaxDim = MPBarTextureDim - vec2(10.0f, 10.0f);
	r32 MPBarScale = ((r32)WorldMode->Hero.Entity->Sim.ManaPoints / WorldMode->Hero.Entity->Sim.MaxManaPoints);
	vec2 MPBarDim = vec2(MPBarMaxDim.x * MPBarScale, MPBarMaxDim.y);
	r32 MPDiff = 0.5f*(MPBarMaxDim.x - MPBarDim.x);
	vec2 MPBarPos = MPBarTextureP + vec2(-MPDiff, 1.0f);
	RenderQuad(WorldMode->UIQuadShader, WorldMode->UIQuadVAO,  MPBarPos, 
			   MPBarDim, vec4(0.0f, 0.0f, 1.0f, 1.0f));

	asset_tag_vector GameFontMatchVector = { };
	GameFontMatchVector.E[Tag_FontType] = FontType_GameFont;
	WorldMode->FontID = GetBestMatchFont(TempState->GameAssets, &GameFontMatchVector);
	WorldMode->Font = GetFont(TempState->GameAssets, WorldMode->FontID);
	if(WorldMode->Font)
	{
		vec2 HitPointsP = HPBarTextureP + vec2(0.0f, 1.5f);
		rect2 SlashRect = GetTextLineRect(WorldMode->Font, vec2(0.0f, 0.0f), "/", 0.2f);
		vec2 SlashDim = GetDim(SlashRect);
		vec2 SlashScreenP = HitPointsP - 0.5f*SlashDim;
		RenderTextLine(WorldMode->Font, WorldMode->UIGlyphShader, WorldMode->UIGlyphVAO, 
					   SlashScreenP, "/", 0.2f);

		char Buffer[256];
		_snprintf_s(Buffer, sizeof(Buffer), "%i", WorldMode->Hero.Entity->Sim.MaxHitPoints);
		vec2 MaxHPScreenP = SlashScreenP + vec2(SlashDim.x, 0.0f);
		RenderTextLine(WorldMode->Font, WorldMode->UIGlyphShader, WorldMode->UIGlyphVAO, 
					   MaxHPScreenP, Buffer, 0.2f);

		_snprintf_s(Buffer, sizeof(Buffer), "%i", WorldMode->Hero.Entity->Sim.HitPoints);
		rect2 HPRect = GetTextLineRect(WorldMode->Font, vec2(0.0f, 0.0f), Buffer, 0.2f);
		vec2 HPDim = GetDim(HPRect);
		vec2 HPScreenP = SlashScreenP - vec2(HPDim.x, 0.0f);
		RenderTextLine(WorldMode->Font, WorldMode->UIGlyphShader, WorldMode->UIGlyphVAO, 
					   HPScreenP, Buffer, 0.2f);

		vec2 ManaPointsP = MPBarTextureP + vec2(0.0f, 1.5f);
		SlashScreenP = ManaPointsP - 0.5f*SlashDim;
		RenderTextLine(WorldMode->Font, WorldMode->UIGlyphShader, WorldMode->UIGlyphVAO, 
					   SlashScreenP, "/", 0.2f);

		_snprintf_s(Buffer, sizeof(Buffer), "%i", WorldMode->Hero.Entity->Sim.MaxManaPoints);
		vec2 MaxMPScreenP = SlashScreenP + vec2(SlashDim.x, 0.0f);
		RenderTextLine(WorldMode->Font, WorldMode->UIGlyphShader, WorldMode->UIGlyphVAO, 
					   MaxMPScreenP, Buffer, 0.2f);

		_snprintf_s(Buffer, sizeof(Buffer), "%i", WorldMode->Hero.Entity->Sim.ManaPoints);
		rect2 MPRect = GetTextLineRect(WorldMode->Font, vec2(0.0f, 0.0f), Buffer, 0.2f);
		vec2 MPDim = GetDim(MPRect);
		vec2 MPScreenP = SlashScreenP - vec2(MPDim.x, 0.0f);
		RenderTextLine(WorldMode->Font, WorldMode->UIGlyphShader, WorldMode->UIGlyphVAO, 
					   MPScreenP, Buffer, 0.2f);
	}
	else
	{
		LoadFont(TempState->GameAssets, WorldMode->FontID);
	}
	
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	EndSimulation(WorldMode, SimRegion, &WorldMode->WorldAllocator);

	if(WasDown(&Input->Esc))
	{
		PlayTitleScreen(GameState);
	}

	bool32 ChangeMode = false;
	return(ChangeMode);
}