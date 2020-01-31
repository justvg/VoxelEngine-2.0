#include "voxel_engine_world.h"

inline void
InitializeWorld(world *World)
{
	World->ChunkDimInMeters = CHUNK_DIM / 2.0f;
	World->BlockDimInMeters = World->ChunkDimInMeters / CHUNK_DIM;
	World->RecentlyUsedCount = 0;
	World->ChunksToRender = 0;
	World->FirstFreeChunkBlocksInfo = 0;
	World->FirstFreeWorldEntityBlock = 0;
	World->Lock = 0;
	World->ChunksToSetupBlocksThisFrameCount = 0;
	World->ChunksToSetupFullyThisFrameCount = 0;
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
		*Chunk = {};
		Chunk->X = ChunkX;
		Chunk->Y = ChunkY;
		Chunk->Z = ChunkZ;

		*ChunkPtr = Chunk;
	}

	return(Chunk);
}

enum operation_between_chunks
{
	OperationBetweenChunks_IsBlockActive = 0x1,
	OperationBetweenChunks_SetActiveness = 0x2,
	OperationBetweenChunks_GetColor = 0x4,
};
internal bool32
OperationBetweenChunks(world *World, chunk *Chunk, i32 X, i32 Y, i32 Z, operation_between_chunks Op, 
					   bool32 Activeness = false, vec4 *Color = 0)
{
	bool32 Result = false;

	if((X >=0) && (X < CHUNK_DIM) &&
	   (Y >=0) && (Y < CHUNK_DIM) &&
	   (Z >=0) && (Z < CHUNK_DIM))
	{
		
	}
	else
	{
		i32 NewChunkX = Chunk->X;
		i32 NewChunkY = Chunk->Y;
		i32 NewChunkZ = Chunk->Z;
		if(X < 0)
		{
			X = CHUNK_DIM + X;
			NewChunkX--;
		}
		else if(X >= CHUNK_DIM)
		{
			X = X - CHUNK_DIM;
			NewChunkX++;
		}

		if(Y < 0)
		{
			Y = CHUNK_DIM + Y;
			NewChunkY--;
		}
		else if(Y >= CHUNK_DIM)
		{
			Y = Y - CHUNK_DIM;
			NewChunkY++;
		}

		if(Z < 0)
		{
			Z = CHUNK_DIM + Z;
			NewChunkZ--;
		}
		else if(Z >= CHUNK_DIM)
		{
			Z = Z - CHUNK_DIM;
			NewChunkZ++;
		}
		
		Chunk = GetChunk(World, NewChunkX, NewChunkY, NewChunkZ);
		Assert(Chunk);
	}

	if(Chunk->Y > MAX_CHUNKS_Y)
	{
		Result = false;
	}
	else
	{
		Assert(Chunk->IsSetupBlocks);
		Result = IsBlockActive(Chunk->BlocksInfo->Blocks, X, Y, Z) &&
				 (GetBlockType(Chunk->BlocksInfo->Blocks, X, Y, Z) != BlockType_Water);
		// Result = IsBlockActive(Chunk->BlocksInfo->Blocks, X, Y, Z);
		if(Op & OperationBetweenChunks_SetActiveness)
		{
			Chunk->BlocksInfo->Blocks[Z*CHUNK_DIM*CHUNK_DIM + Y*CHUNK_DIM + X].Active = Activeness;
			Chunk->IsModified = true;
		}
		if(Op & OperationBetweenChunks_GetColor)
		{
			Assert(Color);
			*Color = Chunk->BlocksInfo->Colors[Z*CHUNK_DIM*CHUNK_DIM + Y*CHUNK_DIM + X];
		}
	}

	return(Result);
}

inline bool32
IsBlockActiveBetweenChunks(world *World, chunk *Chunk, i32 X, i32 Y, i32 Z)
{
	bool32 Result = OperationBetweenChunks(World, Chunk, X, Y, Z, OperationBetweenChunks_IsBlockActive);
	return(Result);
}

inline void
SetBlockActiveBetweenChunks(world *World, chunk *Chunk, i32 X, i32 Y, i32 Z, bool32 Activeness)
{
	OperationBetweenChunks(World, Chunk, X, Y, Z, OperationBetweenChunks_SetActiveness, Activeness);
}

inline vec4
GetBlockColorBetweenChunks(world *World, chunk *Chunk, i32 X, i32 Y, i32 Z)
{
	vec4 Result;
	OperationBetweenChunks(World, Chunk, X, Y, Z, OperationBetweenChunks_GetColor, false, &Result);
	return(Result);
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

	vec3 ChunkOffset = vec3i(ChunkXOffset, ChunkYOffset, ChunkZOffset);
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

enum world_biome_type
{
	WorldBiome_Water,
	WorldBiome_Beach,
	WorldBiome_Scorched,
	WorldBiome_Tundra,
	WorldBiome_Snow,
	WorldBiome_Grassland,
};

inline r32
VertexAO(bool32 Side1, bool32 Side2, bool32 Corner)
{
	r32 AOValues[] = { 1.0f, 0.8f, 0.6f, 0.15f};
	r32 Result = AOValues[Side1 + Side2 + Corner];

	return(Result);
}

internal void
GenerateChunkVertices(world *World, chunk *Chunk)
{
	block *Blocks = Chunk->BlocksInfo->Blocks;
	vec4 *Colors = Chunk->BlocksInfo->Colors;

	for(i32 BlockZ = 0;
		BlockZ < CHUNK_DIM;
		BlockZ++)
	{
		for(i32 BlockY = 0;
			BlockY < CHUNK_DIM;
			BlockY++)
		{
			for(i32 BlockX = 0;
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
					vec4 Color = Colors[BlockZ*CHUNK_DIM*CHUNK_DIM + BlockY*CHUNK_DIM + BlockX];

					block_type BlockType = (block_type)GetBlockType(Blocks, BlockX, BlockY, BlockZ);
					bool32 IsWater = (BlockType ==  BlockType_Water);

					if ((BlockX == 0) || 
						!IsBlockActive(Blocks, BlockX - 1, BlockY, BlockZ) ||
						((GetBlockType(Blocks, BlockX - 1, BlockY, BlockZ) == BlockType_Water) && !IsWater))
					{
						if(!IsWater)
						{
							A = vec3(X, Y, Z);
							bool32 Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY - 1, BlockZ);
							bool32 Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY, BlockZ - 1);
							bool32 Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY - 1, BlockZ - 1);
							A.SetW(VertexAO(Side1, Side2, Corner));

							B = vec3(X, Y, Z + BlockDimInMeters);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY - 1, BlockZ);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY, BlockZ + 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY - 1, BlockZ + 1);
							B.SetW(VertexAO(Side1, Side2, Corner));

							C = vec3(X, Y + BlockDimInMeters, Z);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY + 1, BlockZ);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY, BlockZ - 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY + 1, BlockZ - 1);
							C.SetW(VertexAO(Side1, Side2, Corner));

							D = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY + 1, BlockZ);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY, BlockZ + 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY + 1, BlockZ + 1);
							D.SetW(VertexAO(Side1, Side2, Corner));

							if((C.w() + B.w()) < (A.w() + D.w()))
							{
								AddTriangle(&Chunk->VerticesP, A, B, D);
								AddTriangle(&Chunk->VerticesP, A, D, C);
							}
							else
							{
								AddTriangle(&Chunk->VerticesP, A, B, C);
								AddTriangle(&Chunk->VerticesP, C, B, D);
							}

							vec3 N = vec3(-1.0f, 0.0f, 0.0f);
							AddQuad(&Chunk->VerticesNormals, N, N, N, N);
							AddQuad(&Chunk->VerticesColors, Color, Color, Color, Color);
						}
					}

					if ((BlockX == CHUNK_DIM - 1) || 
						!IsBlockActive(Blocks, BlockX + 1, BlockY, BlockZ) || 
						((GetBlockType(Blocks, BlockX + 1, BlockY, BlockZ) == BlockType_Water) && !IsWater))
					{
						if(!IsWater)
						{
							A = vec3(X + BlockDimInMeters, Y, Z);
							bool32 Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY - 1, BlockZ);
							bool32 Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY, BlockZ - 1);
							bool32 Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY - 1, BlockZ - 1);
							A.SetW(VertexAO(Side1, Side2, Corner));

							B = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY + 1, BlockZ);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY, BlockZ - 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY + 1, BlockZ - 1);
							B.SetW(VertexAO(Side1, Side2, Corner));
							
							C = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY - 1, BlockZ);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY, BlockZ + 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY - 1, BlockZ + 1);
							C.SetW(VertexAO(Side1, Side2, Corner));

							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY + 1, BlockZ);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY, BlockZ + 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY + 1, BlockZ + 1);
							D.SetW(VertexAO(Side1, Side2, Corner));

							if((C.w() + B.w()) < (A.w() + D.w()))
							{
								AddTriangle(&Chunk->VerticesP, A, B, D);
								AddTriangle(&Chunk->VerticesP, A, D, C);
							}
							else
							{
								AddTriangle(&Chunk->VerticesP, A, B, C);
								AddTriangle(&Chunk->VerticesP, C, B, D);
							}

							vec3 N = vec3(1.0f, 0.0f, 0.0f);
							AddQuad(&Chunk->VerticesNormals, N, N, N, N);
							AddQuad(&Chunk->VerticesColors, Color, Color, Color, Color);
						}
					}

					if ((BlockZ == 0) || 
						!IsBlockActive(Blocks, BlockX, BlockY, BlockZ - 1) ||
						((GetBlockType(Blocks, BlockX, BlockY, BlockZ - 1) == BlockType_Water) && !IsWater))
					{
						if(!IsWater)
						{
							A = vec3(X, Y, Z);
							bool32 Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY - 1, BlockZ - 1);
							bool32 Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY, BlockZ - 1);
							bool32 Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY - 1, BlockZ - 1);
							r32 AAO = VertexAO(Side1, Side2, Corner);
							A.SetW(AAO);

							B = vec3(X, Y + BlockDimInMeters, Z);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY + 1, BlockZ - 1);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY, BlockZ - 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY + 1, BlockZ - 1);
							r32 BAO = VertexAO(Side1, Side2, Corner); 
							B.SetW(BAO);

							C = vec3(X + BlockDimInMeters, Y, Z);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY - 1, BlockZ - 1);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY, BlockZ - 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY - 1, BlockZ - 1);
							r32 CAO = VertexAO(Side1, Side2, Corner); 
							C.SetW(CAO);

							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY + 1, BlockZ - 1);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY, BlockZ - 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY + 1, BlockZ - 1);
							r32 DAO = VertexAO(Side1, Side2, Corner); 
							D.SetW(DAO);

							if((CAO + BAO) < (AAO + DAO))
							{
								AddTriangle(&Chunk->VerticesP, A, B, D);
								AddTriangle(&Chunk->VerticesP, A, D, C);
							}
							else
							{
								AddTriangle(&Chunk->VerticesP, A, B, C);
								AddTriangle(&Chunk->VerticesP, C, B, D);
							}

							vec3 N = vec3(0.0f, 0.0f, -1.0f);
							AddQuad(&Chunk->VerticesNormals, N, N, N, N);
							AddQuad(&Chunk->VerticesColors, Color, Color, Color, Color);
						}
					}

					if ((BlockZ == CHUNK_DIM - 1) || 
						!IsBlockActive(Blocks, BlockX, BlockY, BlockZ + 1) ||
						((GetBlockType(Blocks, BlockX, BlockY, BlockZ + 1) == BlockType_Water) && !IsWater))
					{
						if(!IsWater)
						{
							A = vec3(X, Y, Z + BlockDimInMeters);
							bool32 Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY - 1, BlockZ + 1);
							bool32 Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY, BlockZ + 1);
							bool32 Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY - 1, BlockZ + 1);
							r32 AAO = VertexAO(Side1, Side2, Corner);
							A.SetW(AAO);

							B = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY - 1, BlockZ + 1);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY, BlockZ + 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY - 1, BlockZ + 1);
							r32 BAO = VertexAO(Side1, Side2, Corner); 
							B.SetW(BAO);

							C = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY + 1, BlockZ + 1);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY, BlockZ + 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY + 1, BlockZ + 1);
							r32 CAO = VertexAO(Side1, Side2, Corner); 
							C.SetW(CAO);

							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY + 1, BlockZ + 1);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY, BlockZ + 1);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY + 1, BlockZ + 1);
							r32 DAO = VertexAO(Side1, Side2, Corner); 
							D.SetW(DAO);

							if((CAO + BAO) < (AAO + DAO))
							{
								AddTriangle(&Chunk->VerticesP, A, B, D);
								AddTriangle(&Chunk->VerticesP, A, D, C);
							}
							else
							{
								AddTriangle(&Chunk->VerticesP, A, B, C);
								AddTriangle(&Chunk->VerticesP, C, B, D);
							}

							vec3 N = vec3(0.0f, 0.0f, 1.0f);
							AddQuad(&Chunk->VerticesNormals, N, N, N, N);
							AddQuad(&Chunk->VerticesColors, Color, Color, Color, Color);
						}
					}

					if ((BlockY == 0) || (!IsBlockActive(Blocks, BlockX, BlockY - 1, BlockZ)))
					{
						if(BlockType != BlockType_Water)
						{
							A = vec3(X, Y, Z);
							bool32 Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY - 1, BlockZ - 1);
							bool32 Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY - 1, BlockZ);
							bool32 Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY - 1, BlockZ - 1);
							r32 AAO = VertexAO(Side1, Side2, Corner);
							A.SetW(AAO);

							B = vec3(X + BlockDimInMeters, Y, Z);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY - 1, BlockZ - 1);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY - 1, BlockZ);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY - 1, BlockZ - 1);
							r32 BAO = VertexAO(Side1, Side2, Corner); 
							B.SetW(BAO);

							C = vec3(X, Y, Z + BlockDimInMeters);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY - 1, BlockZ + 1);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY - 1, BlockZ);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY - 1, BlockZ + 1);
							r32 CAO = VertexAO(Side1, Side2, Corner); 
							C.SetW(CAO);

							D = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX , BlockY - 1, BlockZ + 1);
							Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY - 1, BlockZ);
							Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY - 1, BlockZ + 1);
							r32 DAO = VertexAO(Side1, Side2, Corner); 
							D.SetW(DAO);

							if((CAO + BAO) < (AAO + DAO))
							{
								AddTriangle(&Chunk->VerticesP, A, B, D);
								AddTriangle(&Chunk->VerticesP, A, D, C);
							}
							else
							{
								AddTriangle(&Chunk->VerticesP, A, B, C);
								AddTriangle(&Chunk->VerticesP, C, B, D);
							}

							vec3 N = vec3(0.0f, -1.0f, 0.0f);
							AddQuad(&Chunk->VerticesNormals, N, N, N, N);
							AddQuad(&Chunk->VerticesColors, Color, Color, Color, Color);
						}
					}

					if ((BlockY == CHUNK_DIM - 1) || (!IsBlockActive(Blocks, BlockX, BlockY + 1, BlockZ)))
					{
						A = vec3(X, Y + BlockDimInMeters, Z);
						bool32 Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX, BlockY + 1, BlockZ - 1);
						bool32 Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY + 1, BlockZ);
						bool32 Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY + 1, BlockZ - 1);
						r32 AAO = VertexAO(Side1, Side2, Corner);
						A.SetW(AAO);

						B = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
						Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX, BlockY + 1, BlockZ + 1);
						Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY + 1, BlockZ);
						Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX - 1, BlockY + 1, BlockZ + 1);
						r32 BAO = VertexAO(Side1, Side2, Corner); 
						B.SetW(BAO);

						C = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
						Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX, BlockY + 1, BlockZ - 1);
						Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY + 1, BlockZ);
						Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY + 1, BlockZ - 1);
						r32 CAO = VertexAO(Side1, Side2, Corner); 
						C.SetW(CAO);

						D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
						Side1 = IsBlockActiveBetweenChunks(World, Chunk, BlockX, BlockY + 1, BlockZ + 1);
						Side2 = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY + 1, BlockZ);
						Corner = IsBlockActiveBetweenChunks(World, Chunk, BlockX + 1, BlockY + 1, BlockZ + 1);
						r32 DAO = VertexAO(Side1, Side2, Corner); 
						D.SetW(DAO);

						dynamic_array_vec3 *Positions = IsWater ? &Chunk->WaterVerticesP : &Chunk->VerticesP;
						dynamic_array_vec4 *Colors = IsWater ? &Chunk->WaterVerticesColors : &Chunk->VerticesColors;
						if((CAO + BAO) < (AAO + DAO))
						{
							AddTriangle(Positions, A, B, D);
							AddTriangle(Positions, A, D, C);
						}
						else
						{
							AddTriangle(Positions, A, B, C);
							AddTriangle(Positions, C, B, D);
						}
							
						AddQuad(Colors, Color, Color, Color, Color);

						if(!IsWater)
						{
							vec3 N = vec3(0.0f, 1.0f, 0.0f);
							AddQuad(&Chunk->VerticesNormals, N, N, N, N);
						}
					}
				}
			}
		}
	}
}

struct setup_chunk_blocks_job
{
	world *World;
	chunk *Chunk;
	stack_allocator *WorldAllocator;
};
internal PLATFORM_JOB_SYSTEM_CALLBACK(SetupChunkBlocks)
{
	TIME_BLOCK;
	setup_chunk_blocks_job *Job = (setup_chunk_blocks_job *)Data;
	world *World = Job->World;
	chunk *Chunk = Job->Chunk;
	stack_allocator *WorldAllocator = Job->WorldAllocator;

	Chunk->IsNotEmpty = false;
	
	BeginWorldLock(World);
	if(!World->FirstFreeChunkBlocksInfo)
	{
		World->FirstFreeChunkBlocksInfo = PushStruct(WorldAllocator, chunk_blocks_info);
	}
	Chunk->BlocksInfo = World->FirstFreeChunkBlocksInfo;
	World->FirstFreeChunkBlocksInfo = World->FirstFreeChunkBlocksInfo->Next;
	EndWorldLock(World);

	ZeroSize(Chunk->BlocksInfo, sizeof(chunk_blocks_info));

	block *Blocks = Chunk->BlocksInfo->Blocks;
	vec4 *Colors = Chunk->BlocksInfo->Colors;

	for(u32 BlockZ = 0;
		BlockZ < CHUNK_DIM;
		BlockZ++)
	{
		for(u32 BlockX = 0;
			BlockX < CHUNK_DIM;
			BlockX++)
		{
			if(Chunk->Y == 0)
			{
				Chunk->IsNotEmpty = true;
				Blocks[BlockZ*CHUNK_DIM*CHUNK_DIM + (CHUNK_DIM - 1)*CHUNK_DIM + BlockX].Active = true;
				Colors[BlockZ*CHUNK_DIM*CHUNK_DIM + (CHUNK_DIM - 1)*CHUNK_DIM + BlockX] = vec4(0.53f, 0.53f, 0.53f, 1.0f);
			}
			else
			{
				r32 X = (Chunk->X * CHUNK_DIM) + (r32)BlockX + 0.5f;
				r32 Z = (Chunk->Z * CHUNK_DIM) + (r32)BlockZ + 0.5f;
				r32 NoiseValue = Clamp(PerlinNoise2D(0.01f*vec2(X, Z)), 0.0f, 1.0f);
				NoiseValue += 0.15f*Clamp(PerlinNoise2D(0.05f*vec2(X, Z)), 0.0f, 1.0f)*NoiseValue;
				NoiseValue = NoiseValue*NoiseValue*NoiseValue*NoiseValue*NoiseValue;

				r32 BiomeNoise = Clamp(PerlinNoise2D(0.007f*vec2(X, Z)), 0.0f, 1.0f);
				BiomeNoise += 0.5f*Clamp(PerlinNoise2D(0.03f*vec2(X, Z)), 0.0f, 1.0f);
				BiomeNoise += 0.25f*Clamp(PerlinNoise2D(0.06f*vec2(X, Z)), 0.0f, 1.0f);
				BiomeNoise /= 1.75f;

				world_biome_type BiomeType = WorldBiome_Grassland;
				if(NoiseValue < 0.018f)
				// if(NoiseValue < 0.008f)
				{
					Chunk->HasWater = true;
					BiomeType = WorldBiome_Water;
				}
				else if(NoiseValue < 0.011f)
				{
					BiomeType = WorldBiome_Beach;
				}
				else if(NoiseValue > 0.7f)
				{
					if(BiomeNoise < 0.3f)
					{
						BiomeType = WorldBiome_Scorched;
					}
					else if(BiomeNoise < 0.4f)
					{
						BiomeType = WorldBiome_Tundra;
					}
					else
					{
						BiomeType = WorldBiome_Snow;
					}
				}
				else 
				{
					BiomeType = WorldBiome_Grassland;
				}

				u32 Height = (u32)roundf(CHUNK_DIM * MAX_CHUNKS_Y * NoiseValue);
				Height++;
				u32 HeightForThisChunk = (u32)Clamp((r32)Height - (r32)(Chunk->Y-1)*CHUNK_DIM, 0.0f, CHUNK_DIM);
				for(u32 BlockY = 0;
					BlockY < HeightForThisChunk;
					BlockY++)
				{
					r32 Y = ((Chunk->Y - 1) * CHUNK_DIM) + (r32)BlockY + 0.5f;
					if((NoiseValue > 0.2f) && (NoiseValue < 0.55f))
					{
						r32 CaveNoise = Clamp(PerlinNoise3D(0.02f*vec3(X, 2.0f*(Y + 10.0f), Z)), 0.0f, 1.0f);
						CaveNoise = CaveNoise*CaveNoise*CaveNoise;
						Blocks[BlockZ*CHUNK_DIM*CHUNK_DIM + BlockY*CHUNK_DIM + BlockX].Active = (CaveNoise < 0.475f);
					}
					else
					{
						Blocks[BlockZ*CHUNK_DIM*CHUNK_DIM + BlockY*CHUNK_DIM + BlockX].Active = true;
					}

					if(IsBlockActive(Blocks, BlockX, BlockY, BlockZ))
					{
						Chunk->IsNotEmpty = true;
					}

					r32 ColorNoise = Clamp(PerlinNoise3D(0.02f*vec3(X, Y, Z)), 0.0f, 1.0f);
					ColorNoise *= ColorNoise;
					vec4 Color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
					switch(BiomeType)
					{
						case WorldBiome_Water:
						{
							// Color = Lerp(vec4(0.0f, 0.0f, 1.0f, 0.25f), vec4(0.0f, 0.0f, 1.0f, 0.25f), ColorNoise);
							Color = Lerp(vec4(0.0f, 0.25f, 0.8f, 0.1f), vec4(0.0f, 0.18f, 1.0f, 0.1f), ColorNoise);
							Blocks[BlockZ*CHUNK_DIM*CHUNK_DIM + BlockY*CHUNK_DIM + BlockX].Type = BlockType_Water;
						} break;

						case WorldBiome_Beach:
						{
							Color = Lerp(vec4(0.9f, 0.85f, 0.7f, 1.0f), vec4(1.0f, 0.819f, 0.6f, 1.0f), ColorNoise);
						} break;

						case WorldBiome_Scorched:
						{
							Color = Lerp(vec4(0.55f, 0.36f, 0.172f, 1.0f), vec4(0.63f, 0.4f, 0.172f, 1.0f), ColorNoise);
						} break;

						case WorldBiome_Tundra:
						{
							Color = Lerp(vec4(0.78f, 0.925f, 0.54f, 1.0f), vec4(0.815f, 0.925f, 0.59f, 1.0f), ColorNoise);
						} break;

						case WorldBiome_Snow:
						{
							Color = Lerp(vec4(0.75f, 0.75f, 0.85f, 1.0f), vec4(0.67f, 0.55f, 1.0f, 1.0f), ColorNoise);
							Blocks[BlockZ*CHUNK_DIM*CHUNK_DIM + BlockY*CHUNK_DIM + BlockX].Type = BlockType_Snow;
						} break;

						case WorldBiome_Grassland:
						{
							Color = Lerp(vec4(0.0f, 0.56f, 0.16f, 1.0f), vec4(0.65f, 0.9f, 0.0f, 1.0f), ColorNoise);
						} break;
					}
					Colors[BlockZ*CHUNK_DIM*CHUNK_DIM + BlockY*CHUNK_DIM + BlockX] = Color;
				}
			}
		}
	}

	CompletePreviousWritesBeforeFutureWrites;

	Chunk->IsSetupBlocks = true;
}

internal void
SetupChunksBlocks(world *World, stack_allocator *WorldAllocator, temp_state *TempState)
{
	for(u32 ChunkIndex = 0;
		ChunkIndex < World->ChunksToSetupBlocksThisFrameCount;
		ChunkIndex++)
	{
		setup_chunk_blocks_job *Job = PushStruct(&TempState->Allocator, setup_chunk_blocks_job);
		Job->World = World;
		Job->Chunk = World->ChunksToSetupBlocks[ChunkIndex];
		Job->WorldAllocator = WorldAllocator;

		PlatformAddEntry(TempState->JobSystemQueue, SetupChunkBlocks, Job);
	}
	
	PlatformCompleteAllWork(TempState->JobSystemQueue);

	World->ChunksToSetupBlocksThisFrameCount = 0;
}

internal bool32
CanSetupAO(world *World, chunk *Chunk, stack_allocator *WorldAllocator)
{
	bool32 Result;
	bool32 ChunkMinY = (Chunk->Y == MIN_CHUNKS_Y);
	bool32 ChunkMaxY = (Chunk->Y == MAX_CHUNKS_Y);

	Result = GetChunk(World, Chunk->X - 1, Chunk->Y - 1, Chunk->Z, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X - 1, Chunk->Y - 1, Chunk->Z - 1, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X - 1, Chunk->Y - 1, Chunk->Z + 1, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X - 1, Chunk->Y, Chunk->Z, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X - 1, Chunk->Y, Chunk->Z - 1, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X - 1, Chunk->Y, Chunk->Z + 1, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X - 1, Chunk->Y + 1, Chunk->Z, WorldAllocator)->IsSetupBlocks || ChunkMaxY;
	Result &= GetChunk(World, Chunk->X - 1, Chunk->Y + 1, Chunk->Z - 1, WorldAllocator)->IsSetupBlocks || ChunkMaxY;
	Result &= GetChunk(World, Chunk->X - 1, Chunk->Y + 1, Chunk->Z + 1, WorldAllocator)->IsSetupBlocks || ChunkMaxY;
	Result &= GetChunk(World, Chunk->X + 1, Chunk->Y - 1, Chunk->Z, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X + 1, Chunk->Y - 1, Chunk->Z - 1, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X + 1, Chunk->Y - 1, Chunk->Z + 1, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X + 1, Chunk->Y, Chunk->Z, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X + 1, Chunk->Y, Chunk->Z - 1, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X + 1, Chunk->Y, Chunk->Z + 1, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X + 1, Chunk->Y + 1, Chunk->Z, WorldAllocator)->IsSetupBlocks || ChunkMaxY;
	Result &= GetChunk(World, Chunk->X + 1, Chunk->Y + 1, Chunk->Z - 1, WorldAllocator)->IsSetupBlocks || ChunkMaxY;
	Result &= GetChunk(World, Chunk->X + 1, Chunk->Y + 1, Chunk->Z + 1, WorldAllocator)->IsSetupBlocks || ChunkMaxY;
	Result &= GetChunk(World, Chunk->X, Chunk->Y - 1, Chunk->Z, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X, Chunk->Y - 1, Chunk->Z - 1, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X, Chunk->Y - 1, Chunk->Z + 1, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X, Chunk->Y + 1, Chunk->Z, WorldAllocator)->IsSetupBlocks || ChunkMaxY;
	Result &= GetChunk(World, Chunk->X, Chunk->Y + 1, Chunk->Z - 1, WorldAllocator)->IsSetupBlocks || ChunkMaxY;
	Result &= GetChunk(World, Chunk->X, Chunk->Y + 1, Chunk->Z + 1, WorldAllocator)->IsSetupBlocks || ChunkMaxY;
	Result &= GetChunk(World, Chunk->X, Chunk->Y, Chunk->Z - 1, WorldAllocator)->IsSetupBlocks;
	Result &= GetChunk(World, Chunk->X, Chunk->Y, Chunk->Z + 1, WorldAllocator)->IsSetupBlocks;

	return(Result);
}

// TODO(georgy): Should I use EBO here??
struct setup_chunk_vertices_job
{
	world *World;
	chunk *Chunk;
};
internal PLATFORM_JOB_SYSTEM_CALLBACK(SetupChunkVertices)
{
	TIME_BLOCK;
	setup_chunk_vertices_job *Job = (setup_chunk_vertices_job *)Data;
	world *World = Job->World;
	chunk *Chunk = Job->Chunk;

	InitializeDynamicArray(&Chunk->VerticesP);
	InitializeDynamicArray(&Chunk->VerticesNormals);
	InitializeDynamicArray(&Chunk->VerticesColors);
	
	InitializeDynamicArray(&Chunk->WaterVerticesP);
	InitializeDynamicArray(&Chunk->WaterVerticesColors);

	block *Blocks = Chunk->BlocksInfo->Blocks;
	vec4 *Colors = Chunk->BlocksInfo->Colors;

	GenerateChunkVertices(World, Chunk);

	CompletePreviousWritesBeforeFutureWrites;

	Chunk->IsFullySetup = true;
}

internal void
SetupChunksVertices(world *World, temp_state *TempState)
{
	Assert(World->ChunksToSetupFullyThisFrameCount <= MAX_CHUNKS_TO_SETUP_FULLY);
	for(u32 ChunkIndex = 0;
		ChunkIndex < World->ChunksToSetupFullyThisFrameCount;
		ChunkIndex++)
	{
		setup_chunk_vertices_job *Job = PushStruct(&TempState->Allocator, setup_chunk_vertices_job);
		Job->World = World;
		Job->Chunk = World->ChunksToSetupFully[ChunkIndex];

		PlatformAddEntry(TempState->JobSystemQueue, SetupChunkVertices, Job);
	}
	
	PlatformCompleteAllWork(TempState->JobSystemQueue);

	World->ChunksToSetupFullyThisFrameCount = 0;
}

internal void
LoadChunk(chunk *Chunk)
{
	glGenVertexArrays(1, &Chunk->VAO);
	glGenBuffers(1, &Chunk->PVBO);
	glBindVertexArray(Chunk->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, Chunk->PVBO);
	glBufferData(GL_ARRAY_BUFFER, Chunk->VerticesP.EntriesCount*sizeof(vec3), Chunk->VerticesP.Entries, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
	glGenBuffers(1, &Chunk->NormalsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, Chunk->NormalsVBO);
	glBufferData(GL_ARRAY_BUFFER, Chunk->VerticesNormals.EntriesCount*sizeof(vec3), Chunk->VerticesNormals.Entries, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
	glGenBuffers(1, &Chunk->ColorsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, Chunk->ColorsVBO);
	glBufferData(GL_ARRAY_BUFFER, Chunk->VerticesColors.EntriesCount*sizeof(vec4), Chunk->VerticesColors.Entries, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void *)0);
	glBindVertexArray(0);

	glGenVertexArrays(1, &Chunk->WaterVAO);
	glGenBuffers(1, &Chunk->WaterPVBO);
	glBindVertexArray(Chunk->WaterVAO);
	glBindBuffer(GL_ARRAY_BUFFER, Chunk->WaterPVBO);
	glBufferData(GL_ARRAY_BUFFER, Chunk->WaterVerticesP.EntriesCount*sizeof(vec3), Chunk->WaterVerticesP.Entries, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
	glGenBuffers(1, &Chunk->WaterColorsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, Chunk->WaterColorsVBO);
	glBufferData(GL_ARRAY_BUFFER, Chunk->WaterVerticesColors.EntriesCount*sizeof(vec4), Chunk->WaterVerticesColors.Entries, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void *)0);
	glBindVertexArray(0);

	CompletePreviousWritesBeforeFutureWrites;

	Chunk->IsLoaded = true;
}

internal void
LoadChunks(world *World)
{
	TIME_BLOCK;
	for(u32 ChunkIndex = 0;
		ChunkIndex < World->ChunksToLoadThisFrameCount;
		ChunkIndex++)
	{
		LoadChunk(World->ChunksToLoad[ChunkIndex]);
	}
	
	World->ChunksToLoadThisFrameCount = 0;
}

internal void
UnloadChunk(world *World, chunk *Chunk)
{
	World->RecentlyUsedCount--;

	if(Chunk->IsSetupBlocks)
	{
		Chunk->BlocksInfo->Next = World->FirstFreeChunkBlocksInfo;
		World->FirstFreeChunkBlocksInfo = Chunk->BlocksInfo;

		Chunk->IsSetupBlocks = false;
	}

	if(Chunk->IsFullySetup)
	{
		FreeDynamicArray(&Chunk->VerticesP);
		FreeDynamicArray(&Chunk->VerticesNormals);
		FreeDynamicArray(&Chunk->VerticesColors);

		FreeDynamicArray(&Chunk->WaterVerticesP);
		FreeDynamicArray(&Chunk->WaterVerticesColors);

		Chunk->IsFullySetup = false;
	}

	if(Chunk->IsLoaded)
	{
		glDeleteBuffers(1, &Chunk->PVBO);
		glDeleteBuffers(1, &Chunk->NormalsVBO);
		glDeleteBuffers(1, &Chunk->ColorsVBO);
		glDeleteVertexArrays(1, &Chunk->VAO);

		glDeleteBuffers(1, &Chunk->WaterPVBO);
		glDeleteBuffers(1, &Chunk->WaterColorsVBO);
		glDeleteVertexArrays(1, &Chunk->WaterVAO);

		Chunk->IsLoaded = false;
	}

#if VOXEL_ENGINE_INTERNAL
	Assert(DEBUGGlobalPlaybackInfo.ChunksUnloadedDuringRecordPhaseCount < ArrayCount(DEBUGGlobalPlaybackInfo.ChunksUnloadedDuringRecordPhase));
	if(DEBUGGlobalPlaybackInfo.RecordPhase)
	{
		DEBUGGlobalPlaybackInfo.ChunksUnloadedDuringRecordPhase[DEBUGGlobalPlaybackInfo.ChunksUnloadedDuringRecordPhaseCount++] = 
			Chunk;
	}
#endif


	// TODO(georgy): Should I free the chunk itself and than use its memory for new chunk in GetChunk? As I do e.g. with BlockInfo
}

internal void
UnloadChunks(world *World, world_position *MinChunkP, world_position *MaxChunkP)
{
	u32 MaxChunksToUnloadPerFrame = 6;
	u32 ChunksUnloaded = 0;
	for(u32 ChunkHashBucket = 0;
	   (ChunkHashBucket < ArrayCount(World->ChunkHash)) && (ChunksUnloaded < MaxChunksToUnloadPerFrame);
		ChunkHashBucket++)
	{
		chunk *Chunk = World->ChunkHash[ChunkHashBucket];
		while(Chunk && (ChunksUnloaded < MaxChunksToUnloadPerFrame))
		{
			if(Chunk->IsRecentlyUsed)
			{
				if(Chunk->X < (MinChunkP->ChunkX - 4) ||
				   Chunk->Z < (MinChunkP->ChunkZ - 4) ||
				   Chunk->X > (MaxChunkP->ChunkX + 4) ||
				   Chunk->Z > (MaxChunkP->ChunkZ + 4))
				{
					Chunk->IsRecentlyUsed = false;
					ChunksUnloaded++;
 					UnloadChunk(World, Chunk);
				}
			}

			Chunk = Chunk->Next;
		}
	}
}

internal bool32
CheckChunkEmptiness(chunk *Chunk)
{
	bool32 IsNotEmpty = false;

	block *Blocks = Chunk->BlocksInfo->Blocks;
	for(i32 BlockZ = 0;
		BlockZ < CHUNK_DIM;
		BlockZ++)
	{
		for(i32 BlockY = 0;
			BlockY < CHUNK_DIM;
			BlockY++)
		{
			for(i32 BlockX = 0;
				BlockX < CHUNK_DIM;
				BlockX++)
			{
				if(IsBlockActive(Blocks, BlockX, BlockY, BlockZ))
				{
					IsNotEmpty = true;
					break;
				}
			}
		}
	}

	return(IsNotEmpty);
}

internal void
UpdateChunk(world *World, chunk *Chunk)
{
	Chunk->IsNotEmpty = CheckChunkEmptiness(Chunk);
	
	if(Chunk->IsNotEmpty)
	{
		Chunk->VerticesP.EntriesCount = 0;
		Chunk->VerticesNormals.EntriesCount = 0;
		Chunk->VerticesColors.EntriesCount = 0;
		GenerateChunkVertices(World, Chunk);

		glBindVertexArray(Chunk->VAO);
		glBindBuffer(GL_ARRAY_BUFFER, Chunk->PVBO);
		glBufferData(GL_ARRAY_BUFFER, Chunk->VerticesP.EntriesCount*sizeof(vec3), Chunk->VerticesP.Entries, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, Chunk->NormalsVBO);
		glBufferData(GL_ARRAY_BUFFER, Chunk->VerticesNormals.EntriesCount*sizeof(vec3), Chunk->VerticesNormals.Entries, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, Chunk->ColorsVBO);
		glBufferData(GL_ARRAY_BUFFER, Chunk->VerticesColors.EntriesCount*sizeof(vec3), Chunk->VerticesColors.Entries, GL_STATIC_DRAW);
		glBindVertexArray(0);
	}

	Chunk->IsModified = false;

#if VOXEL_ENGINE_INTERNAL
	if(DEBUGGlobalPlaybackInfo.RecordPhase)
	{
		if((Chunk->X >= DEBUGGlobalPlaybackInfo.MinChunkP.ChunkX) &&
		   (Chunk->Z >= DEBUGGlobalPlaybackInfo.MinChunkP.ChunkZ) &&
		   (Chunk->X <= DEBUGGlobalPlaybackInfo.MaxChunkP.ChunkX) &&
		   (Chunk->Z <= DEBUGGlobalPlaybackInfo.MaxChunkP.ChunkZ))
		{
			Assert(DEBUGGlobalPlaybackInfo.ChunksModifiedDuringRecordPhaseCount < 
					ArrayCount(DEBUGGlobalPlaybackInfo.ChunksModifiedDuringRecordPhase));
			DEBUGGlobalPlaybackInfo.ChunksModifiedDuringRecordPhase[DEBUGGlobalPlaybackInfo.ChunksModifiedDuringRecordPhaseCount++] = 
				Chunk;
		}
	}
#endif
}

internal chunk *
SortedMerge(chunk *A, chunk *B)
{
	chunk DummyNode;
	chunk *Tail = &DummyNode;
	Tail->Next = 0;

	while(1)
	{
		if(A == 0)
		{
			Tail->Next = B;
			break;
		}
		else if(B == 0)
		{
			Tail->Next = A;
			break;
		}
		
		if(A->LengthSqTranslation <= B->LengthSqTranslation)
		{
			chunk *Temp = A;
			A = A->Next;
			Tail->Next = Temp;
			Temp->Next = 0;
		}
		else
		{
			chunk *Temp = B;
			B = B->Next;
			Tail->Next = Temp;
			Temp->Next = 0;
		}

		Tail = Tail->Next;
	}

	return(DummyNode.Next);
}

internal void
SplitLists(chunk *Chunk, chunk **A, chunk **B)
{
	chunk *Slow = Chunk;
	chunk *Fast = Chunk->Next;

	while(Fast)
	{
		Fast = Fast->Next;
		if(Fast)
		{
			Fast = Fast->Next;
			Slow = Slow->Next;
		}
	}

	*A = Chunk;
	*B = Slow->Next;
	Slow->Next = 0;
}

internal void 
MergeSort(chunk **ChunkPtr)
{
	chunk *Chunk = *ChunkPtr;
	chunk *A, *B;

	if((Chunk == 0) || (Chunk->Next == 0))
	{
		return;
	}

	SplitLists(Chunk, &A, &B);

	MergeSort(&A);
	MergeSort(&B);

	*ChunkPtr = SortedMerge(A, B);
}

// NOTE(georgy): This function is optimized for Min to be vec3(0.0f, 0.0f, 0.0f) what is nice for chunks!
internal bool32
ChunkFrustumCulling(mat4 MVP, vec3 Min, vec3 Dim)
{
	vec4 Points[8];

	r32 X = Dim.x();
	r32 Y = Dim.y();
	r32 Z = Dim.z();
	Points[0] = MVP.FourthColumn;
	Points[1] = Y * MVP.SecondColumn + MVP.FourthColumn;
	Points[2] = Z * MVP.ThirdColumn + MVP.FourthColumn;
	Points[3] = Y * MVP.SecondColumn + Z * MVP.ThirdColumn + MVP.FourthColumn;
	Points[4] = X * MVP.FirstColumn + MVP.FourthColumn;
	Points[5] = X * MVP.FirstColumn + Y * MVP.SecondColumn + MVP.FourthColumn;
	Points[6] = X * MVP.FirstColumn + Z * MVP.ThirdColumn + MVP.FourthColumn;
	Points[7] = X * MVP.FirstColumn + Y * MVP.SecondColumn + Z * MVP.ThirdColumn + MVP.FourthColumn;

	vec4 W0P = vec4(Points[0].w(), Points[1].w(), Points[2].w(), Points[3].w());
	vec4 W1P = vec4(Points[4].w(), Points[5].w(), Points[6].w(), Points[7].w());
	vec4 W0N = -W0P;
	vec4 W1N = -W1P;

	vec4 X0 = vec4(Points[0].x(), Points[1].x(), Points[2].x(), Points[3].x());
	vec4 X1 = vec4(Points[4].x(), Points[5].x(), Points[6].x(), Points[7].x());

	vec4 Y0 = vec4(Points[0].y(), Points[1].y(), Points[2].y(), Points[3].y());
	vec4 Y1 = vec4(Points[4].y(), Points[5].y(), Points[6].y(), Points[7].y());

	vec4 Z0 = vec4(Points[0].z(), Points[1].z(), Points[2].z(), Points[3].z());
	vec4 Z1 = vec4(Points[4].z(), Points[5].z(), Points[6].z(), Points[7].z());

	if(All(X0 > W0P) && All(X1 > W1P))
	{
		return(false);
	}
	if(All(X0 < W0N) && All(X1 < W1N))
	{
		return(false);
	}

	if(All(Y0 > W0P) && All(Y1 > W1P))
	{
		return(false);
	}
	if(All(Y0 < W0N) && All(Y1 < W1N))
	{
		return(false);
	}

	if(All(Z0 > W0P) && All(Z1 > W1P))
	{
		return(false);
	}
	if(All(Z0 < W0N) && All(Z1 < W1N))
	{
		return(false);
	}

	return(true);
}

internal void
RenderChunks(world *World, shader Shader, shader WaterShader, mat4 VP, vec3 CameraP)
{
	DEBUG_VARIABLE(bool32, RenderChunksBB, Rendering);

	UseShader(Shader);
	
	MergeSort(&World->ChunksToRender);
	vec3 ChunkDim = vec3(World->ChunkDimInMeters, World->ChunkDimInMeters, World->ChunkDimInMeters);

	for(chunk *Chunk = World->ChunksToRender;
		Chunk;
		Chunk = Chunk->Next)
	{
		mat4 Model = Translate(Chunk->Translation);
		if(ChunkFrustumCulling(VP * Model, vec3(0.0f, 0.0f, 0.0f), ChunkDim))
		{
			SetMat4(Shader, "Model", Model);
			DrawFromVAO(Chunk->VAO, Chunk->VerticesP.EntriesCount);

			if(RenderChunksBB)
			{
				DEBUGRenderCube(Chunk->Translation + 0.5f*ChunkDim, ChunkDim, 0.0f);
				UseShader(Shader);
			}
		}
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	UseShader(WaterShader);

	for(chunk *Chunk = World->ChunksToRender;
		Chunk;
		Chunk = Chunk->Next)
	{
		if(Chunk->WaterVerticesP.EntriesCount)
		{
			mat4 Model = Translate(Chunk->Translation);
			if(ChunkFrustumCulling(VP * Model, vec3(0.0f, 0.0f, 0.0f), ChunkDim))
			{
				SetMat4(WaterShader, "Model", Model);
				DrawFromVAO(Chunk->WaterVAO, Chunk->WaterVerticesP.EntriesCount);

				if(RenderChunksBB)
				{
					DEBUGRenderCube(Chunk->Translation + 0.5f*ChunkDim, ChunkDim, 0.0f);
					UseShader(WaterShader);
				}
			}
		}
	}
	
	glDisable(GL_BLEND);
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
