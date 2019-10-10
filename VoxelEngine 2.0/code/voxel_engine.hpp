#include "voxel_engine.h"

internal void
BeginSimRegion(world *World, vec3 Origin, vec3 Bounds, 
			   stack_allocator *WorldAllocator, stack_allocator *TempAllocator)
{
	i32 MinChunkX = floorf((Origin.x() - Bounds.x()) / World->ChunkDimInMeters);
	i32 MinChunkY = 0;
	i32 MinChunkZ = floorf((Origin.z() - Bounds.z()) / World->ChunkDimInMeters);

	i32 MaxChunkX = floorf((Origin.x() + Bounds.x()) / World->ChunkDimInMeters);
	i32 MaxChunkY = 1;
	i32 MaxChunkZ = floorf((Origin.z() + Bounds.z()) / World->ChunkDimInMeters);

	for(i32 ChunkZ = MinChunkZ;
		ChunkZ <= MaxChunkZ;
		ChunkZ++)
	{
		for(i32 ChunkY = MinChunkY;
		ChunkY <= MaxChunkY;
		ChunkY++)
		{
			for(i32 ChunkX = MinChunkX;
				ChunkX <= MaxChunkX;
				ChunkX++)
				{
					chunk *Chunk = GetChunk(World, ChunkX, ChunkY, ChunkZ, WorldAllocator);
					if(Chunk)
					{
						if(!Chunk->IsReady)
						{
							Chunk->IsReady = true;

							if(!World->FirstFreeChunkBlocksInfo)
							{
								World->FirstFreeChunkBlocksInfo = PushStruct(WorldAllocator, chunk_blocks_info);
							}
							Chunk->BlocksInfo = World->FirstFreeChunkBlocksInfo;
							World->FirstFreeChunkBlocksInfo = World->FirstFreeChunkBlocksInfo->Next;

							for(u32 BlockIndex = 0;
								BlockIndex < CHUNK_DIM*CHUNK_DIM*CHUNK_DIM;
								BlockIndex++)
							{
								Chunk->BlocksInfo->Blocks[BlockIndex].Active = true;
							}

							block *Blocks = Chunk->BlocksInfo->Blocks;

							InitializeDynamicArray(&Chunk->VerticesP);
							InitializeDynamicArray(&Chunk->VerticesNormals);
							
							for(u32 BlockZ = 0;
								BlockZ < CHUNK_DIM;
								BlockZ++)
							{
								for(u32 BlockY = 0;
									BlockY < CHUNK_DIM;
									BlockY++)
								{
									for(u32 BlockX = 0;
										BlockX < CHUNK_DIM;
										BlockX++)
									{
										if(IsBlockActive(Blocks, BlockX, BlockY, BlockZ))
										{
											r32 BlockDimInMeters = World->BlockDimInMeters;
											r32 X = BlockX*BlockDimInMeters;
											r32 Y = BlockY*BlockDimInMeters;
											r32 Z = BlockZ*BlockDimInMeters;

											vec3 A, B, C, D;

											if ((BlockX == 0) || !IsBlockActive(Blocks, BlockX - 1, BlockY, BlockZ))
											{
												A = vec3(X, Y, Z);
												B = vec3(X, Y, Z + BlockDimInMeters);
												C = vec3(X, Y + BlockDimInMeters, Z);
												D = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
												vec3 N = vec3(-1.0f, 0.0f, 0.0f);
												AddQuad(&Chunk->VerticesP, A, B, C, D);
												AddQuad(&Chunk->VerticesNormals, N, N, N, N);
											}

											if ((BlockX == CHUNK_DIM - 1) || !IsBlockActive(Blocks, BlockX + 1, BlockY, BlockZ))
											{
												A = vec3(X + BlockDimInMeters, Y, Z);
												B = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
												C = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
												D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
												vec3 N = vec3(1.0f, 0.0f, 0.0f);
												AddQuad(&Chunk->VerticesP, A, B, C, D);
												AddQuad(&Chunk->VerticesNormals, N, N, N, N);
											}

											if ((BlockZ == 0) || !IsBlockActive(Blocks, BlockX, BlockY, BlockZ - 1))
											{
												A = vec3(X, Y, Z);
												B = vec3(X, Y + BlockDimInMeters, Z);
												C = vec3(X + BlockDimInMeters, Y, Z);
												D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
												vec3 N = vec3(0.0f, 0.0f, -1.0f);
												AddQuad(&Chunk->VerticesP, A, B, C, D);
												AddQuad(&Chunk->VerticesNormals, N, N, N, N);
											}

											if ((BlockZ == CHUNK_DIM - 1) || !IsBlockActive(Blocks, BlockX, BlockY, BlockZ + 1))
											{
												A = vec3(X, Y, Z + BlockDimInMeters);
												B = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
												C = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
												D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
												vec3 N = vec3(0.0f, 0.0f, 1.0f);
												AddQuad(&Chunk->VerticesP, A, B, C, D);
												AddQuad(&Chunk->VerticesNormals, N, N, N, N);
											}

											if ((BlockY == 0) || (!IsBlockActive(Blocks, BlockX, BlockY - 1, BlockZ)))
											{
												A = vec3(X, Y, Z);
												B = vec3(X + BlockDimInMeters, Y, Z);
												C = vec3(X, Y, Z + BlockDimInMeters);
												D = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
												vec3 N = vec3(0.0f, -1.0f, 0.0f);
												AddQuad(&Chunk->VerticesP, A, B, C, D);
												AddQuad(&Chunk->VerticesNormals, N, N, N, N);
											}

											if ((BlockY == CHUNK_DIM - 1) || (!IsBlockActive(Blocks, BlockX, BlockY + 1, BlockZ)))
											{
												A = vec3(X, Y + BlockDimInMeters, Z);
												B = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
												C = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
												D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
												vec3 N = vec3(0.0f, 1.0f, 0.0f);
												AddQuad(&Chunk->VerticesP, A, B, C, D);
												AddQuad(&Chunk->VerticesNormals, N, N, N, N);
											}
										}
									}
								}
							}

							glGenVertexArrays(1, &Chunk->VAO);
							glGenBuffers(1, &Chunk->PVBO);
							glBindVertexArray(Chunk->VAO);
							glBindBuffer(GL_ARRAY_BUFFER, Chunk->PVBO);
							glBufferData(GL_ARRAY_BUFFER, Chunk->VerticesP.EntriesCount*sizeof(vec3), Chunk->VerticesP.Entries, GL_STATIC_DRAW);
							glEnableVertexAttribArray(0);
							glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
							glGenBuffers(1, &Chunk->NormalsVBO);
							glBindBuffer(GL_ARRAY_BUFFER, Chunk->NormalsVBO);
							glBufferData(GL_ARRAY_BUFFER, Chunk->VerticesNormals.EntriesCount*sizeof(vec3), Chunk->VerticesNormals.Entries, GL_STATIC_DRAW);
							glEnableVertexAttribArray(1);
							glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
							glBindVertexArray(0);
						}

						chunk *ChunkToRender = PushStruct(TempAllocator, chunk);
						*ChunkToRender = *Chunk;
						ChunkToRender->Translation = World->ChunkDimInMeters*vec3(ChunkToRender->X, ChunkToRender->Y, ChunkToRender->Z) -
													            Origin;
						ChunkToRender->Next = World->ChunksToRender;
						World->ChunksToRender = ChunkToRender;	
					}
				}	
		}
	}
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

		GameState->Camera.DistanceFromHero = 12.0f;
		GameState->Camera.Pitch = GameState->Camera.Head = 0;
		GameState->Camera.RotSensetivity = 0.05f;
		GameState->Camera.OffsetFromHero = {};

		GameState->World.ChunkDimInMeters = 8.0f;
		GameState->World.BlockDimInMeters = GameState->World.ChunkDimInMeters / CHUNK_DIM;
		GameState->World.ChunksToRender = 0;
		GameState->World.FirstFreeChunkBlocksInfo = 0;

		CompileShader(&GameState->DefaultShader, "data/shaders/DefaultVS.glsl", "data/shaders/DefaultFS.glsl");

		GameState->CubeP = vec3(0.0f, 0.0f, 0.0f);
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
	
	BeginSimRegion(&GameState->World, GameState->CubeP, vec3(10.0f, 10.0f, 10.0f), 
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

	//vec3 Forward = Normalize(vec3(-Camera->OffsetFromHero.x(), 0.0f, -Camera->OffsetFromHero.z()));
	vec3 Forward = Normalize(vec3(-Camera->OffsetFromHero.x(), -Camera->OffsetFromHero.y(), -Camera->OffsetFromHero.z()));
	vec3 Right = Normalize(Cross(Forward, vec3(0.0f, 1.0f, 0.0f)));
	r32 Theta = -RAD2DEG(atan2f(Forward.z(), Forward.x())) + 90.0f;
	if (Input->MoveForward)
	{
		GameState->CubeP += 5.0f*Forward*Input->dt;
		GameState->CubeAdditionalRotation = Theta;
	}
	if (Input->MoveBack)
	{
		GameState->CubeP -= 5.0f*Forward*Input->dt;
		GameState->CubeAdditionalRotation = Theta - 180.0f;
	}
	if (Input->MoveRight)
	{
		GameState->CubeP += 5.0f*Right*Input->dt;
		GameState->CubeAdditionalRotation = Theta - 90.0f;
	}
	if (Input->MoveLeft)
	{
		GameState->CubeP -= 5.0f*Right*Input->dt;
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

	EndTemporaryMemory(RenderMemory);
}