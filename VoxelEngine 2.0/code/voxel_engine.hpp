#include "voxel_engine.h"
#include "voxel_engine_asset.hpp"
#include "voxel_engine_render.hpp"
#include "voxel_engine_world.hpp"
#include "voxel_engine_sim_region.hpp"

internal sim_entity_collision_volume *
MakeSimpleCollision(game_state *GameState, vec3 Dim)
{
	sim_entity_collision_volume *Result = PushStruct(&GameState->WorldAllocator, sim_entity_collision_volume);

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

	return(Entity.StoredEntity);
}

internal void
GameUpdate(game_memory *Memory, game_input *Input, int Width, int Height)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!GameState->IsInitialized)
	{
		PlatformAddEntry = Memory->PlatformAddEntry;
		PlatformCompleteAllWork = Memory->PlatformCompleteAllWork;
		PlatformReadEntireFile = Memory->PlatformReadEntireFile;
		PlatformFreeFileMemory = Memory->PlatformFreeFileMemory;
		PlatformAllocateMemory = Memory->PlatformAllocateMemory;
		PlatformFreeMemory = Memory->PlatformFreeMemory;
		PlatformOutputDebugString = Memory->PlatformOutputDebugString;

		InitializeStackAllocator(&GameState->WorldAllocator, Memory->PermanentStorageSize - sizeof(game_state),
															 (u8 *)Memory->PermanentStorage + sizeof(game_state));

		GameState->StoredEntityCount = 0;

		GameState->Camera.DistanceFromHero = 12.0f;
		GameState->Camera.Pitch = GameState->Camera.Head = 0;
		GameState->Camera.RotSensetivity = 0.05f;
		GameState->Camera.OffsetFromHero = {};

		InitializeWorld(&GameState->World);

		GameState->HeroCollision = MakeSimpleCollision(GameState, vec3(0.54f, 0.54f, 0.48f));
		GameState->FireballCollision = MakeSimpleCollision(GameState, vec3(0.25f, 0.25f, 0.25f));
		GameState->TESTCubeCollision = MakeSimpleCollision(GameState, vec3(1.0f, 1.0f, 1.0f));

		CompileShader(&GameState->CharacterShader, "data/shaders/CharacterVS.glsl", "data/shaders/CharacterFS.glsl");
		CompileShader(&GameState->WorldShader, "data/shaders/WorldVS.glsl", "data/shaders/WorldFS.glsl");
		CompileShader(&GameState->BillboardShader, "data/shaders/BillboardVS.glsl", "data/shaders/BillboardFS.glsl");

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

		// NOTE(georgy): Reserve slot 0
		AddStoredEntity(GameState, EntityType_Null, InvalidPosition());

		world_position TestP = {};
		TestP.ChunkX = 0;
		TestP.ChunkY = MAX_CHUNKS_Y;
		TestP.ChunkZ = 0;
		TestP.Offset = vec3(0.0f, 0.0f, 3.0f);
		TESTAddCube(GameState, TestP, vec3(1.0f, 1.0f, 1.0f), 36, CubeVertices);

		world_position HeroP = {};
		HeroP.ChunkX = 0;
		HeroP.ChunkY = MAX_CHUNKS_Y;
		HeroP.ChunkZ = 0;
		HeroP.Offset = vec3(0.3f, 5.0f, 3.0f);
		GameState->Hero.Entity = AddHero(GameState, HeroP);

		InitializeDefaultAnimations(GameState->CharacterAnimations);

		GameState->IsInitialized = true;
	}

	Assert(sizeof(temp_state) <= Memory->TemporaryStorageSize);
	temp_state *TempState = (temp_state *)Memory->TemporaryStorage;
	if (!TempState->IsInitialized)
	{
		InitializeStackAllocator(&TempState->Allocator, Memory->TemporaryStorageSize - sizeof(temp_state),
														(u8 *)Memory->TemporaryStorage + sizeof(temp_state));

		TempState->IsInitialized = true;

		TempState->JobSystemQueue = Memory->JobSystemQueue;
		// TempState->GameAssets = AllocateGameAssets(TempState, &TempState->Allocator, 500000);
		TempState->GameAssets = AllocateGameAssets(TempState, &TempState->Allocator, Megabytes(64));
	}

	temporary_memory RenderMemory = BeginTemporaryMemory(&TempState->Allocator);
	
	rect3 SimRegionUpdatableBounds = RectMinMax(vec3(-100.0f, -20.0f, -100.0f), vec3(100.0f, 20.0f, 100.0f));
	sim_region *SimRegion = BeginSimulation(GameState, &GameState->World, GameState->Hero.Entity->P, SimRegionUpdatableBounds, 
											&GameState->WorldAllocator, &TempState->Allocator, Input->dt);

	camera *Camera = &GameState->Camera;
	Camera->Pitch -= Input->MouseYDisplacement*Camera->RotSensetivity;
	Camera->Head += Input->MouseXDisplacement*Camera->RotSensetivity;

	Camera->Pitch = Camera->Pitch > 89.0f ? 89.0f : Camera->Pitch;
	Camera->Pitch = Camera->Pitch < -89.0f ? -89.0f : Camera->Pitch;

	r32 PitchRadians = DEG2RAD(Camera->Pitch);
	r32 HeadRadians = DEG2RAD(Camera->Head);
	r32 HorizontalDistanceFromHero = Camera->DistanceFromHero*Cos(-PitchRadians);
	r32 XOffsetFromHero = -HorizontalDistanceFromHero * Sin(HeadRadians);
	r32 YOffsetFromHero = Camera->DistanceFromHero*Sin(-PitchRadians);
	r32 ZOffsetFromHero = HorizontalDistanceFromHero * Cos(HeadRadians);
	Camera->OffsetFromHero = vec3(XOffsetFromHero, YOffsetFromHero, ZOffsetFromHero);

#if 0
	r32 CameraTargetDirX = Sin(DEG2RAD(Camera->Head))*Cos(DEG2RAD(Camera->Pitch));
	r32 CameraTargetDirY = Sin(DEG2RAD(Camera->Pitch));
	r32 CameraTargetDirZ = -Cos(DEG2RAD(Camera->Head))*Cos(DEG2RAD(Camera->Pitch));
	Camera->Front = Normalize(vec3(CameraTargetDirX, CameraTargetDirY, CameraTargetDirZ));
#endif

	// vec3 Forward = Normalize(vec3(-Camera->OffsetFromHero.x(), 0.0f, -Camera->OffsetFromHero.z()));
	vec3 Forward = Normalize(vec3(-Camera->OffsetFromHero.x(), -Camera->OffsetFromHero.y(), -Camera->OffsetFromHero.z()));
	vec3 Right = Normalize(Cross(Forward, vec3(0.0f, 1.0f, 0.0f)));
	r32 Theta = -RAD2DEG(ATan2(Forward.z(), Forward.x())) + 90.0f;
	GameState->Hero.ddP = vec3(0.0f, 0.0f, 0.0f);
	if (Input->MoveForward)
	{
		GameState->Hero.ddP += Forward;
		GameState->Hero.AdditionalRotation = Theta;
	}
	if (Input->MoveBack)
	{
		GameState->Hero.ddP -= Forward;
		GameState->Hero.AdditionalRotation = Theta - 180.0f;
	}
	if (Input->MoveRight)
	{
		GameState->Hero.ddP += Right;
		GameState->Hero.AdditionalRotation = Theta - 90.0f;
	}
	if (Input->MoveLeft)
	{
		GameState->Hero.ddP -= Right;
		GameState->Hero.AdditionalRotation = Theta + 90.0f;
	}

	GameState->Hero.dY = 0.0f;
	if(Input->MoveUp)
	{
		GameState->Hero.dY = 3.0f;
	}

	if (Input->MoveForward && Input->MoveRight)
	{
		GameState->Hero.AdditionalRotation = Theta - 45.0f;
	}
	if (Input->MoveForward && Input->MoveLeft)
	{
		GameState->Hero.AdditionalRotation = Theta + 45.0f;
	}
	if (Input->MoveBack && Input->MoveRight)
	{
		GameState->Hero.AdditionalRotation = Theta - 135.0f;
	}
	if (Input->MoveBack && Input->MoveLeft)
	{
		GameState->Hero.AdditionalRotation = Theta + 135.0f;
	}

	GameState->Hero.Fireball = Input->MouseRight;
	if(GameState->Hero.Fireball)
	{
		GameState->Hero.AdditionalRotation = Theta;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 View = RotationMatrixFromDirection(Camera->OffsetFromHero) * Translate(-Camera->OffsetFromHero);
	mat4 Projection = Perspective(45.0f, (r32)Width/Height, 0.1f, 150.0f);
	mat4 ViewProjection = Projection * View;
	shader Shaders3D[] = {GameState->WorldShader, GameState->CharacterShader, GameState->BillboardShader};
	Initialize3DTransforms(Shaders3D, ArrayCount(Shaders3D), ViewProjection);

	for(u32 EntityIndex = 0;
		EntityIndex < SimRegion->EntityCount;
		EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;
		r32 dt = Input->dt;
		if(Entity->Updatable)
		{
			UseShader(GameState->WorldShader);	
			
			vec3 ddP = {};
			r32 Drag = 0.0f;
			switch(Entity->Type)
			{
				case EntityType_Hero:
				{
					ddP = GameState->Hero.ddP;
					Drag = 2.0f;
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

					// TODO(georgy): I don't like this switch and so on. Make this code "beautiful"!
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

					switch(DesiredAnimation)
					{
						case CharacterAnimation_Run:
						{
							if(Entity->AnimationState.Type != CharacterAnimation_Run)
							{
								Entity->AnimationState.Type = CharacterAnimation_Run;
								Entity->AnimationState.Time = 0.0f;
							}
							else
							{
								AnimationTimeStep = 2.0f*Length(vec3(Entity->dP.x(), 0.0f, Entity->dP.z()))*Input->dt;
							}
						} break;

						case CharacterAnimation_Jump:
						{
							if(Entity->AnimationState.Type != CharacterAnimation_Jump)
							{
								Entity->AnimationState.Type = CharacterAnimation_Jump;
								Entity->AnimationState.Time = 0.0f;
							}
							else
							{
								AnimationTimeStep = Input->dt;
							}
						} break;

						case CharacterAnimation_Idle:
						{
							if(Entity->AnimationState.Type != CharacterAnimation_Idle)
							{
								Entity->AnimationState.Type = CharacterAnimation_Idle;	
								Entity->AnimationState.Time = 0.0f;
							}
							else
							{
								AnimationTimeStep = Input->dt;
							}
						} break;
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

			if(IsSet(Entity, EntityFlag_Moveable) && !IsSet(Entity, EntityFlag_NonSpatial))
			{
				MoveEntity(GameState, SimRegion, Entity, ddP, Drag, dt, false);
				AddFlags(Entity, EntityFlag_OnGround);
				if(IsSet(Entity, EntityFlag_GravityAffected))
				{
					//MoveEntity(GameState, SimRegion, Entity, ddP, Drag, dt, true);
				}
			}

			switch(Entity->Type)
			{
				case EntityType_Hero:
				{
					UseShader(GameState->CharacterShader);

					mat4 BoneTransformations[CharacterBone_Count];
					for(u32 BoneIndex = 0;
						BoneIndex < CharacterBone_Count;
						BoneIndex++)
					{
						BoneTransformations[BoneIndex] = GetBoneForEntity(GameState->CharacterAnimations, &Entity->AnimationState, 
																		  (character_bone_id)BoneIndex);
					}
					for(u32 BoneIndex = 0;
						BoneIndex < CharacterBone_Count;
						BoneIndex++)
					{
						if(BoneIndex != CharacterBone_Body)
						{
							BoneTransformations[BoneIndex] = BoneTransformations[BoneIndex] * BoneTransformations[CharacterBone_Body];
						}
					}
					
					SetMat4(GameState->CharacterShader, "BoneTransformations[0]", BoneTransformations[CharacterBone_Head]);
					SetMat4(GameState->CharacterShader, "BoneTransformations[1]", BoneTransformations[CharacterBone_Shoulders]);
					SetMat4(GameState->CharacterShader, "BoneTransformations[2]", BoneTransformations[CharacterBone_Body]);
					SetMat4(GameState->CharacterShader, "BoneTransformations[3]", BoneTransformations[CharacterBone_Hand_Right]);
					SetMat4(GameState->CharacterShader, "BoneTransformations[4]", BoneTransformations[CharacterBone_Hand_Left]);
					SetMat4(GameState->CharacterShader, "BoneTransformations[5]", BoneTransformations[CharacterBone_Foot_Right]);
					SetMat4(GameState->CharacterShader, "BoneTransformations[6]", BoneTransformations[CharacterBone_Foot_Left]);

					r32 HeroRotRadians = DEG2RAD(Entity->Rotation);
					vec3 FacingDir = vec3(Sin(HeroRotRadians), 0.0f, Cos(HeroRotRadians));
					vec3 HeroRight = Normalize(Cross(FacingDir, vec3(0.0f, 1.0f, 0.0f))); 
					asset_tag_vector MatchVector = { 1.0f };
					u32 HeadIndex = GetBestMatchAsset(TempState->GameAssets, AssetType_Head, &MatchVector);
					SetInt(GameState->CharacterShader, "BoneID", CharacterBone_Head);
					DrawModel(GameState->CharacterShader, TempState->GameAssets, HeadIndex, Entity->Rotation, 1.0f, HeroRight);
					SetInt(GameState->CharacterShader, "BoneID", CharacterBone_Shoulders);
					DrawModel(GameState->CharacterShader, TempState->GameAssets, TempState->GameAssets->AssetTypes[AssetType_Shoulders].FirstAssetIndex, 
							  Entity->Rotation, 1.0f, HeroRight);
					SetInt(GameState->CharacterShader, "BoneID", CharacterBone_Body);
					DrawModel(GameState->CharacterShader, TempState->GameAssets, TempState->GameAssets->AssetTypes[AssetType_Body].FirstAssetIndex,
							  Entity->Rotation, 1.0f, HeroRight);
					SetInt(GameState->CharacterShader, "BoneID", CharacterBone_Hand_Right);
					DrawModel(GameState->CharacterShader, TempState->GameAssets, TempState->GameAssets->AssetTypes[AssetType_Hand].FirstAssetIndex, 
							  Entity->Rotation, 0.8f, HeroRight);
					SetInt(GameState->CharacterShader, "BoneID", CharacterBone_Hand_Left);
					DrawModel(GameState->CharacterShader, TempState->GameAssets, TempState->GameAssets->AssetTypes[AssetType_Hand].FirstAssetIndex,
							  Entity->Rotation, 0.8f, -HeroRight);
					SetInt(GameState->CharacterShader, "BoneID", CharacterBone_Foot_Right);
					DrawModel(GameState->CharacterShader, TempState->GameAssets, TempState->GameAssets->AssetTypes[AssetType_Foot].FirstAssetIndex, 
							  Entity->Rotation, 1.4f, HeroRight);
					SetInt(GameState->CharacterShader, "BoneID", CharacterBone_Foot_Left);
					DrawModel(GameState->CharacterShader, TempState->GameAssets, TempState->GameAssets->AssetTypes[AssetType_Foot].FirstAssetIndex,
							  Entity->Rotation, 1.4f, -HeroRight);
				} break;

				case EntityType_Fireball:
				{
					mat4 Model = Translate(vec3(0.0f, 0.5f*0.25f, 0.0f) + Entity->P) * Scale(vec3(0.25f, 0.25f, 0.25f));
					SetMat4(GameState->WorldShader, "Model", Model);
					DrawFromVAO(GameState->CubeVAO, 36);
				} break;

				// TEST
				default:
				{
					mat4 Model = Translate(vec3(0.0f, 0.5f, 0.0f) + Entity->P);
					SetMat4(GameState->WorldShader, "Model", Model);
					DrawFromVAO(GameState->CubeVAO, 36);

					UseShader(GameState->BillboardShader);
					SetVec3(GameState->BillboardShader, "Color", vec3(1.0f, 0.0f, 0.0f));
					SetVec3(GameState->BillboardShader, "CameraRight", Right);
					SetVec3(GameState->BillboardShader, "BillboardSimCenterP", Entity->P + vec3(0.0f, 1.0f + 0.1f, 0.0f));
					SetVec2(GameState->BillboardShader, "Scale", vec2((r32)Entity->HitPoints / (r32)Entity->MaxHitPoints, 0.2f));
					
					glBindVertexArray(GameState->QuadVAO);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					glBindVertexArray(0);
				}  break;
			}
		}
	}
	
	RenderChunks(&GameState->World, GameState->WorldShader);
	
	EndSimulation(GameState, SimRegion, &GameState->WorldAllocator);
	EndTemporaryMemory(RenderMemory);

	UnloadAssetsIfNecessary(TempState->GameAssets);
}