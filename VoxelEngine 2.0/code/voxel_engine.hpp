#include "voxel_engine.h"
#include "voxel_engine_world.hpp"
#include "voxel_engine_sim_region.hpp"

internal stored_entity *
TESTAddCube(game_state *GameState, world_position P, vec3 Dim, u32 VerticesCount, r32 *Vertices)
{
	Assert(GameState->StoredEntityCount < ArrayCount(GameState->StoredEntities));
	u32 EntityIndex = GameState->StoredEntityCount++;

	stored_entity *StoredEntity = GameState->StoredEntities + EntityIndex;
	*StoredEntity = {};
	StoredEntity->VerticesCount = VerticesCount;
	for(u32 VertexIndex = 0;
		VertexIndex < VerticesCount;
		VertexIndex++)
	{
		StoredEntity->Vertices[VertexIndex] = vec3(Vertices[VertexIndex*3], Vertices[VertexIndex*3 + 1], Vertices[VertexIndex*3 + 2]);
	}
	StoredEntity->Sim.StorageIndex = EntityIndex;
	StoredEntity->Sim.Type = (entity_type)10000;
	StoredEntity->Sim.Dim = Dim;
	StoredEntity->P = InvalidPosition();
	StoredEntity->Sim.Collides = true;

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
AddFireball(game_state *GameState, vec3 Dim)
{
	add_stored_entity_result Entity = AddStoredEntity(GameState, EntityType_Fireball, InvalidPosition());

	Entity.StoredEntity->Sim.Moveable = true;
	Entity.StoredEntity->Sim.NonSpatial = true;
	Entity.StoredEntity->Sim.Collides = true;
	Entity.StoredEntity->Sim.Rotation = 0.0f;
	Entity.StoredEntity->Sim.Dim = Dim;

	return(Entity.StorageIndex);
}

internal stored_entity *
AddHero(game_state *GameState, world_position P, vec3 Dim)
{
	add_stored_entity_result Entity = AddStoredEntity(GameState, EntityType_Hero, P);

	Entity.StoredEntity->Sim.Moveable = true;
	Entity.StoredEntity->Sim.Collides = true;
	Entity.StoredEntity->Sim.GravityAffected = true;
	Entity.StoredEntity->Sim.Rotation = 0.0f;
	Entity.StoredEntity->Sim.Dim = Dim;
	Entity.StoredEntity->Sim.Fireball.StorageIndex = AddFireball(GameState, vec3(0.25f, 0.25f, 0.25f));

	return(Entity.StoredEntity);
}

internal void
GameUpdate(game_memory *Memory, game_input *Input, int Width, int Height)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!GameState->IsInitialized)
	{
		PlatformReadEntireFile = Memory->PlatformReadEntireFile;
		PlatformFreeFileMemory = Memory->PlatformFreeFileMemory;
		PlatformAllocateMemory = Memory->PlatformAllocateMemory;
		PlatformFreeMemory = Memory->PlatformFreeMemory;
		PlatformOutputDebugString = Memory->PlatformOutputDebugString;

		InitializeStackAllocator(&GameState->WorldAllocator, Memory->PermanentStorageSize - sizeof(game_state),
															 (u8 *)Memory->PermanentStorage + sizeof(game_state));

		GameState->StoredEntityCount = 0;

		GameState->Camera.DistanceFromHero = 6.0f;
		GameState->Camera.Pitch = GameState->Camera.Head = 0;
		GameState->Camera.RotSensetivity = 0.05f;
		GameState->Camera.OffsetFromHero = {};

		InitializeWorld(&GameState->World);

		CompileShader(&GameState->DefaultShader, "data/shaders/DefaultVS.glsl", "data/shaders/DefaultFS.glsl");

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

		// NOTE(georgy): Reserve slot 0
		AddStoredEntity(GameState, EntityType_Null, InvalidPosition());

		world_position TestP = {};
		TestP.ChunkY = 0;
		TestP.Offset = vec3(6.0f, 2.0f, 3.0f);
		TESTAddCube(GameState, TestP, vec3(1.0f, 1.0f, 1.0f), 36, CubeVertices);

		world_position HeroP = {};
		HeroP.ChunkY = 1;
		HeroP.Offset = vec3(0.3f, 5.0f, 3.0f);
		GameState->Hero.Entity = AddHero(GameState, HeroP, vec3(1.0f, 1.0f, 1.0f));

		GameState->IsInitialized = true;
	}

	Assert(sizeof(temp_state) <= Memory->TemporaryStorageSize);
	temp_state *TempState = (temp_state *)Memory->TemporaryStorage;
	if (!TempState->IsInitialized)
	{
		InitializeStackAllocator(&TempState->Allocator, Memory->TemporaryStorageSize - sizeof(temp_state),
														(u8 *)Memory->TemporaryStorage + sizeof(temp_state));

		TempState->IsInitialized = true;
	}

	temporary_memory RenderMemory = BeginTemporaryMemory(&TempState->Allocator);
	
	rect3 SimRegionUpdatableBounds = RectMinMax(vec3(-60.0f, -20.0f, -60.0f), vec3(60.0f, 20.0f, 60.0f));
	sim_region *SimRegion = BeginSimulation(GameState, &GameState->World, GameState->Hero.Entity->P, SimRegionUpdatableBounds, 
											&GameState->WorldAllocator, &TempState->Allocator, Input->dt);

	camera *Camera = &GameState->Camera;
	Camera->Pitch -= Input->MouseYDisplacement*Camera->RotSensetivity;
	Camera->Head += Input->MouseXDisplacement*Camera->RotSensetivity;

	Camera->Pitch = Camera->Pitch > 89.0f ? 89.0f : Camera->Pitch;
	Camera->Pitch = Camera->Pitch < -89.0f ? -89.0f : Camera->Pitch;

	r32 PitchRadians = DEG2RAD(Camera->Pitch);
	r32 HeadRadians = DEG2RAD(Camera->Head);
	r32 HorizontalDistanceFromHero = Camera->DistanceFromHero*cosf(-PitchRadians);
	r32 XOffsetFromHero = -HorizontalDistanceFromHero * sinf(HeadRadians);
	r32 YOffsetFromHero = Camera->DistanceFromHero*sinf(-PitchRadians);
	r32 ZOffsetFromHero = HorizontalDistanceFromHero * cosf(HeadRadians);
	Camera->OffsetFromHero = vec3(XOffsetFromHero, YOffsetFromHero, ZOffsetFromHero);

#if 0
	r32 CameraTargetDirX = sinf(DEG2RAD(Camera->Head))*cosf(DEG2RAD(Camera->Pitch));
	r32 CameraTargetDirY = sinf(DEG2RAD(Camera->Pitch));
	r32 CameraTargetDirZ = -cosf(DEG2RAD(Camera->Head))*cosf(DEG2RAD(Camera->Pitch));
	Camera->Front = Normalize(vec3(CameraTargetDirX, CameraTargetDirY, CameraTargetDirZ));
#endif

	vec3 Forward = Normalize(vec3(-Camera->OffsetFromHero.x(), 0.0f, -Camera->OffsetFromHero.z()));
	// vec3 Forward = Normalize(vec3(-Camera->OffsetFromHero.x(), -Camera->OffsetFromHero.y(), -Camera->OffsetFromHero.z()));
	vec3 Right = Normalize(Cross(Forward, vec3(0.0f, 1.0f, 0.0f)));
	r32 Theta = -RAD2DEG(atan2f(Forward.z(), Forward.x())) + 90.0f;
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

	UseShader(GameState->DefaultShader);	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	mat4 View = RotationMatrixFromDirection(Camera->OffsetFromHero) * Translate(-Camera->OffsetFromHero);
	mat4 Projection = Perspective(45.0f, (r32)Width/Height, 0.1f, 100.0f);
	SetMat4(GameState->DefaultShader, "View", View);
	SetMat4(GameState->DefaultShader, "Projection", Projection);
	for(u32 EntityIndex = 0;
		EntityIndex < SimRegion->EntityCount;
		EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;
		r32 dt = Input->dt;
		if(Entity->Updatable)
		{
			vec3 ddP = {};
			r32 Drag = 0.0f;
			switch(Entity->Type)
			{
				case EntityType_Hero:
				{
					ddP = GameState->Hero.ddP;
					Drag = 2.0f;
					if(GameState->Hero.dY)
					{
						Entity->dP.SetY(GameState->Hero.dY);
					}

					Entity->Rotation = GameState->Hero.AdditionalRotation;

					if(GameState->Hero.Fireball)
					{
						sim_entity *Fireball = Entity->Fireball.SimPtr;
						if(Fireball && Fireball->NonSpatial)
						{
							Fireball->NonSpatial = false;
							Fireball->DistanceLimit = 8.0f;
							Fireball->P = Entity->P + vec3(0.0f, 0.5f*Entity->Dim.y(), 0.0f);
							Fireball->dP = vec3(Entity->dP.x(), 0.0f, Entity->dP.z()) - vec3(0.0f, 0.0f, 5.0f);
						}
					}
				} break;

				case EntityType_Fireball:
				{
					if(Entity->DistanceLimit == 0.0f)
					{
						Entity->P = vec3((r32)INVALID_POSITION, (r32)INVALID_POSITION, (r32)INVALID_POSITION);
						Entity->NonSpatial = true;						
					}
				} break;
			}

			if(Entity->Moveable && !Entity->NonSpatial)
			{
				MoveEntity(GameState, SimRegion, Entity, ddP, Drag, dt, false);
				if(Entity->GravityAffected)
				{
					MoveEntity(GameState, SimRegion, Entity, ddP, Drag, dt, true);
				}
			}

			switch(Entity->Type)
			{
				case EntityType_Hero:
				{
					glBindVertexArray(GameState->CubeVAO);
					mat4 Model = Translate(vec3(0.0f, 0.5f*Entity->Dim.y(), 0.0f)) * Rotate(Entity->Rotation, vec3(0.0f, 1.0f, 0.0f));
					SetMat4(GameState->DefaultShader, "Model", Model);
					glDrawArrays(GL_TRIANGLES, 0, 36);
					glBindVertexArray(0);
				} break;

				case EntityType_Fireball:
				{
					glBindVertexArray(GameState->CubeVAO);
					mat4 Model = Translate(vec3(0.0f, 0.5f*Entity->Dim.y(), 0.0f) + Entity->P) * Scale(vec3(0.25f, 0.25f, 0.25f));
					SetMat4(GameState->DefaultShader, "Model", Model);
					glDrawArrays(GL_TRIANGLES, 0, 36);
					glBindVertexArray(0);
				} break;

				// TEST
				default:
				{
					glBindVertexArray(GameState->CubeVAO);
					mat4 Model = Translate(vec3(0.0f, 0.5f*Entity->Dim.y(), 0.0f) + Entity->P);
					SetMat4(GameState->DefaultShader, "Model", Model);
					glDrawArrays(GL_TRIANGLES, 0, 36);
					glBindVertexArray(0);
				}  break;
			}
		}
	}
	
	RenderChunks(&GameState->World, GameState->DefaultShader);
	
	EndSimulation(GameState, SimRegion, &GameState->WorldAllocator);
	EndTemporaryMemory(RenderMemory);
}