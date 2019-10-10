#define CHUNK_DIM 16
#define IsBlockActive(Blocks, X, Y, Z) ((Blocks[(Z)*CHUNK_DIM*CHUNK_DIM + (Y)*CHUNK_DIM + (X)]).Active) 

struct block
{
	bool8 Active;
};
struct chunk_blocks_info
{
	block Blocks[CHUNK_DIM*CHUNK_DIM*CHUNK_DIM];

	chunk_blocks_info *Next;
};
struct chunk
{
	i32 X, Y, Z;

	bool32 IsReady;

	chunk_blocks_info *BlocksInfo;

	dynamic_array_vec3 VerticesP;
	dynamic_array_vec3 VerticesNormals;

	GLuint VAO, PVBO, NormalsVBO;

	vec3 Translation;

	chunk *Next;
};

struct world
{
	r32 ChunkDimInMeters;
	r32 BlockDimInMeters;
 
	chunk *ChunksToRender;

	chunk_blocks_info *FirstFreeChunkBlocksInfo;

	// NOTE(georgy): Must be a power of 2!
	chunk *ChunkHash[4096];
};

internal chunk *
GetChunk(world *World, u32 ChunkX, u32 ChunkY, u32 ChunkZ, stack_allocator *Allocator = 0)
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

		Chunk->Next = 0;

		*ChunkPtr = Chunk;
	}

	return(Chunk);
}

internal void
UnloadChunk(world *World, chunk *Chunk)
{
	Chunk->IsReady = false;

	Chunk->BlocksInfo->Next = World->FirstFreeChunkBlocksInfo;
	World->FirstFreeChunkBlocksInfo = Chunk->BlocksInfo->Next;

	FreeDynamicArray(&Chunk->VerticesP);
	FreeDynamicArray(&Chunk->VerticesNormals);

	glDeleteBuffers(1, &Chunk->PVBO);
	glDeleteBuffers(1, &Chunk->NormalsVBO);
	glDeleteVertexArrays(1, &Chunk->VAO);
}

internal void
RenderChunks(world *World, shader Shader)
{
	UseShader(Shader);
	for(chunk *Chunk = World->ChunksToRender;
		Chunk;
		Chunk = Chunk->Next)
	{
		glBindVertexArray(Chunk->VAO);
		mat4 Model = Translate(Chunk->Translation);
		SetMat4(Shader, "Model", Model);
		glDrawArrays(GL_TRIANGLES, 0, Chunk->VerticesP.EntriesCount);
	}

	World->ChunksToRender = 0;		
	glBindVertexArray(0);
}