#include "voxel_engine.h"
#include "voxel_engine_world.hpp"
#include "voxel_engine_sim_region.hpp"

internal stored_entity *
AddEntity(game_state *GameState, world_position P)
{
	Assert(GameState->StoredEntityCount < ArrayCount(GameState->StoredEntities));
	u32 EntityIndex = GameState->StoredEntityCount++;

	stored_entity *StoredEntity = GameState->StoredEntities + EntityIndex;
	*StoredEntity = {};
	StoredEntity->Sim.StorageIndex = EntityIndex;
	StoredEntity->P = InvalidPosition();

	ChangeEntityLocation(&GameState->World, &GameState->WorldAllocator, EntityIndex, StoredEntity, P);

	return(StoredEntity);
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

		GameState->Camera.DistanceFromHero = 12.0f;
		GameState->Camera.Pitch = GameState->Camera.Head = 0;
		GameState->Camera.RotSensetivity = 0.05f;
		GameState->Camera.OffsetFromHero = {};

		InitializeWorld(&GameState->World);

		CompileShader(&GameState->DefaultShader, "data/shaders/DefaultVS.glsl", "data/shaders/DefaultFS.glsl");

		GameState->CubeP = {};
		GameState->CubeP.Offset = vec3(3.0f, 0.0f, 3.0f);
		GameState->Hero = AddEntity(GameState, GameState->CubeP);
		GameState->CubeAdditionalRotation = 0.0f;
		r32 CubeVertices[] = {
			// Back face
			-1.0f, -1.0f, -1.0f,
			1.0f,  1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			// Front face
			-1.0f, -1.0f,  1.0f,
			1.0f, -1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,
			// Left face
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			// Right face
			1.0f,  1.0f,  1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f,  1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f, -1.0f,  1.0f,
			// Bottom face
			-1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f,  1.0f,
			1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			// Top face
			-1.0f,  1.0f, -1.0f,
			1.0f,  1.0f , 1.0f,
			1.0f,  1.0f, -1.0f,
			1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
		};
		glGenVertexArrays(1, &GameState->CubeVAO);
		glGenBuffers(1, &GameState->CubeVBO);
		glBindVertexArray(GameState->CubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, GameState->CubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(CubeVertices), CubeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(r32), (void *)0);
		glBindVertexArray(0);

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
	
	sim_region *SimRegion = BeginSimulation(GameState, &GameState->World, GameState->Hero->P, vec3(60.0f, 10.0f, 60.0f), 
											&GameState->WorldAllocator, &TempState->Allocator);

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

	for(u32 EntityIndex = 0;
		EntityIndex < SimRegion->EntityCount;
		EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;

		//vec3 Forward = Normalize(vec3(-Camera->OffsetFromHero.x(), 0.0f, -Camera->OffsetFromHero.z()));
		vec3 Forward = Normalize(vec3(-Camera->OffsetFromHero.x(), -Camera->OffsetFromHero.y(), -Camera->OffsetFromHero.z()));
		vec3 Right = Normalize(Cross(Forward, vec3(0.0f, 1.0f, 0.0f)));
		r32 Theta = -RAD2DEG(atan2f(Forward.z(), Forward.x())) + 90.0f;
		if (Input->MoveForward)
		{
			Entity->P += 15.0f*Forward*Input->dt;
			GameState->CubeAdditionalRotation = Theta;
		}
		if (Input->MoveBack)
		{
			Entity->P -= 15.0f*Forward*Input->dt;
			GameState->CubeAdditionalRotation = Theta - 180.0f;
		}
		if (Input->MoveRight)
		{
			Entity->P += 15.0f*Right*Input->dt;
			GameState->CubeAdditionalRotation = Theta - 90.0f;
		}
		if (Input->MoveLeft)
		{
			Entity->P -= 15.0f*Right*Input->dt;
			GameState->CubeAdditionalRotation = Theta + 90.0f;
		}

		if (Input->MoveForward && Input->MoveRight)
		{
			GameState->CubeAdditionalRotation = Theta - 45.0f;
		}
		if (Input->MoveForward && Input->MoveLeft)
		{
			GameState->CubeAdditionalRotation = Theta + 45.0f;
		}
		if (Input->MoveBack && Input->MoveRight)
		{
			GameState->CubeAdditionalRotation = Theta - 135.0f;
		}
		if (Input->MoveBack && Input->MoveLeft)
		{
			GameState->CubeAdditionalRotation = Theta + 135.0f;
		}
	}
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	UseShader(GameState->DefaultShader);
	glBindVertexArray(GameState->CubeVAO);
	mat4 Model = Rotate(GameState->CubeAdditionalRotation, vec3(0.0f, 1.0f, 0.0f));
	mat4 View = RotationMatrixFromDirection(Camera->OffsetFromHero) * Translate(-Camera->OffsetFromHero);
	mat4 Projection = Perspective(45.0f, (r32)Width/Height, 0.1f, 100.0f);
	SetMat4(GameState->DefaultShader, "Model", Model);
	SetMat4(GameState->DefaultShader, "View", View);
	SetMat4(GameState->DefaultShader, "Projection", Projection);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	RenderChunks(&GameState->World, GameState->DefaultShader);
	
	EndSimulation(GameState, SimRegion, &GameState->WorldAllocator);
	EndTemporaryMemory(RenderMemory);
}