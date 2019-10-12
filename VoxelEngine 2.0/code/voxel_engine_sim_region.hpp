#include "voxel_engine_sim_region.h"

internal sim_region *
BeginSimulation(game_state *GameState, world *World, world_position Origin, vec3 Bounds, 
			   stack_allocator *WorldAllocator, stack_allocator *TempAllocator)
{
	sim_region *SimRegion = PushStruct(TempAllocator, sim_region);
	SimRegion->World = World;
	SimRegion->Origin = Origin;
	
	SimRegion->MaxEntityCount = 4096;
	SimRegion->EntityCount = 0;
	SimRegion->Entities = PushArray(TempAllocator, SimRegion->MaxEntityCount, sim_entity);

	world_position MinChunkP = MapIntoChunkSpace(World, &Origin, -Bounds);
	MinChunkP.ChunkY = 0;
	world_position MaxChunkP = MapIntoChunkSpace(World, &Origin, Bounds);
	MaxChunkP.ChunkY = 1;

	for(i32 ChunkZ = MinChunkP.ChunkZ;
		ChunkZ <= MaxChunkP.ChunkZ;
		ChunkZ++)
	{
		for(i32 ChunkY = MinChunkP.ChunkY;
			ChunkY <= MaxChunkP.ChunkY;
			ChunkY++)
		{
			for(i32 ChunkX = MinChunkP.ChunkX;
				ChunkX <= MaxChunkP.ChunkX;
				ChunkX++)
			{
				chunk *Chunk = GetChunk(World, ChunkX, ChunkY, ChunkZ, WorldAllocator);
				if(Chunk)
				{
					if(!IsRecentlyUsed(World, Chunk))
					{
						Chunk->NextRecentlyUsed = World->RecentlyUsedChunks;
						World->RecentlyUsedChunks = Chunk;

						World->RecentlyUsedCount++;
					}

					if(!Chunk->IsSetup)
					{
						SetupChunk(World, Chunk, WorldAllocator);
					}

					if(Chunk->IsSetup && !Chunk->IsLoaded)
					{
						LoadChunk(Chunk);
					}

					if(Chunk->IsSetup && Chunk->IsLoaded)
					{
						chunk *ChunkToRender = PushStruct(TempAllocator, chunk);
						*ChunkToRender = *Chunk;
						world_position ChunkPosition = { ChunkX, ChunkY, ChunkZ, vec3(0.0f, 0.0f, 0.0f) };
						ChunkToRender->Translation = Substract(World, &ChunkPosition, &Origin);
						ChunkToRender->Next = World->ChunksToRender;
						World->ChunksToRender = ChunkToRender;
					}

					for(world_entity_block *Block = &Chunk->FirstEntityBlock;
						Block;
						Block = Block->Next)
					{
						for(u32 EntityIndexInBlock = 0;
							EntityIndexInBlock < Block->StoredEntityCount;
							EntityIndexInBlock++)
						{
							u32 StoredEntityIndex = Block->StoredEntityIndex[EntityIndexInBlock];
							stored_entity *StoredEntity = GameState->StoredEntities + StoredEntityIndex;
							vec3 SimSpaceP = Substract(World, &StoredEntity->P, &Origin);
							//if(EntityInBounds)
							{
								sim_entity *Entity = SimRegion->Entities + SimRegion->EntityCount++;
								StoredEntity->Sim.P = SimSpaceP;

								*Entity = StoredEntity->Sim;
							}
						}
					}
				}
			}
		}
	}

	// TODO(georgy): Need more accurate value for this! Profile! Can be based upon sim region bounds!
#define MAX_CHUNKS_IN_MEMORY 1024
	if(World->RecentlyUsedCount >= MAX_CHUNKS_IN_MEMORY)
	{
		UnloadChunks(World, &MinChunkP, &MaxChunkP);
	}

	return(SimRegion);
}

internal void
EndSimulation(game_state *GameState, sim_region *SimRegion, stack_allocator *WorldAllocator)
{
	SimRegion->World->ChunksToRender = 0;			

	for(u32 EntityIndex = 0;
		EntityIndex < SimRegion->EntityCount;
		EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;
		stored_entity *StoredEntity = GameState->StoredEntities + Entity->StorageIndex;

		world_position NewP = MapIntoChunkSpace(SimRegion->World, &SimRegion->Origin, Entity->P);
		ChangeEntityLocation(SimRegion->World, WorldAllocator, Entity->StorageIndex, StoredEntity, NewP);
	}
}