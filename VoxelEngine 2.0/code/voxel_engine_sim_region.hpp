#include "voxel_engine_sim_region.h"

internal entity_reference *
GetHashFromStorageIndex(sim_region *SimRegion, u32 StorageIndex)
{
	entity_reference *Result = 0;

	for(u32 Offset = 0;
		Offset < ArrayCount(SimRegion->Hash);
		Offset++)
	{
		u32 HashSlot = (StorageIndex + Offset) & (ArrayCount(SimRegion) - 1);
		entity_reference *Entry = SimRegion->Hash + HashSlot;
		if((Entry->StorageIndex == 0) || (Entry->StorageIndex == StorageIndex))
		{
			Result = Entry;
			break;
		}
	}

	return(Result);
}

inline vec3
GetSimSpaceP(sim_region *SimRegion, stored_entity *StoredEntity)
{
	vec3 Result = vec3((r32)INVALID_POSITION, (r32)INVALID_POSITION, (r32)INVALID_POSITION);
	if(!IsSet(&StoredEntity->Sim, EntityFlag_NonSpatial))
	{
		Result = Substract(SimRegion->World, &StoredEntity->P, &SimRegion->Origin);
	}

	return(Result);
}

internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, stored_entity *StoredEntity, vec3 SimSpaceP);
inline void
LoadEntityReference(game_state *GameState, sim_region *SimRegion, entity_reference *Ref)
{
	if(Ref->StorageIndex && !Ref->SimPtr)
	{
		stored_entity *StoredEntity = GameState->StoredEntities + Ref->StorageIndex;
		vec3 SimSpaceP = GetSimSpaceP(SimRegion, StoredEntity);
		Ref->SimPtr = AddEntity(GameState, SimRegion, StoredEntity, SimSpaceP);
	}
}

internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, stored_entity *StoredEntity, vec3 SimSpaceP)
{
	sim_entity *Entity = 0;

	entity_reference *Entry = GetHashFromStorageIndex(SimRegion, StoredEntity->Sim.StorageIndex);
	if(Entry->SimPtr == 0)
	{
		if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
		{
			Entity = SimRegion->Entities + SimRegion->EntityCount++;
			StoredEntity->Sim.P = SimSpaceP;
			StoredEntity->Sim.Updatable = IsInRect(SimRegion->UpdatableBounds, SimSpaceP);

			*Entity = StoredEntity->Sim;

			Entry->StorageIndex = Entity->StorageIndex;
			Entry->SimPtr = Entity;

			LoadEntityReference(GameState, SimRegion, &Entity->Fireball);
		}
	}
	else
	{
		Entity = Entry->SimPtr;
	}

	return(Entity);
}

internal sim_region *
BeginSimulation(game_state *GameState, world_position Origin, rect3 Bounds, 
				stack_allocator *TempAllocator, r32 dt)
{
	TIME_BLOCK;
	world *World = &GameState->World;
	stack_allocator *WorldAllocator = &GameState->WorldAllocator;

	sim_region *SimRegion = PushStruct(TempAllocator, sim_region);
	SimRegion->World = World;
	SimRegion->Origin = Origin;
	SimRegion->UpdatableBounds = Bounds;

	// TODO(georgy): More accurate values here!
	r32 MaxEntityRadius = 5.0f;
	r32 MaxEntityVelocity = 10.0f;
	r32 UpdateSafetyRadiusXZ = MaxEntityRadius + dt*MaxEntityVelocity + 1.0f;
	r32 UpdateSafetyRadiusY = 1.0f;
	SimRegion->Bounds = AddRadiusTo(SimRegion->UpdatableBounds, vec3(UpdateSafetyRadiusXZ, UpdateSafetyRadiusY, UpdateSafetyRadiusXZ));
	
	SimRegion->MaxEntityCount = 4096;
	SimRegion->EntityCount = 0;
	SimRegion->Entities = PushArray(TempAllocator, SimRegion->MaxEntityCount, sim_entity);

	ZeroArray(SimRegion->Hash, ArrayCount(SimRegion->Hash));

	world_position MinChunkP = MapIntoChunkSpace(World, &Origin, Bounds.Min);
	MinChunkP.ChunkY = MIN_CHUNKS_Y;
	world_position MaxChunkP = MapIntoChunkSpace(World, &Origin, Bounds.Max);
	MaxChunkP.ChunkY = MAX_CHUNKS_Y;

#if VOXEL_ENGINE_INTERNAL
	if(DEBUGGlobalPlaybackInfo.RecordPhaseStarted)
	{
		DEBUGGlobalPlaybackInfo.MinChunkP = MinChunkP;
		DEBUGGlobalPlaybackInfo.MaxChunkP = MaxChunkP;
		DEBUGGlobalPlaybackInfo.RecordPhaseStarted = false;
	}
#endif

	// NOTE(georgy): We add one extra boundary in Z and X to have ambient occlusion in all visible chunks
	for(i32 ChunkZ = MinChunkP.ChunkZ - 1;
		ChunkZ <= MaxChunkP.ChunkZ + 1;
		ChunkZ++)
	{
		for(i32 ChunkY = MinChunkP.ChunkY;
			ChunkY <= MaxChunkP.ChunkY;
			ChunkY++)
		{
			for(i32 ChunkX = MinChunkP.ChunkX - 1;
				ChunkX <= MaxChunkP.ChunkX + 1;
				ChunkX++)
			{
				chunk *Chunk = GetChunk(World, ChunkX, ChunkY, ChunkZ, WorldAllocator);
				if(Chunk)
				{
					if(!Chunk->IsRecentlyUsed)
					{
						Chunk->IsRecentlyUsed = true;
						World->RecentlyUsedCount++;
					}

					if((World->ChunksToSetupBlocksThisFrameCount < MAX_CHUNKS_TO_SETUP_BLOCKS) && 
					   (!Chunk->IsSetupBlocks))
					{
						World->ChunksToSetupBlocks[World->ChunksToSetupBlocksThisFrameCount++] = Chunk;
					}

					if((ChunkX > MinChunkP.ChunkX - 1) && (ChunkX < MaxChunkP.ChunkX + 1) &&
					   (ChunkZ > MinChunkP.ChunkZ - 1) && (ChunkZ < MaxChunkP.ChunkZ + 1))
					{
						if((World->ChunksToSetupFullyThisFrameCount < MAX_CHUNKS_TO_SETUP_FULLY) && 
							Chunk->IsSetupBlocks && !Chunk->IsFullySetup && CanSetupAO(World, Chunk, WorldAllocator))
						{
							World->ChunksToSetupFully[World->ChunksToSetupFullyThisFrameCount++] = Chunk;
						}

						if(Chunk->IsSetupBlocks && Chunk->IsFullySetup && Chunk->IsLoaded && Chunk->IsModified)
						{
							UpdateChunk(World, Chunk);
						}

						if((World->ChunksToLoadThisFrameCount < MAX_CHUNKS_TO_LOAD) && 
							Chunk->IsFullySetup && !Chunk->IsLoaded)
						{
							World->ChunksToLoad[World->ChunksToLoadThisFrameCount++] = Chunk;
						}

						if(Chunk->IsFullySetup && Chunk->IsLoaded && Chunk->IsNotEmpty)
						{
							world_position ChunkPosition = { ChunkX, ChunkY, ChunkZ, vec3(0.0f, 0.0f, 0.0f) };
							Chunk->Translation = Substract(World, &ChunkPosition, &Origin);
							chunk *ChunkToRender = PushStruct(TempAllocator, chunk);
							*ChunkToRender = *Chunk;
							ChunkToRender->LengthSqTranslation = LengthSq(ChunkToRender->Translation);

							ChunkToRender->Next = World->ChunksToRender;
							World->ChunksToRender = ChunkToRender;
						}

						if(Chunk->IsFullySetup && Chunk->IsLoaded)
						{
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
									if(!IsSet(&StoredEntity->Sim, EntityFlag_NonSpatial))
									{
										vec3 SimSpaceP = Substract(World, &StoredEntity->P, &Origin);
										rect3 EntityAABB = RectCenterDim(SimSpaceP + StoredEntity->Sim.Collision->OffsetP, StoredEntity->Sim.Collision->Dim);
										if(RectIntersect(SimRegion->Bounds, EntityAABB))
										{
											AddEntity(GameState, SimRegion, StoredEntity, SimSpaceP);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// TODO(georgy): Need more accurate value for this! Profile! Can be based upon sim region bounds!
#define MAX_CHUNKS_IN_MEMORY 2048
	if(World->RecentlyUsedCount >= MAX_CHUNKS_IN_MEMORY)
	{
		UnloadChunks(World, &MinChunkP, &MaxChunkP);
	}

	return(SimRegion);
}

internal void
EndSimulation(game_state *GameState, sim_region *SimRegion, stack_allocator *WorldAllocator)
{
	for(u32 EntityIndex = 0;
		EntityIndex < SimRegion->EntityCount;
		EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;
		Entity->Fireball.SimPtr = 0;
		stored_entity *StoredEntity = GameState->StoredEntities + Entity->StorageIndex;
		StoredEntity->Sim = *Entity;

		world_position NewP = MapIntoChunkSpace(SimRegion->World, &SimRegion->Origin, Entity->P);
		ChangeEntityLocation(SimRegion->World, WorldAllocator, Entity->StorageIndex, StoredEntity, NewP);
	}

	SimRegion->World->ChunksToRender = 0;			
}

internal bool32
GetLowestRoot(r32 A, r32 B, r32 C, r32 *t)
{
	bool32 Result = false;

	r32 Descriminant = B*B - 4.0f*A*C;

	if(Descriminant >= 0.0f)
	{
		r32 SqrtD = SquareRoot(Descriminant);
		r32 R1 = (-B - SqrtD) / (2.0f*A);
		r32 R2 = (-B + SqrtD) / (2.0f*A);
		
		if(R1 > R2)
		{
			r32 Temp = R1;
			R1 = R2;
			R2 = Temp;
		}

		if((R1 > 0.0f) && (R1 < *t))
		{
			*t = R1;
			Result = true;
		}
		else if((R2 > 0.0f) && (R2 < *t))
		{
			*t = R2;
			Result = true;
		}
	}

	return(Result);
}

struct collided_chunk_info
{
	chunk *Chunk;
	vec3 Min, Max;
};

struct broad_phase_chunk_collision_detection
{
	u32 CollideChunksCount = 0;
	collided_chunk_info CollideChunks[27];
};
internal broad_phase_chunk_collision_detection
BroadPhaseChunkCollisionDetection(world *World, world_position WorldP, rect3 AABB)
{
	broad_phase_chunk_collision_detection Result = {};

	for(i32 ChunkZ = WorldP.ChunkZ - 1;
		ChunkZ <= WorldP.ChunkZ + 1;
		ChunkZ++)
	{
		for(i32 ChunkY = WorldP.ChunkY - 1;
			ChunkY <= WorldP.ChunkY + 1;
			ChunkY++)
		{
			for(i32 ChunkX = WorldP.ChunkX - 1;
				ChunkX <= WorldP.ChunkX + 1;
				ChunkX++)
			{
				chunk *Chunk = GetChunk(World, ChunkX, ChunkY, ChunkZ);
				if(Chunk)
				{
					r32 ChunkDimInMeters = World->ChunkDimInMeters;
					vec3 SimSpaceChunkAABBMin = Chunk->Translation;
					vec3 SimSpaceChunkAABBMax = SimSpaceChunkAABBMin + vec3(ChunkDimInMeters, ChunkDimInMeters, ChunkDimInMeters);
					rect3 SimSpaceChunkAABB = RectMinMax(SimSpaceChunkAABBMin, SimSpaceChunkAABBMax);

					if(RectIntersect(SimSpaceChunkAABB, AABB))
					{
						vec3 MinOffset = AABB.Min - SimSpaceChunkAABB.Min;
						vec3 MaxOffset = AABB.Max - SimSpaceChunkAABB.Min;
						Result.CollideChunks[Result.CollideChunksCount].Min = MinOffset;
						Result.CollideChunks[Result.CollideChunksCount].Max = MaxOffset;

						Result.CollideChunks[Result.CollideChunksCount++].Chunk = Chunk;
					}
				}
			}	
		}	
	}

	return(Result);
}

struct ellipsoid_triangle_collision_detection_data
{
	mat3 WorldToEllipsoid;
	vec3 NormalizedESpaceEntityDelta;
	vec3 ESpaceP;
	vec3 ESpaceEntityDelta;
	r32 t;
	vec3 CollisionP;
	entity_type HitEntityType;
};

internal bool32
EllipsoidTriangleCollisionDetection(vec3 P1, vec3 P2, vec3 P3, ellipsoid_triangle_collision_detection_data *EllipsoidTriangleData)
{
	mat3 WorldToEllipsoid = EllipsoidTriangleData->WorldToEllipsoid;
	vec3 NormalizedESpaceEntityDelta = EllipsoidTriangleData->NormalizedESpaceEntityDelta;
	vec3 ESpaceP = EllipsoidTriangleData->ESpaceP;
	vec3 ESpaceEntityDelta = EllipsoidTriangleData->ESpaceEntityDelta;
	r32 *t = &EllipsoidTriangleData->t;
	vec3 *CollisionP = &EllipsoidTriangleData->CollisionP;

	bool32 Result = false;

	bool32 FoundCollisionWithInsideTheTriangle = false;

	vec3 EP1 = WorldToEllipsoid * P1;
	vec3 EP2 = WorldToEllipsoid * P2;
	vec3 EP3 = WorldToEllipsoid * P3;

	plane TrianglePlane = PlaneFromTriangle(EP1, EP2, EP3);

	if (Dot(TrianglePlane.Normal, NormalizedESpaceEntityDelta) <= 0)
	{
		r32 t0, t1;
		bool32 EmbeddedInPlane = false;

		r32 SignedDistanceToTrianglePlane = SignedDistance(TrianglePlane, ESpaceP);
		r32 NormalDotEntityDelta = Dot(TrianglePlane.Normal, ESpaceEntityDelta);

		if (NormalDotEntityDelta == 0.0f)
		{
			if (fabs(SignedDistanceToTrianglePlane) >= 1.0f)
			{
				return(Result);
			}
			else
			{
				EmbeddedInPlane = true;
				t0 = 0.0f;
				t1 = 1.0f;
			}
		}
		else
		{
			t0 = (1.0f - SignedDistanceToTrianglePlane) / NormalDotEntityDelta;
			t1 = (-1.0f - SignedDistanceToTrianglePlane) / NormalDotEntityDelta;

			if (t0 > t1)
			{
				r32 Temp = t0;
				t0 = t1;
				t1 = Temp;
			}

			if ((t0 > 1.0f) || (t1 < 0.0f))
			{
				return(Result);
			}

			if (t0 < 0.0f)
			{
				t0 = 0.0f;
			}
			if (t1 > 1.0f)
			{
				t1 = 1.0f;
			}
		}

		if (!EmbeddedInPlane)
		{
			vec3 PlaneIntersectionPoint = ESpaceP + t0 * ESpaceEntityDelta - TrianglePlane.Normal;

			if (IsPointInTriangle(PlaneIntersectionPoint, EP1, EP2, EP3))
			{
				FoundCollisionWithInsideTheTriangle = true;
				if (t0 < *t)
				{
					Result = true;
					*t = t0;
					*CollisionP = PlaneIntersectionPoint;
				}
			}
		}

		if (!FoundCollisionWithInsideTheTriangle)
		{
			r32 EntityDeltaSquaredLength = LengthSq(ESpaceEntityDelta);
			r32 A, B, C;
			r32 NewT = *t;

			// NOTE(georgy): Check triangle vertices
			A = EntityDeltaSquaredLength;

			B = 2.0f * (Dot(ESpaceEntityDelta, ESpaceP - EP1));
			C = LengthSq((EP1 - ESpaceP)) - 1.0f;
			if (GetLowestRoot(A, B, C, &NewT))
			{
				Result = true;
				*t = NewT;
				*CollisionP = EP1;
			}

			B = 2.0f * (Dot(ESpaceEntityDelta, ESpaceP - EP2));
			C = LengthSq((EP2 - ESpaceP)) - 1.0f;
			if (GetLowestRoot(A, B, C, &NewT))
			{
				Result = true;
				*t = NewT;
				*CollisionP = EP2;
			}

			B = 2.0f * (Dot(ESpaceEntityDelta, ESpaceP - EP3));
			C = LengthSq((EP3 - ESpaceP)) - 1.0f;
			if (GetLowestRoot(A, B, C, &NewT))
			{
				Result = true;
				*t = NewT;
				*CollisionP = EP3;
			}

			// NOTE(georgy): Check triangle edges
			vec3 Edge = EP2 - EP1;
			vec3 BaseToVertex = EP1 - ESpaceP;
			r32 EdgeSquaredLength = LengthSq(Edge);
			r32 EdgeDotdP = Dot(Edge, ESpaceEntityDelta);
			r32 EdgeDotBaseToVertex = Dot(BaseToVertex, Edge);

			A = EdgeSquaredLength * -EntityDeltaSquaredLength +
				EdgeDotdP * EdgeDotdP;
			B = EdgeSquaredLength * (2.0f * Dot(ESpaceEntityDelta, BaseToVertex)) -
				2.0f * EdgeDotdP * EdgeDotBaseToVertex;
			C = EdgeSquaredLength * (1.0f - LengthSq(BaseToVertex)) +
				EdgeDotBaseToVertex * EdgeDotBaseToVertex;
			if (GetLowestRoot(A, B, C, &NewT))
			{
				r32 f = (EdgeDotdP * NewT - EdgeDotBaseToVertex) / EdgeSquaredLength;
				if ((f >= 0.0f) && (f <= 1.0f))
				{
					Result = true;
					*t = NewT;
					*CollisionP = EP1 + f * Edge;
				}
			}

			Edge = EP3 - EP2;
			BaseToVertex = EP2 - ESpaceP;
			EdgeSquaredLength = LengthSq(Edge);
			EdgeDotdP = Dot(Edge, ESpaceEntityDelta);
			EdgeDotBaseToVertex = Dot(BaseToVertex, Edge);

			A = EdgeSquaredLength * -EntityDeltaSquaredLength +
				EdgeDotdP * EdgeDotdP;
			B = EdgeSquaredLength * (2.0f * Dot(ESpaceEntityDelta, BaseToVertex)) -
				2.0f * EdgeDotdP * EdgeDotBaseToVertex;
			C = EdgeSquaredLength * (1.0f - LengthSq(BaseToVertex)) +
				EdgeDotBaseToVertex * EdgeDotBaseToVertex;
			if (GetLowestRoot(A, B, C, &NewT))
			{
				r32 f = (EdgeDotdP * NewT - EdgeDotBaseToVertex) / EdgeSquaredLength;
				if ((f >= 0.0f) && (f <= 1.0f))
				{
					Result = true;
					*t = NewT;
					*CollisionP = EP2 + f * Edge;
				}
			}

			Edge = EP1 - EP3;
			BaseToVertex = EP3 - ESpaceP;
			EdgeSquaredLength = LengthSq(Edge);
			EdgeDotdP = Dot(Edge, ESpaceEntityDelta);
			EdgeDotBaseToVertex = Dot(BaseToVertex, Edge);

			A = EdgeSquaredLength * -EntityDeltaSquaredLength +
				EdgeDotdP * EdgeDotdP;
			B = EdgeSquaredLength * (2.0f * Dot(ESpaceEntityDelta, BaseToVertex)) -
				2.0f * EdgeDotdP * EdgeDotBaseToVertex;
			C = EdgeSquaredLength * (1.0f - LengthSq(BaseToVertex)) +
				EdgeDotBaseToVertex * EdgeDotBaseToVertex;
			if (GetLowestRoot(A, B, C, &NewT))
			{
				r32 f = (EdgeDotdP * NewT - EdgeDotBaseToVertex) / EdgeSquaredLength;
				if ((f >= 0.0f) && (f <= 1.0f))
				{
					Result = true;
					*t = NewT;
					*CollisionP = EP3 + f * Edge;
				}
			}
		}
	}

	return(Result);
}

struct ray_triangle_collision_detection_data
{
	vec3 RayOrigin;
	vec3 RayEnd;
	vec3 RayDirection;
	r32 t;

	bool32 Water;
};

enum narrow_phase_collision_detection_type
{
	NarrowPhaseCollisionDetection_EllipsoidTriangle,
	NarrowPhaseCollisionDetection_RayTriangle,
};
internal void
NarrowPhaseCollisionDetection(world *World, broad_phase_chunk_collision_detection *BroadPhaseData,
							  narrow_phase_collision_detection_type Type, bool32 CollideWithWater,
							  ray_triangle_collision_detection_data *RayTriangleData = 0,
							  ellipsoid_triangle_collision_detection_data *EllipsoidTriangleData = 0)
{
	u32 CollideChunksCount = BroadPhaseData->CollideChunksCount;
	collided_chunk_info *CollideChunks = BroadPhaseData->CollideChunks;

	for(u32 ChunkIndex = 0;
		ChunkIndex < CollideChunksCount;
		ChunkIndex++)
	{
		chunk *Chunk = CollideChunks[ChunkIndex].Chunk;
		vec3 *Vertices = Chunk->VerticesP.Entries;
		r32 BlockDimInMeters = World->BlockDimInMeters;
		vec3 MinChunkP = CollideChunks[ChunkIndex].Min - vec3(BlockDimInMeters, BlockDimInMeters, BlockDimInMeters);
		vec3 MaxChunkP = CollideChunks[ChunkIndex].Max + vec3(BlockDimInMeters, BlockDimInMeters, BlockDimInMeters);
		for(u32 TriangleIndex = 0;
			TriangleIndex < (Chunk->VerticesP.EntriesCount / 3);
			TriangleIndex++)
		{
			u32 FirstVertexIndex = TriangleIndex * 3;
			vec3 P1 = Vertices[FirstVertexIndex];
			vec3 P2 = Vertices[FirstVertexIndex + 1];
			vec3 P3 = Vertices[FirstVertexIndex + 2];

			vec3 MinP = Min(Min(P1, P2), P3);
			vec3 MaxP = Max(Max(P1, P2), P3);
			
			if(All(MinP < MaxChunkP) &&
			   All(MaxP > MinChunkP))
			{
				P1 += Chunk->Translation;
				P2 += Chunk->Translation;
				P3 += Chunk->Translation;

				if(Type == NarrowPhaseCollisionDetection_RayTriangle)
				{
					Assert(RayTriangleData);

					plane TrianglePlane = PlaneFromTriangle(P1, P2, P3);
					r32 NormalDotDir = Dot(TrianglePlane.Normal, RayTriangleData->RayDirection);
					if(NormalDotDir < -0.0001f)
					{
						r32 NewRayLength = -SignedDistance(TrianglePlane, RayTriangleData->RayOrigin) / NormalDotDir;
						r32 NewT = NewRayLength / Length(RayTriangleData->RayEnd - RayTriangleData->RayOrigin);
						if((NewT < RayTriangleData->t) && (NewT > 0.0f))
						{
							if(IsPointInTriangle(RayTriangleData->RayOrigin + NewRayLength*RayTriangleData->RayDirection, P1, P2, P3))
							{
								RayTriangleData->t = NewT;
								RayTriangleData->Water = false;
							}
						}
					}
				}
				else
				{
					Assert(Type == NarrowPhaseCollisionDetection_EllipsoidTriangle);
					Assert(EllipsoidTriangleData);

					if(EllipsoidTriangleCollisionDetection(P1, P2, P3, EllipsoidTriangleData))
					{
						EllipsoidTriangleData->HitEntityType = EntityType_Chunk;
					}
				}
			}
		}

		if(CollideWithWater)
		{
			if((Chunk->WaterVerticesP.EntriesCount > 0))
			{
				vec3 *WaterVertices = Chunk->WaterVerticesP.Entries;
				for(u32 TriangleIndex = 0;
					TriangleIndex < (Chunk->WaterVerticesP.EntriesCount / 3);
					TriangleIndex++)
				{
					u32 FirstVertexIndex = TriangleIndex * 3;
					vec3 P1 = WaterVertices[FirstVertexIndex];
					vec3 P2 = WaterVertices[FirstVertexIndex + 1];
					vec3 P3 = WaterVertices[FirstVertexIndex + 2];

					vec3 MinP = Min(Min(P1, P2), P3);
					vec3 MaxP = Max(Max(P1, P2), P3);
					
					if(All(MinP < MaxChunkP) &&
					   All(MaxP > MinChunkP))
					{
						P1 += Chunk->Translation;
						P2 += Chunk->Translation;
						P3 += Chunk->Translation;

						P1 -= vec3(0.0f, 0.3f, 0.0f);
						P2 -= vec3(0.0f, 0.3f, 0.0f);
						P3 -= vec3(0.0f, 0.3f, 0.0f);

						if(Type == NarrowPhaseCollisionDetection_RayTriangle)
						{
							Assert(RayTriangleData);

							plane TrianglePlane = PlaneFromTriangle(P1, P2, P3);
							r32 NormalDotDir = Dot(TrianglePlane.Normal, RayTriangleData->RayDirection);
							if(NormalDotDir < -0.0001f)
							{
								r32 NewRayLength = -SignedDistance(TrianglePlane, RayTriangleData->RayOrigin) / NormalDotDir;
								r32 NewT = NewRayLength / Length(RayTriangleData->RayEnd - RayTriangleData->RayOrigin);
								if((NewT < RayTriangleData->t) && (NewT > 0.0f))
								{
									if(IsPointInTriangle(RayTriangleData->RayOrigin + NewRayLength*RayTriangleData->RayDirection, P1, P2, P3))
									{
										RayTriangleData->t = NewT;
										RayTriangleData->Water = true;
									}
								}
							}
						}
						else
						{
							Assert(Type == NarrowPhaseCollisionDetection_EllipsoidTriangle);
							Assert(EllipsoidTriangleData);

							if(EllipsoidTriangleCollisionDetection(P1, P2, P3, EllipsoidTriangleData))
							{
								EllipsoidTriangleData->HitEntityType = EntityType_Water;
							}
						}
					}
				}
			}
		}
	}
}

internal void
CameraCollisionDetection(world *World, camera *Camera, world_position *OriginP)
{
	TIME_BLOCK;
	r32 NearDistance = Camera->NearDistance;
	r32 FoV = Camera->FoV;
	r32 NearPlaneHalfHeight = Tan(DEG2RAD(FoV)*0.5f)*NearDistance;
	r32 NearPlaneHalfWidth = NearPlaneHalfHeight*Camera->AspectRatio;

	vec3 CameraRight = vec3(Camera->RotationMatrix.FirstColumn.x(), Camera->RotationMatrix.SecondColumn.x(), Camera->RotationMatrix.ThirdColumn.x());;
	vec3 CameraUp = vec3(Camera->RotationMatrix.FirstColumn.y(), Camera->RotationMatrix.SecondColumn.y(), Camera->RotationMatrix.ThirdColumn.y());;
	vec3 CameraOut = -vec3(Camera->RotationMatrix.FirstColumn.z(), Camera->RotationMatrix.SecondColumn.z(), Camera->RotationMatrix.ThirdColumn.z());
	ray_triangle_collision_detection_data RayTriangleData;
	RayTriangleData.t = 1.0f;

	vec3 OriginPos = Camera->TargetOffset;
	vec3 RayStartCornerRightBot = OriginPos + 
								  CameraRight*NearPlaneHalfWidth - CameraUp*NearPlaneHalfHeight;
	vec3 RayStartCornerRightTop = OriginPos + 
								  CameraRight*NearPlaneHalfWidth + CameraUp*NearPlaneHalfHeight;
	vec3 RayStartCornerLeftBot = OriginPos - 
							     CameraRight*NearPlaneHalfWidth - CameraUp*NearPlaneHalfHeight;
	vec3 RayStartCornerLeftTop = OriginPos - 
								 CameraRight*NearPlaneHalfWidth + CameraUp*NearPlaneHalfHeight;

	vec3 RayStartCorners[] = 
	{
		RayStartCornerRightBot,
		RayStartCornerRightTop,
		RayStartCornerLeftBot,
		RayStartCornerLeftTop
	};

	vec3 CameraNearCentreP = Camera->OffsetFromHero + Camera->TargetOffset + NearDistance*CameraOut;
	vec3 FrustumNearCornerRightBot = CameraNearCentreP + 
									 CameraRight*NearPlaneHalfWidth - CameraUp*NearPlaneHalfHeight;
	vec3 FrustumNearCornerRightTop = CameraNearCentreP + 
									 CameraRight*NearPlaneHalfWidth + CameraUp*NearPlaneHalfHeight;
	vec3 FrustumNearCornerLeftBot = CameraNearCentreP - 
									 CameraRight*NearPlaneHalfWidth - CameraUp*NearPlaneHalfHeight;
	vec3 FrustumNearCornerLeftTop = CameraNearCentreP - 
									 CameraRight*NearPlaneHalfWidth + CameraUp*NearPlaneHalfHeight;
	vec3 FrustumNearCorners[] = 
	{
		FrustumNearCornerRightBot,
		FrustumNearCornerRightTop,
		FrustumNearCornerLeftBot,
		FrustumNearCornerLeftTop
	};

	rect3 CameraCollisionVolumeAABB;
	CameraCollisionVolumeAABB.Min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	CameraCollisionVolumeAABB.Max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for(u32 PointIndex = 0;
		PointIndex < 4;
		PointIndex++)
	{
		CameraCollisionVolumeAABB.Min = Min(CameraCollisionVolumeAABB.Min, RayStartCorners[PointIndex]);
		CameraCollisionVolumeAABB.Max = Max(CameraCollisionVolumeAABB.Max, RayStartCorners[PointIndex]);
	}
	for(u32 PointIndex = 0;
		PointIndex < 4;
		PointIndex++)
	{
		CameraCollisionVolumeAABB.Min = Min(CameraCollisionVolumeAABB.Min, FrustumNearCorners[PointIndex]);
		CameraCollisionVolumeAABB.Max = Max(CameraCollisionVolumeAABB.Max, FrustumNearCorners[PointIndex]);
	}

	broad_phase_chunk_collision_detection BroadPhaseResult = BroadPhaseChunkCollisionDetection(World, *OriginP, CameraCollisionVolumeAABB);

	for(u32 RayIndex = 0;
		RayIndex < 4;
		RayIndex++)
	{
		RayTriangleData.RayOrigin = RayStartCorners[RayIndex];
		RayTriangleData.RayEnd = FrustumNearCorners[RayIndex];
		RayTriangleData.RayDirection = Normalize(FrustumNearCorners[RayIndex] - RayStartCorners[RayIndex]);

		NarrowPhaseCollisionDetection(World, &BroadPhaseResult, NarrowPhaseCollisionDetection_RayTriangle, false, &RayTriangleData);
	}

    Camera->OffsetFromHero = Camera->OffsetFromHero * RayTriangleData.t;
}

internal bool32
CanCollide(game_state *GameState, sim_entity *A, sim_entity *B)
{
	bool32 Result = false;

	if(A != B)
	{
		if(IsSet(A, EntityFlag_Collides) && IsSet(B, EntityFlag_Collides))
		{
			if (A->StorageIndex > B->StorageIndex)
			{
				sim_entity *Temp = A;
				A = B;
				B = Temp;
			}

			if(!IsSet(A, EntityFlag_NonSpatial) && !IsSet(B, EntityFlag_NonSpatial))
			{
				Result = true;
			}

			for(u32 CollisionRuleIndex = 0;
				CollisionRuleIndex < ArrayCount(GameState->CollisionRules);
				CollisionRuleIndex++)
			{
				pairwise_collision_rule *CollisionRule = GameState->CollisionRules + CollisionRuleIndex;
				if((CollisionRule->StorageIndexA == A->StorageIndex) &&
				   (CollisionRule->StorageIndexB == B->StorageIndex))
				{
					Result = CollisionRule->CanCollide;
					break;
				}
			}
		}
	}

	return(Result);
}

internal bool32
HandleCollision(game_state *GameState, sim_region *SimRegion, sim_entity *EntityA, sim_entity *EntityB, entity_type A, entity_type B,
				mat3 EllipsoidToWorld, vec3 ESpaceCollisionP)
{
	bool32 Result;
	world *World = &GameState->World;

	if(A == EntityType_Fireball)
	{
		if(EntityB)
		{
			AddCollisionRule(GameState, EntityA->StorageIndex, EntityB->StorageIndex, false);
		}
	}

	if((A == EntityType_Fireball) || (B == EntityType_Fireball))
	{
		Result = false;
	}
	else
	{
		Result = true;
	}

	if(A > B)
	{
		entity_type Temp = A;
		A = B;
		B = Temp;

		sim_entity *TempEntity = EntityA;
		EntityA = EntityB;
		EntityB = TempEntity; 
	}

	if((A == EntityType_Chunk) && (B == EntityType_Fireball))
	{
		MakeEntityNonSpatial(EntityB);

		world_position CollisionWorldP = MapIntoChunkSpace(World, &SimRegion->Origin, EllipsoidToWorld * ESpaceCollisionP);
		chunk *Chunk = GetChunk(World, CollisionWorldP.ChunkX, CollisionWorldP.ChunkY, CollisionWorldP.ChunkZ);
		Assert(Chunk && Chunk->IsSetupBlocks && Chunk->IsFullySetup && Chunk->IsLoaded);

		for(i32 BlockZ = -2;
			BlockZ <= 2;
			BlockZ++)
		{
			for(i32 BlockY = -2;
				BlockY <= 2;
				BlockY++)
			{
				for(i32 BlockX = -2;
					BlockX <= 2;
					BlockX++)
				{
					i32 X = (i32)((CollisionWorldP.Offset.x() + World->BlockDimInMeters*BlockX) / World->BlockDimInMeters);
					i32 Y = (i32)((CollisionWorldP.Offset.y() + World->BlockDimInMeters*BlockY) / World->BlockDimInMeters);
					i32 Z = (i32)((CollisionWorldP.Offset.z() + World->BlockDimInMeters*BlockZ) / World->BlockDimInMeters);
					//if(Chunk->BlocksInfo->Blocks[Z*CHUNK_DIM*CHUNK_DIM + Y*CHUNK_DIM + X].Type == BlockType_Snow)
					{
						bool32 Active = IsBlockActiveBetweenChunks(World, Chunk, X, Y, Z);
						if(Active)
						{
							vec3 OffsetInChunk = World->BlockDimInMeters*vec3((r32)X, (r32)Y, (r32)Z) + 
												 0.5f*vec3(World->BlockDimInMeters, World->BlockDimInMeters, World->BlockDimInMeters);
							world_position WorldBlockP = { Chunk->X, Chunk->Y, Chunk->Z, OffsetInChunk };
							RecanonicalizeCoords(World, &WorldBlockP);
							vec4 BlockColor;
							u32 Op = OperationBetweenChunks_SetActiveness |
									 OperationBetweenChunks_GetColor;
							OperationBetweenChunks(World, Chunk, X, Y, Z, (operation_between_chunks)Op, false, &BlockColor);
							AddBlockParticle(&GameState->BlockParticleGenerator, WorldBlockP, vec3(BlockColor.m));
						}
					} 
				}	
			}
		}
	}
	else if((A == EntityType_Chunk) && (B == EntityType_Hero))
	{
		EntityB->HitPoints++;
		if(EntityB->HitPoints > 100)
		{
			EntityB->HitPoints = 0;
		}
	}
	else if((A == EntityType_Fireball) && (B == 10000))
	{
		EntityB->HitPoints -= 10;
		if(EntityB->HitPoints <= 0)
		{
			EntityB->HitPoints = 0;
			MakeEntityNonSpatial(EntityB);
		}
	}
	else if((A == EntityType_Water) && (B == EntityType_Hero))
	{
		// Result = false;
		if(!IsSet(EntityB, EntityFlag_InWater))
		{
			AddFlags(EntityB, EntityFlag_InWater);
			EntityB->dP.SetY(0.0f);
		}

		world_position CollisionWorldP = MapIntoChunkSpace(World, &SimRegion->Origin, EllipsoidToWorld * ESpaceCollisionP);
		chunk *Chunk = GetChunk(World, CollisionWorldP.ChunkX, CollisionWorldP.ChunkY, CollisionWorldP.ChunkZ);
		Assert(Chunk && Chunk->IsSetupBlocks && Chunk->IsFullySetup && Chunk->IsLoaded);

		i32 X = (i32)((CollisionWorldP.Offset.x()) / World->BlockDimInMeters);
		i32 Y = (i32)((CollisionWorldP.Offset.y()) / World->BlockDimInMeters);
		i32 Z = (i32)((CollisionWorldP.Offset.z()) / World->BlockDimInMeters);
		{
			vec3 OffsetInChunk = World->BlockDimInMeters*vec3((r32)X, (r32)Y, (r32)Z) + 
									0.5f*vec3(World->BlockDimInMeters, World->BlockDimInMeters, World->BlockDimInMeters);
			world_position WorldBlockP = { Chunk->X, Chunk->Y, Chunk->Z, OffsetInChunk };
			RecanonicalizeCoords(World, &WorldBlockP);
			vec4 BlockColor;
			u32 Op = OperationBetweenChunks_GetColor;
			OperationBetweenChunks(World, Chunk, X, Y, Z, (operation_between_chunks)Op, false, &BlockColor);
			AddBlockParticle(&GameState->BlockParticleGenerator, WorldBlockP, vec3(BlockColor.m));
			AddBlockParticle(&GameState->BlockParticleGenerator, WorldBlockP, vec3(BlockColor.m));
			AddBlockParticle(&GameState->BlockParticleGenerator, WorldBlockP, vec3(BlockColor.m));
			AddBlockParticle(&GameState->BlockParticleGenerator, WorldBlockP, vec3(BlockColor.m));
			AddBlockParticle(&GameState->BlockParticleGenerator, WorldBlockP, vec3(BlockColor.m));
		} 
	}

	return(Result);
}

internal void
MoveEntity(game_state *GameState, sim_region *SimRegion, sim_entity *Entity, move_spec MoveSpec, r32 dt, bool32 Gravity)
{
	mat3 EllipsoidToWorld = Rotate3x3(Entity->Rotation, vec3(0.0f, 1.0f, 0.0f)) * 
							Scale3x3(0.5f*Entity->Collision->Dim);
	mat3 WorldToEllipsoid = Scale3x3(1.0f / (0.5f*Entity->Collision->Dim)) * 
							Rotate3x3(-Entity->Rotation, vec3(0.0f, 1.0f, 0.0f));

	ellipsoid_triangle_collision_detection_data EllipsoidTriangleData;
	EllipsoidTriangleData.WorldToEllipsoid = WorldToEllipsoid;

	if(Gravity && Entity->CanBeGravityAffected && IsSet(Entity, EntityFlag_GravityAffected))
	{
		MoveSpec.ddP = vec3(0.0f, -9.8f, 0.0f);
	}
	else
	{
		r32 ddPLength = LengthSq(MoveSpec.ddP);
		if(ddPLength > 1.0f)
		{
			MoveSpec.ddP = Normalize(MoveSpec.ddP);
		}
		MoveSpec.ddP *= MoveSpec.Speed;
		vec3 DragV = -MoveSpec.Drag*Entity->dP;
		DragV.SetY(0.0f);
		MoveSpec.ddP += DragV;
	}

	vec3 CollisionVolumeDisplacement = Entity->Collision->OffsetP;
	vec3 CollisionEntityP = Entity->P + CollisionVolumeDisplacement;
	vec3 OldP = CollisionEntityP;
	vec3 DesiredP = (0.5f*MoveSpec.ddP*dt*dt) + (Entity->dP*dt) + CollisionEntityP;

	vec3 EntityDelta = DesiredP - CollisionEntityP;
	Entity->dP += MoveSpec.ddP*dt;

	// NOTE(georgy): Collision broad phase
	stored_entity *StoredEntity = GameState->StoredEntities + Entity->StorageIndex;
	world_position OldWorldP = StoredEntity->P;
	
	box EntityBox = ConstructBoxDim(Entity->Collision->Dim);
	mat3 EntityRotationMatrix = Rotate3x3(Entity->Rotation, vec3(0.0f, 1.0f, 0.0f)); 

	box EntityBoxOldP = ConstructBox(&EntityBox, EntityRotationMatrix, OldP);
	rect3 EntityOldPAABB = RectFromBox(&EntityBoxOldP);

	box EntityBoxDesiredP = ConstructBox(&EntityBox, EntityRotationMatrix, DesiredP);
	rect3 EntityDesiredPAABB = RectFromBox(&EntityBoxDesiredP);

	vec3 MinForAABB = Min(EntityOldPAABB.Min, EntityDesiredPAABB.Min);
	vec3 MaxForAABB = Max(EntityOldPAABB.Max, EntityDesiredPAABB.Max);
	rect3 AABB = RectMinMax(MinForAABB, MaxForAABB);

	broad_phase_chunk_collision_detection BroadPhaseResult = BroadPhaseChunkCollisionDetection(SimRegion->World, OldWorldP, AABB);

	u32 CollideEntitiesCount = 0;
	sim_entity *CollideEntities[10];
	for(u32 TestEntityIndex = 0;
		TestEntityIndex < SimRegion->EntityCount;
		TestEntityIndex++)
	{
		sim_entity *TestEntity = SimRegion->Entities + TestEntityIndex;

		if(CanCollide(GameState, Entity, TestEntity))
		{
			box TestEntityBox = ConstructBoxDim(TestEntity->Collision->Dim);
			mat3 TestEntityTransformation = Rotate3x3(TestEntity->Rotation, vec3(0.0f, 1.0f, 0.0f));
			TransformBox(&TestEntityBox, TestEntityTransformation, TestEntity->P + TestEntity->Collision->OffsetP);
			rect3 TestEntityAABB = RectFromBox(&TestEntityBox);

			if(RectIntersect(TestEntityAABB, EntityOldPAABB) || RectIntersect(TestEntityAABB, EntityDesiredPAABB))
			{
				CollideEntities[CollideEntitiesCount++] = TestEntity;
			}
		}
	}

	// NOTE(georgy): Collision narrow phase
	vec3 ESpaceEntityDelta = WorldToEllipsoid * EntityDelta;
	vec3 ESpaceP = WorldToEllipsoid * CollisionEntityP;

	r32 DistanceRemaining = Entity->DistanceLimit;
	if(DistanceRemaining == 0.0f)
	{
		DistanceRemaining = 10000.0f;
	}

	bool32 MoveOnBlock = false;
	for(u32 Iteration = 0;
		Iteration < 4;
		Iteration++)
	{
		if(!IsSet(Entity, EntityFlag_NonSpatial))
		{
			r32 EntityDeltaLength = Length(ESpaceEntityDelta);
			if(EntityDeltaLength > 0.0f)
			{
				vec3 ESpaceDesiredP = ESpaceP + ESpaceEntityDelta;
				vec3 NormalizedESpaceEntityDelta = Normalize(ESpaceEntityDelta);
				sim_entity *HitEntity = 0;

				EllipsoidTriangleData.NormalizedESpaceEntityDelta = NormalizedESpaceEntityDelta;
				EllipsoidTriangleData.ESpaceP = ESpaceP;
				EllipsoidTriangleData.ESpaceEntityDelta = ESpaceEntityDelta;
				EllipsoidTriangleData.t = 1.0f;
				r32 EntityDeltaLengthWorld = Length((EllipsoidToWorld * ESpaceEntityDelta));
				if(EntityDeltaLengthWorld > DistanceRemaining)
				{
					EllipsoidTriangleData.t = DistanceRemaining / EntityDeltaLengthWorld;
				}
				EllipsoidTriangleData.HitEntityType = EntityType_Null;

				NarrowPhaseCollisionDetection(SimRegion->World, &BroadPhaseResult, NarrowPhaseCollisionDetection_EllipsoidTriangle,
											  !IsSet(Entity, EntityFlag_InWater), 0, &EllipsoidTriangleData);
				
				for(u32 CollisionEntityIndex = 0;
					CollisionEntityIndex < CollideEntitiesCount;
					CollisionEntityIndex++)
				{
					sim_entity *TestEntity = CollideEntities[CollisionEntityIndex];
					if(CanCollide(GameState, Entity, TestEntity))
					{
						vec3 Displacement = TestEntity->Collision->OffsetP;
						mat3 TestEntityTransformation = Rotate3x3(TestEntity->Rotation, vec3(0.0f, 1.0f, 0.0f));
						for(u32 TriangleIndex = 0;
							TriangleIndex < (TestEntity->Collision->VerticesP.EntriesCount / 3);
							TriangleIndex++)
						{
							vec3 P1 = (TestEntityTransformation*TestEntity->Collision->VerticesP.Entries[TriangleIndex*3]) + TestEntity->P + Displacement;
							vec3 P2 = (TestEntityTransformation*TestEntity->Collision->VerticesP.Entries[TriangleIndex*3 + 1]) + TestEntity->P + Displacement;
							vec3 P3 = (TestEntityTransformation*TestEntity->Collision->VerticesP.Entries[TriangleIndex*3 + 2]) + TestEntity->P + Displacement;

							if(EllipsoidTriangleCollisionDetection(P1, P2, P3, &EllipsoidTriangleData))
							{
								HitEntity = TestEntity;
								EllipsoidTriangleData.HitEntityType = HitEntity->Type;
							}
						}
					}
				}

				DistanceRemaining -= EllipsoidTriangleData.t * EntityDeltaLengthWorld;
				//EllipsoidTriangleData.t -= 0.001f;
				ESpaceP += EllipsoidTriangleData.t*ESpaceEntityDelta;
				ESpaceEntityDelta = ESpaceDesiredP - ESpaceP;
				if(EllipsoidTriangleData.HitEntityType)
				{
					bool32 StopOnCollision = HandleCollision(GameState, SimRegion, Entity, HitEntity, Entity->Type, 
															 EllipsoidTriangleData.HitEntityType, EllipsoidToWorld, EllipsoidTriangleData.CollisionP);

					if(StopOnCollision)
					{
						vec3 SlidingPlaneNormal = Normalize((ESpaceP - EllipsoidTriangleData.CollisionP));

						if((SlidingPlaneNormal.y() > -0.1f) && (SlidingPlaneNormal.y() < 0.1f) &&
						    IsSet(Entity, EntityFlag_OnGround) && (Entity->dP.y() < 0.01f))
						{
							ray_triangle_collision_detection_data RayTriangleData;
							RayTriangleData.RayOrigin = EllipsoidToWorld * ESpaceP - CollisionVolumeDisplacement + vec3(0.0f, Entity->Collision->Dim.y(), 0.0f);
							vec3 SlidingPlaneNormalWorld = EllipsoidToWorld * SlidingPlaneNormal;
							vec3 MovementDir = -Normalize(vec3(SlidingPlaneNormalWorld.x(), 0.0f, SlidingPlaneNormalWorld.z()));
							RayTriangleData.RayEnd = RayTriangleData.RayOrigin + SimRegion->World->BlockDimInMeters*MovementDir;
							RayTriangleData.RayDirection = Normalize(RayTriangleData.RayEnd - RayTriangleData.RayOrigin);
							RayTriangleData.t = 1.0f;

							NarrowPhaseCollisionDetection(SimRegion->World, &BroadPhaseResult, NarrowPhaseCollisionDetection_RayTriangle,
														  false, &RayTriangleData);
							if(RayTriangleData.t == 1.0f)
							{
								MoveOnBlock = true;					
							}
						}

						ESpaceEntityDelta = ESpaceEntityDelta - 1.1f*Dot(SlidingPlaneNormal, ESpaceEntityDelta)*SlidingPlaneNormal;
						Entity->dP = Entity->dP - (1.1f*Dot(EllipsoidToWorld * SlidingPlaneNormal, Entity->dP)) *
														(EllipsoidToWorld * SlidingPlaneNormal);
					}
				}

				if(!Gravity && Entity->CanBeGravityAffected)
				{
					ray_triangle_collision_detection_data RayTriangleData = {};
					RayTriangleData.RayOrigin = EllipsoidToWorld * ESpaceP - CollisionVolumeDisplacement + vec3(0.0f, 0.001f, 0.0f);
					RayTriangleData.RayEnd = RayTriangleData.RayOrigin - vec3(0.0f, 0.1f, 0.0f);
					RayTriangleData.RayDirection = Normalize(RayTriangleData.RayEnd - RayTriangleData.RayOrigin);
					RayTriangleData.t = 1.0f;

					NarrowPhaseCollisionDetection(SimRegion->World, &BroadPhaseResult, NarrowPhaseCollisionDetection_RayTriangle,
												  true, &RayTriangleData);
											
					if(RayTriangleData.t < 1.0f)
					{
						AddFlags(Entity, EntityFlag_OnGround);
						if(!RayTriangleData.Water)
						{
							ClearFlags(Entity, EntityFlag_InWater);
						}
					}
					else
					{
						ClearFlags(Entity, EntityFlag_OnGround | EntityFlag_InWater);
						AddFlags(Entity, EntityFlag_GravityAffected);
					}
				}
			}
		}
	}

	vec3 dPNormalized = vec3(0.0f, 0.0f, 0.0f);
	if(MoveOnBlock)
	{
		dPNormalized = Normalize(Entity->dP);
		dPNormalized.SetY(0.0f);
	}
	Entity->P = EllipsoidToWorld * ESpaceP - CollisionVolumeDisplacement + 
				(r32)MoveOnBlock*(vec3(0.0f, 1.1f*SimRegion->World->BlockDimInMeters, 0.0f) + 0.3f*dPNormalized);

	if(Entity->DistanceLimit != 0.0f)
	{
		Entity->DistanceLimit = DistanceRemaining;
	}
}

internal void
RenderEntities(game_state *GameState, temp_state *TempState, sim_region *SimRegion, 
			   shader WorldShader, shader CharacterShader, shader HitpointsShader, 
			   vec3 Right, bool32 DEBUGRenderBB = false)
{
	for(u32 EntityIndex = 0;
		EntityIndex < SimRegion->EntityCount;
		EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;
		if(Entity->Updatable)
		{
			UseShader(WorldShader);

			switch(Entity->Type)
			{
				case EntityType_Hero:
				{
					UseShader(CharacterShader);

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
					
					SetMat4(CharacterShader, "BoneTransformations[0]", BoneTransformations[CharacterBone_Head]);
					SetMat4(CharacterShader, "BoneTransformations[1]", BoneTransformations[CharacterBone_Shoulders]);
					SetMat4(CharacterShader, "BoneTransformations[2]", BoneTransformations[CharacterBone_Body]);
					SetMat4(CharacterShader, "BoneTransformations[3]", BoneTransformations[CharacterBone_Hand_Right]);
					SetMat4(CharacterShader, "BoneTransformations[4]", BoneTransformations[CharacterBone_Hand_Left]);
					SetMat4(CharacterShader, "BoneTransformations[5]", BoneTransformations[CharacterBone_Foot_Right]);
					SetMat4(CharacterShader, "BoneTransformations[6]", BoneTransformations[CharacterBone_Foot_Left]);

					r32 HeroRotRadians = DEG2RAD(Entity->Rotation);
					vec3 FacingDir = vec3(Sin(HeroRotRadians), 0.0f, Cos(HeroRotRadians));
					vec3 HeroRight = Normalize(Cross(FacingDir, vec3(0.0f, 1.0f, 0.0f))); 
					asset_tag_vector MatchVector = { 1.0f };
					model_id HeadIndex = GetBestMatchModelFromType(TempState->GameAssets, AssetType_Head, &MatchVector);
					SetInt(CharacterShader, "BoneID", CharacterBone_Head);
					DrawModel(CharacterShader, TempState->GameAssets, HeadIndex, Entity->Rotation, 1.0f, HeroRight, Entity->P);
					SetInt(CharacterShader, "BoneID", CharacterBone_Shoulders);
					DrawModel(CharacterShader, TempState->GameAssets, GetFirstModelFromType(TempState->GameAssets, AssetType_Shoulders),
							Entity->Rotation, 1.0f, HeroRight, Entity->P);
					SetInt(CharacterShader, "BoneID", CharacterBone_Body);
					DrawModel(CharacterShader, TempState->GameAssets, GetFirstModelFromType(TempState->GameAssets, AssetType_Body),
							Entity->Rotation, 1.0f, HeroRight, Entity->P);
					SetInt(CharacterShader, "BoneID", CharacterBone_Hand_Right);
					DrawModel(CharacterShader, TempState->GameAssets, GetFirstModelFromType(TempState->GameAssets, AssetType_Hand),
							Entity->Rotation, 0.8f, HeroRight, Entity->P);
					SetInt(CharacterShader, "BoneID", CharacterBone_Hand_Left);
					DrawModel(CharacterShader, TempState->GameAssets, GetFirstModelFromType(TempState->GameAssets, AssetType_Hand),
							Entity->Rotation, 0.8f, -HeroRight, Entity->P);
					SetInt(CharacterShader, "BoneID", CharacterBone_Foot_Right);
					DrawModel(CharacterShader, TempState->GameAssets, GetFirstModelFromType(TempState->GameAssets, AssetType_Foot),
							Entity->Rotation, 1.4f, HeroRight, Entity->P);
					SetInt(CharacterShader, "BoneID", CharacterBone_Foot_Left);
					DrawModel(CharacterShader, TempState->GameAssets, GetFirstModelFromType(TempState->GameAssets, AssetType_Foot),
							Entity->Rotation, 1.4f, -HeroRight, Entity->P);

					if(DEBUGRenderBB)
					{
						DEBUGRenderAxes(Identity(1.0f));
						DEBUGRenderSphere(Entity->P + Entity->Collision->OffsetP, Entity->Collision->Dim, Entity->Rotation);
					}
				} break;

				case EntityType_Fireball:
				{
					mat4 Model = Translate(vec3(0.0f, 0.5f*0.25f, 0.0f) + Entity->P) * Scale(vec3(0.25f, 0.25f, 0.25f));
					SetMat4(WorldShader, "Model", Model);
					DrawFromVAO(GameState->CubeVAO, 36);

					if(DEBUGRenderBB)
					{
						DEBUGRenderCube(Entity->P + Entity->Collision->OffsetP, Entity->Collision->Dim, Entity->Rotation);
					}
				} break;

				case EntityType_Tree:
				{
					DrawModel(WorldShader, TempState->GameAssets, GetFirstModelFromType(TempState->GameAssets, AssetType_Tree),
								0.0f, 1.0f, vec3(0.0f, 0.0f, 0.0f), Entity->P);

					if(DEBUGRenderBB)
					{
						DEBUGRenderCube(Entity->P + Entity->Collision->OffsetP, Entity->Collision->Dim, Entity->Rotation);
					}
				} break;

				// TEST
				default:
				{
					mat4 Model = Translate(vec3(0.0f, 0.5f, 0.0f) + Entity->P);
					SetMat4(WorldShader, "Model", Model);
					DrawFromVAO(GameState->CubeVAO, 36);

					UseShader(HitpointsShader);
					SetVec3(HitpointsShader, "Color", vec3(1.0f, 0.0f, 0.0f));
					SetVec3(HitpointsShader, "CameraRight", Right);
					SetVec3(HitpointsShader, "BillboardSimCenterP", Entity->P + vec3(0.0f, 1.0f + 0.1f, 0.0f));
					SetVec2(HitpointsShader, "Scale", vec2((r32)Entity->HitPoints / (r32)Entity->MaxHitPoints, 0.2f));

					glBindVertexArray(GameState->QuadVAO);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					glBindVertexArray(0);

					if(DEBUGRenderBB)
					{
						DEBUGRenderCube(Entity->P + Entity->Collision->OffsetP, Entity->Collision->Dim, Entity->Rotation);
					}
				}  break;
			}
		}
	}
}

internal void
RenderParticleEffects(game_state *GameState, temp_state *TempState, sim_region *SimRegion, 
					  shader BillboardShader, vec3 Right)
{
	TIME_BLOCK;
	UseShader(BillboardShader);
	SetVec3(BillboardShader, "CameraRight", Right);
	SetVec3(BillboardShader, "CameraUp", vec3(0.0f, 1.0f, 0.0f));
	SetInt(BillboardShader, "Texture", 0);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);

	vec3 ParticlesPositions[MAX_PARTICLES_COUNT];
	vec2 ParticlesOffsets[MAX_PARTICLES_COUNT];
	r32 ParticlesScales[MAX_PARTICLES_COUNT];

	for(u32 EntityIndex = 0;
		EntityIndex < SimRegion->EntityCount;
		EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;
		if(Entity->Updatable)
		{
			if(Entity->Particles)
			{
				particle_emitter *EntityParticles = Entity->Particles;
				texture_id TextureIndex = GetFirstTextureFromType(TempState->GameAssets, EntityParticles->TextureType);
				loaded_texture *Texture = GetTexture(TempState->GameAssets, TextureIndex);
				if(Texture)
				{
					SetFloat(BillboardShader, "RowsInAtlas", (r32)EntityParticles->Info.RowsInTextureAtlas);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, Texture->TextureID);
					
					if(EntityParticles->Info.Additive)
					{
						glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					}
					else
					{
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					}

					glBindVertexArray(GameState->ParticleVAO);
					
					u32 ParticlesCount = 0;
					for(u32 ParticleIndex = 0;
						ParticleIndex < ArrayCount(EntityParticles->Particles);
						ParticleIndex++)
					{
						particle *Particle = EntityParticles->Particles + ParticleIndex;

						if(Particle->LifeTime > 0.0f)
						{
							r32 LifeFactor = (EntityParticles->Info.MaxLifeTime - Particle->LifeTime) / EntityParticles->Info.MaxLifeTime;
							u32 Rows = EntityParticles->Info.RowsInTextureAtlas;
							u32 StageCount = Rows*Rows;
							r32 ProgressInTextureAtlas = StageCount*LifeFactor;
							u32 IndexOfStage = (u32)ProgressInTextureAtlas;
							u32 Column = IndexOfStage % Rows;
							u32 Row = IndexOfStage / Rows;
							vec2 Offset = vec2((r32)Column / Rows, (r32)Row / Rows);

							ParticlesPositions[ParticlesCount] = Particle->P + Entity->P;
							ParticlesOffsets[ParticlesCount] = Offset;
							ParticlesScales[ParticlesCount] = Particle->Scale;
							ParticlesCount++;
						}
					}

					glBindBuffer(GL_ARRAY_BUFFER, GameState->ParticlePVBO);
					glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*ParticlesCount, ParticlesPositions, GL_STATIC_DRAW);
					glEnableVertexAttribArray(1);
					glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);

					glBindBuffer(GL_ARRAY_BUFFER, GameState->ParticleOffsetVBO);
					glBufferData(GL_ARRAY_BUFFER, sizeof(vec2)*ParticlesCount, ParticlesOffsets, GL_STATIC_DRAW);
					glEnableVertexAttribArray(2);
					glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void *)0);
					
					glBindBuffer(GL_ARRAY_BUFFER, GameState->ParticleScaleVBO);
					glBufferData(GL_ARRAY_BUFFER, sizeof(r32)*ParticlesCount, ParticlesScales, GL_STATIC_DRAW);
					glEnableVertexAttribArray(3);
					glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(r32), (void *)0);

					glVertexAttribDivisor(1, 1);
					glVertexAttribDivisor(2, 1);
					glVertexAttribDivisor(3, 1);

					glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);

					glBindVertexArray(0);
				}
				else
				{
					LoadTexture(TempState->GameAssets, TextureIndex);
				}
			}
		}
	}

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}