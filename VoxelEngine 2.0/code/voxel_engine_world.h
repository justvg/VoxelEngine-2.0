#pragma once

#define CHUNK_DIM 32
#define MIN_CHUNKS_Y -1
#define MAX_CHUNKS_Y 3
#define IsBlockActive(Blocks, X, Y, Z) ((Blocks[(Z)*CHUNK_DIM*CHUNK_DIM + (Y)*CHUNK_DIM + (X)]).Active) 
#define GetBlockType(Blocks, X, Y, Z) ((Blocks[(Z)*CHUNK_DIM*CHUNK_DIM + (Y)*CHUNK_DIM + (X)]).Type) 

struct world_entity_block
{
	u32 StoredEntityCount;
	u32 StoredEntityIndex[16];

	world_entity_block *Next;
};

enum block_type
{
	BlockType_Null,
	
	BlockType_Snow,
	BlockType_Water,
};
struct block
{
	bool8 Active;
	u8 Type;
};
struct chunk_blocks_info
{
	block Blocks[CHUNK_DIM*CHUNK_DIM*CHUNK_DIM];
	vec4 Colors[CHUNK_DIM*CHUNK_DIM*CHUNK_DIM];

	chunk_blocks_info *Next;
};
struct chunk
{
	i32 X, Y, Z;

	bool32 IsRecentlyUsed;

	bool32 IsSetupBlocks;
	bool32 IsFullySetup;
	bool32 IsLoaded;
	bool32 IsModified;

	bool32 IsNotEmpty;

	chunk_blocks_info *BlocksInfo;

	// TODO(georgy): Do I want collapse these in one dynamic array? (AoS)
	// NOTE(georg): VerticesP vectors' w component stores AO value
	dynamic_array_vec3 VerticesP;
	dynamic_array_vec3 VerticesNormals;
	dynamic_array_vec4 VerticesColors;
	GLuint VAO, PVBO, NormalsVBO, ColorsVBO;

	// TODO(georgy): Do I want collapse these in one dynamic array? (AoS)
	// NOTE(georg): VerticesP vectors' w component stores AO value
	dynamic_array_vec3 WaterVerticesP;
	dynamic_array_vec4 WaterVerticesColors;
	GLuint WaterVAO, WaterPVBO, WaterColorsVBO;
	u32 MaxWaterLevel;

	vec3 Translation;
	r32 LengthSqTranslation;

	world_entity_block FirstEntityBlock;

	chunk *Next;
};

#define MAX_CHUNKS_TO_SETUP_BLOCKS 8
#define MAX_CHUNKS_TO_SETUP_FULLY 8
#define MAX_CHUNKS_TO_LOAD 8
struct world
{
	r32 ChunkDimInMeters;
	r32 BlockDimInMeters;
 
	u32 RecentlyUsedCount;
	chunk *ChunksToRender;

	chunk_blocks_info *FirstFreeChunkBlocksInfo;

	world_entity_block *FirstFreeWorldEntityBlock;

	// NOTE(georgy): Must be a power of 2!
	chunk *ChunkHash[4096];

	u32 Lock;

	u32 ChunksToSetupBlocksThisFrameCount; 
	chunk *ChunksToSetupBlocks[MAX_CHUNKS_TO_SETUP_BLOCKS];

	u32 ChunksToSetupFullyThisFrameCount; 
	chunk *ChunksToSetupFully[MAX_CHUNKS_TO_SETUP_FULLY];

	u32 ChunksToLoadThisFrameCount;
	chunk *ChunksToLoad[MAX_CHUNKS_TO_LOAD];
};

inline void
BeginWorldLock(world *World)
{
	for(;;)
	{
		if(AtomicCompareExchangeU32(&World->Lock, 1, 0) == 0)
		{
			break;
		}
	}
}

inline void
EndWorldLock(world *World)
{
	CompletePreviousWritesBeforeFutureWrites;
	World->Lock = 0;
}

struct world_position
{
	i32 ChunkX, ChunkY, ChunkZ;
	vec3 Offset;
};


inline vec3 Substract(world *World, world_position *A, world_position *B);