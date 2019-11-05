#include "voxel_engine_world.h"

inline void
InitializeWorld(world *World)
{
	World->ChunkDimInMeters = CHUNK_DIM / 2.0f;
	World->BlockDimInMeters = World->ChunkDimInMeters / CHUNK_DIM;
	World->RecentlyUsedCount = 0;
	World->RecentlyUsedChunks = 0;
	World->ChunksToRender = 0;
	World->FirstFreeChunkBlocksInfo = 0;
	World->FirstFreeWorldEntityBlock = 0;
}

inline world_position
InvalidPosition(void)
{
	world_position Result = {};
	Result.ChunkX = INVALID_POSITION;
	return(Result);
}

inline bool32
IsValid(world_position P)
{
	bool32 Result = P.ChunkX != INVALID_POSITION;
	return(Result);
}

internal void
RecanonicalizeCoords(world *World, world_position *Pos)
{
	i32 ChunkXOffset = (i32)FloorReal32ToInt32(Pos->Offset.x() / World->ChunkDimInMeters);
	Pos->ChunkX += ChunkXOffset;

	i32 ChunkYOffset = (i32)FloorReal32ToInt32(Pos->Offset.y() / World->ChunkDimInMeters);
	Pos->ChunkY += ChunkYOffset;

	i32 ChunkZOffset = (i32)FloorReal32ToInt32(Pos->Offset.z() / World->ChunkDimInMeters);
	Pos->ChunkZ += ChunkZOffset;

	vec3 ChunkOffset = vec3(ChunkXOffset, ChunkYOffset, ChunkZOffset);
	Pos->Offset = Pos->Offset - ChunkOffset*World->ChunkDimInMeters;
}

internal world_position
MapIntoChunkSpace(world *World, world_position *Pos, vec3 Offset)
{
	world_position Result = *Pos;

	Result.Offset += Offset;
	RecanonicalizeCoords(World, &Result);

	return(Result);
}

internal chunk *
GetChunk(world *World, i32 ChunkX, i32 ChunkY, i32 ChunkZ, stack_allocator *Allocator = 0)
{
	u32 HashValue = 19*ChunkX + 3*ChunkY + 7*ChunkZ;
	u32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
	Assert(HashSlot < ArrayCount(World->ChunkHash));

	chunk **ChunkPtr = World->ChunkHash + HashSlot;
	chunk *Chunk = *ChunkPtr;
	while(Chunk)
	{
		if((Chunk->X == ChunkX) &&
		   (Chunk->Y == ChunkY) &&
		   (Chunk->Z == ChunkZ))
		{
			break;
		}

		ChunkPtr = &Chunk->Next;
		Chunk = *ChunkPtr;
	}

	if(!Chunk && Allocator)
	{
		Chunk = PushStruct(Allocator, chunk);
		Chunk->X = ChunkX;
		Chunk->Y = ChunkY;
		Chunk->Z = ChunkZ;

		Chunk->IsSetup = false;
		Chunk->IsLoaded = false;
		Chunk->BlocksInfo = 0;

		Chunk->FirstEntityBlock.StoredEntityCount = 0;

		Chunk->Next = 0;

		*ChunkPtr = Chunk;
	}

	return(Chunk);
}

internal bool32
IsRecentlyUsed(world *World, chunk *ChunkToCheck)
{
	bool32 Result = false;
	for(chunk *Chunk = World->RecentlyUsedChunks;
		Chunk;
		Chunk = Chunk->NextRecentlyUsed)
	{
		if((Chunk->X == ChunkToCheck->X) &&
		   (Chunk->Y == ChunkToCheck->Y) &&
		   (Chunk->Z == ChunkToCheck->Z))
		{
			Result = true;
			break;
		}
	}

	return(Result);
}

// TODO(georgy): Should I use EBO here??
internal void
SetupChunk(world *World, chunk *Chunk, stack_allocator *WorldAllocator, bool32 DEBUGEmptyChunk)
{
	Chunk->IsSetup = true;
	Chunk->IsNotEmpty = false;

	if(!World->FirstFreeChunkBlocksInfo)
	{
		World->FirstFreeChunkBlocksInfo = PushStruct(WorldAllocator, chunk_blocks_info);
	}
	Chunk->BlocksInfo = World->FirstFreeChunkBlocksInfo;
	World->FirstFreeChunkBlocksInfo = World->FirstFreeChunkBlocksInfo->Next;

	ZeroSize(Chunk->BlocksInfo, sizeof(chunk_blocks_info));

	block *Blocks = Chunk->BlocksInfo->Blocks;
	vec3 *Colors = Chunk->BlocksInfo->Colors;

	for(u32 BlockZ = 0;
		BlockZ < CHUNK_DIM;
		BlockZ++)
	{
		for(u32 BlockX = 0;
			BlockX < CHUNK_DIM;
			BlockX++)
		{
			if(Chunk->Y < 0)
			{
				for(u32 BlockY = 0;
					BlockY < CHUNK_DIM;
					BlockY++)
				{
					Chunk->IsNotEmpty = true;
					Blocks[BlockZ*CHUNK_DIM*CHUNK_DIM + BlockY*CHUNK_DIM + BlockX].Active = true;
					Colors[BlockZ*CHUNK_DIM*CHUNK_DIM + BlockY*CHUNK_DIM + BlockX] = vec3(0.53f, 0.53f, 0.53f);
				}
			}
			else
			{
				r32 X = (Chunk->X * CHUNK_DIM) + (r32)BlockX + 0.5f;
				r32 Z = (Chunk->Z * CHUNK_DIM) + (r32)BlockZ + 0.5f;
				r32 NoiseValue = Clamp(PerlinNoise2D(0.0125f*vec2(X, Z)), 0.0f, 1.0f);
				NoiseValue += 0.15f*Clamp(PerlinNoise2D(0.05f*vec2(X, Z)), 0.0f, 1.0f);
				NoiseValue /= 1.1f;

				NoiseValue = NoiseValue*NoiseValue*NoiseValue*NoiseValue;

				r32 BiomeNoise = Clamp(PerlinNoise2D(0.0125f*vec2(X, Z)), 0.0f, 1.0f);
				vec3 BiomeColor = vec3(0.0f, 0.0f, 0.0f);
				if(NoiseValue < 0.01f)
				{
					BiomeColor = vec3(0.0f, 0.18f, 1.0f);
				}
				else if(NoiseValue < 0.013)
				{
					BiomeColor = vec3(0.97f, 0.81f, 0.6f);
				}
				else
				{
					BiomeColor = vec3(0.53f, 0.53f, 0.53f);
				}

				u32 Height = (u32)roundf(CHUNK_DIM * MAX_CHUNKS_Y * NoiseValue);
				Height++;
				u32 HeightForThisChunk = (u32)Clamp((r32)Height - (r32)Chunk->Y*CHUNK_DIM, 0.0f, CHUNK_DIM);
				for(u32 BlockY = 0;
					BlockY < HeightForThisChunk;
					BlockY++)
				{
					Chunk->IsNotEmpty = true;
					Blocks[BlockZ*CHUNK_DIM*CHUNK_DIM + BlockY*CHUNK_DIM + BlockX].Active = true;
					Colors[BlockZ*CHUNK_DIM*CHUNK_DIM + BlockY*CHUNK_DIM + BlockX] = BiomeColor;
				}
			}
		}
	}

	InitializeDynamicArray(&Chunk->VerticesP);
	InitializeDynamicArray(&Chunk->VerticesNormals);
	InitializeDynamicArray(&Chunk->VerticesColors);
	
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
					vec3 Color = Colors[BlockZ*CHUNK_DIM*CHUNK_DIM + BlockY*CHUNK_DIM + BlockX];

					if ((BlockX == 0) || !IsBlockActive(Blocks, BlockX - 1, BlockY, BlockZ))
					{
						A = vec3(X, Y, Z);
						B = vec3(X, Y, Z + BlockDimInMeters);
						C = vec3(X, Y + BlockDimInMeters, Z);
						D = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
						vec3 N = vec3(-1.0f, 0.0f, 0.0f);
						AddQuad(&Chunk->VerticesP, A, B, C, D);
						AddQuad(&Chunk->VerticesNormals, N, N, N, N);
						AddQuad(&Chunk->VerticesColors, Color, Color, Color, Color);
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
						AddQuad(&Chunk->VerticesColors, Color, Color, Color, Color);
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
						AddQuad(&Chunk->VerticesColors, Color, Color, Color, Color);
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
						AddQuad(&Chunk->VerticesColors, Color, Color, Color, Color);
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
						AddQuad(&Chunk->VerticesColors, Color, Color, Color, Color);
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
						AddQuad(&Chunk->VerticesColors, Color, Color, Color, Color);
					}
				}
			}
		}
	}
}

internal void
LoadChunk(chunk *Chunk)
{
	Chunk->IsLoaded = true;

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
	glGenBuffers(1, &Chunk->ColorsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, Chunk->ColorsVBO);
	glBufferData(GL_ARRAY_BUFFER, Chunk->VerticesColors.EntriesCount*sizeof(vec3), Chunk->VerticesColors.Entries, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
	glBindVertexArray(0);
}

internal void
UnloadChunk(world *World, chunk *Chunk)
{
	World->RecentlyUsedCount--;

	if(Chunk->IsSetup)
	{
		Chunk->BlocksInfo->Next = World->FirstFreeChunkBlocksInfo;
		World->FirstFreeChunkBlocksInfo = Chunk->BlocksInfo->Next;

		FreeDynamicArray(&Chunk->VerticesP);
		FreeDynamicArray(&Chunk->VerticesNormals);
		FreeDynamicArray(&Chunk->VerticesColors);

		Chunk->IsSetup = false;
	}

	if(Chunk->IsLoaded)
	{
		glDeleteBuffers(1, &Chunk->PVBO);
		glDeleteBuffers(1, &Chunk->NormalsVBO);
		glDeleteBuffers(1, &Chunk->ColorsVBO);
		glDeleteVertexArrays(1, &Chunk->VAO);

		Chunk->IsLoaded = false;
	}
}

internal void
UnloadChunks(world *World, world_position *MinChunkP, world_position *MaxChunkP)
{
	chunk **ChunkPtr = &World->RecentlyUsedChunks;
	chunk *Chunk = *ChunkPtr;
	while(Chunk)
	{
		if(Chunk->X < (MinChunkP->ChunkX - 2) ||
		   Chunk->Y < (MinChunkP->ChunkY - 2) ||
		   Chunk->Z < (MinChunkP->ChunkZ - 2) ||
		   Chunk->X > (MaxChunkP->ChunkX + 2) ||
		   Chunk->Y > (MaxChunkP->ChunkY + 2) ||
		   Chunk->Z > (MaxChunkP->ChunkZ + 2))
		{
			UnloadChunk(World, Chunk);

			*ChunkPtr = Chunk->NextRecentlyUsed;
		}
		else
		{
			ChunkPtr = &Chunk->NextRecentlyUsed;
		}

		Chunk = *ChunkPtr;
	}
}

internal void
RenderChunks(world *World, shader Shader)
{
	UseShader(Shader);
	for(chunk *Chunk = World->ChunksToRender;
		Chunk;
		Chunk = Chunk->Next)
	{
		mat4 Model = Translate(Chunk->Translation);
		SetMat4(Shader, "Model", Model);
		DrawFromVAO(Chunk->VAO, Chunk->VerticesP.EntriesCount);
	}
}

inline vec3
Substract(world *World, world_position *A, world_position *B)
{
	vec3 ChunkDelta = vec3((r32)A->ChunkX - B->ChunkX, (r32)A->ChunkY - B->ChunkY, (r32)A->ChunkZ - B->ChunkZ);
	vec3 Result = World->ChunkDimInMeters*ChunkDelta + (A->Offset - B->Offset);

	return(Result);
}

inline bool32
AreInTheSameChunk(world *World, world_position *A, world_position *B)
{
	r32 Epsilon = 0.001f;
	Assert(A->Offset.x() < World->ChunkDimInMeters + Epsilon);
	Assert(A->Offset.y() < World->ChunkDimInMeters + Epsilon);
	Assert(A->Offset.z() < World->ChunkDimInMeters + Epsilon);
	Assert(B->Offset.x() < World->ChunkDimInMeters + Epsilon);
	Assert(B->Offset.y() < World->ChunkDimInMeters + Epsilon);
	Assert(B->Offset.z() < World->ChunkDimInMeters + Epsilon);

	bool32 Result = ((A->ChunkX == B->ChunkX) &&
					 (A->ChunkY == B->ChunkY) &&
					 (A->ChunkZ == B->ChunkZ));

	return(Result);
}

internal void
ChangeEntityLocation(world *World, stack_allocator *WorldAllocator, u32 EntityIndex, 
					 stored_entity *StoredEntity, world_position NewPInit)
{
	world_position *OldP = 0;
	world_position *NewP = 0;

	if(IsValid(StoredEntity->P))
	{
		OldP = &StoredEntity->P;
	}

	if(IsValid(NewPInit))
	{
		NewP = &NewPInit;
	}

	if(OldP && NewP && AreInTheSameChunk(World, OldP, NewP))
	{

	}
	else
	{
		if(OldP)
		{
			chunk *Chunk = GetChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ, WorldAllocator);
			Assert(Chunk);
			world_entity_block *FirstBlock = &Chunk->FirstEntityBlock;
			bool32 NotFound = true;
			for(world_entity_block *Block = FirstBlock;
				Block && NotFound;
				Block = Block->Next)
			{
				for(u32 Index = 0;
					(Index < Block->StoredEntityCount) && NotFound;
					Index++)
				{
					if(Block->StoredEntityIndex[Index] == EntityIndex)
					{
						Assert(FirstBlock->StoredEntityCount > 0);
						Block->StoredEntityIndex[Index] = FirstBlock->StoredEntityIndex[--FirstBlock->StoredEntityCount];

						if(FirstBlock->StoredEntityCount == 0)
						{
							if(FirstBlock->Next)
							{
								world_entity_block *BlockToFree = FirstBlock->Next;
								*FirstBlock = *BlockToFree;

								BlockToFree->Next = World->FirstFreeWorldEntityBlock;
								World->FirstFreeWorldEntityBlock = BlockToFree;
							}
						}

						NotFound = false;	
					}
				}
			}
		}

		if(NewP)
		{
			chunk *Chunk = GetChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, WorldAllocator);
			Assert(Chunk);

			world_entity_block *Block = &Chunk->FirstEntityBlock;
			if(Block->StoredEntityCount == ArrayCount(Block->StoredEntityIndex))
			{
				if(!World->FirstFreeWorldEntityBlock)
				{
					World->FirstFreeWorldEntityBlock = PushStruct(WorldAllocator, world_entity_block);
				}

				world_entity_block *OldBlock = World->FirstFreeWorldEntityBlock;
				World->FirstFreeWorldEntityBlock = OldBlock->Next;
				*OldBlock = *Block;
				Block->StoredEntityCount = 0;
			}

			Assert(Block->StoredEntityCount < ArrayCount(Block->StoredEntityIndex));
			Block->StoredEntityIndex[Block->StoredEntityCount++] = EntityIndex;
		}
	}

	if(NewP)
	{
		StoredEntity->P = *NewP;
	}
	else
	{
		StoredEntity->P = InvalidPosition();
	}
}
