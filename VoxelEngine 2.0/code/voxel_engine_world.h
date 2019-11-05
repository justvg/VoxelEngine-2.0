#pragma once

#define CHUNK_DIM 32
#define MAX_CHUNKS_Y 3
#define IsBlockActive(Blocks, X, Y, Z) ((Blocks[(Z)*CHUNK_DIM*CHUNK_DIM + (Y)*CHUNK_DIM + (X)]).Active) 

struct world_entity_block
{
	u32 StoredEntityCount;
	u32 StoredEntityIndex[16];

	world_entity_block *Next;
};

struct block
{
	bool8 Active;
};
struct chunk_blocks_info
{
	block Blocks[CHUNK_DIM*CHUNK_DIM*CHUNK_DIM];
	vec3 Colors[CHUNK_DIM*CHUNK_DIM*CHUNK_DIM];

	chunk_blocks_info *Next;
};
struct chunk
{
	i32 X, Y, Z;

	bool32 IsSetup;
	bool32 IsLoaded;

	bool32 IsNotEmpty;

	chunk_blocks_info *BlocksInfo;

	dynamic_array_vec3 VerticesP;
	dynamic_array_vec3 VerticesNormals;
	dynamic_array_vec3 VerticesColors;

	GLuint VAO, PVBO, NormalsVBO, ColorsVBO;

	vec3 Translation;

	world_entity_block FirstEntityBlock;

	chunk *Next;
	chunk *NextRecentlyUsed;
};

struct world
{
	r32 ChunkDimInMeters;
	r32 BlockDimInMeters;
 
	u32 RecentlyUsedCount;
	chunk *RecentlyUsedChunks;
	chunk *ChunksToRender;

	chunk_blocks_info *FirstFreeChunkBlocksInfo;

	world_entity_block *FirstFreeWorldEntityBlock;

	// NOTE(georgy): Must be a power of 2!
	chunk *ChunkHash[4096];
};

struct world_position
{
	i32 ChunkX, ChunkY, ChunkZ;
	vec3 Offset;
};