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
BeginSimulation(game_state *GameState, world *World, world_position Origin, rect3 Bounds, 
				stack_allocator *WorldAllocator, stack_allocator *TempAllocator, r32 dt)
{
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
	SimRegion->Entities = PushArray(TempAllocator, SimRegion->MaxEntityCount, sim_entity, 16);

	ZeroArray(SimRegion->Hash, ArrayCount(SimRegion->Hash));

	world_position MinChunkP = MapIntoChunkSpace(World, &Origin, Bounds.Min);
	MinChunkP.ChunkY = -1;
	world_position MaxChunkP = MapIntoChunkSpace(World, &Origin, Bounds.Max);
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
						bool32 DEBUGEmptyChunk = (ChunkY == 1);
						SetupChunk(World, Chunk, WorldAllocator, DEBUGEmptyChunk);
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
						Chunk->Translation = ChunkToRender->Translation;
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
		Entity->Fireball.SimPtr = 0;
		stored_entity *StoredEntity = GameState->StoredEntities + Entity->StorageIndex;
		StoredEntity->Sim = *Entity;

		world_position NewP = MapIntoChunkSpace(SimRegion->World, &SimRegion->Origin, Entity->P);
		ChangeEntityLocation(SimRegion->World, WorldAllocator, Entity->StorageIndex, StoredEntity, NewP);
	}
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

internal bool32
NarrowPhaseCollisionDetection(vec3 P1, vec3 P2, vec3 P3, mat3 WorldToEllipsoid,
							  vec3 NormalizedESpaceEntityDelta, vec3 ESpaceP, vec3 ESpaceEntityDelta,
							  r32 *t, vec3 *CollisionP)
{
	bool32 Result = false;

	bool32 FoundCollisionWithInsideTheTriangle = false;
	
	vec3 EP1 = WorldToEllipsoid * P1;
	vec3 EP2 = WorldToEllipsoid * P2;
	vec3 EP3 = WorldToEllipsoid * P3;

	plane TrianglePlane = PlaneFromTriangle(EP1, EP2, EP3);

	if(Dot(TrianglePlane.Normal, NormalizedESpaceEntityDelta) <= 0)
	{
		r32 t0, t1;
		bool32 EmbeddedInPlane = false;

		r32 SignedDistanceToTrianglePlane = SignedDistance(TrianglePlane, ESpaceP);
		r32 NormalDotEntityDelta = Dot(TrianglePlane.Normal, ESpaceEntityDelta);

		if(NormalDotEntityDelta == 0.0f)
		{
			if(fabs(SignedDistanceToTrianglePlane) >= 1.0f)
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

			if(t0 > t1)
			{
				r32 Temp = t0;
				t0 = t1;
				t1 = Temp;
			}

			if((t0 > 1.0f) || (t1 < 0.0f))
			{
				return(Result);
			}

			if(t0 < 0.0f)
			{
				t0 = 0.0f;
			}
			if(t1 > 1.0f)
			{
				t1 = 1.0f;
			}
		}

		if(!EmbeddedInPlane)
		{
			vec3 PlaneIntersectionPoint = ESpaceP + t0*ESpaceEntityDelta - TrianglePlane.Normal;

			if(IsPointInTriangle(PlaneIntersectionPoint, EP1, EP2, EP3))
			{
				FoundCollisionWithInsideTheTriangle = true;
				if(t0 < *t)
				{
					Result = true;
					*t = t0;
					*CollisionP = PlaneIntersectionPoint;
				}
			}
		}

		if(!FoundCollisionWithInsideTheTriangle)
		{
			r32 EntityDeltaSquaredLength = LengthSq(ESpaceEntityDelta);
			r32 A, B, C;
			r32 NewT = *t;

			// NOTE(georgy): Check triangle vertices
			A = EntityDeltaSquaredLength;

			B = 2.0f*(Dot(ESpaceEntityDelta, ESpaceP - EP1));
			C = LengthSq((EP1 - ESpaceP)) - 1.0f;
			if (GetLowestRoot(A, B, C, &NewT))
			{
				Result = true;
				*t = NewT;
				*CollisionP = EP1;
			}

			B = 2.0f*(Dot(ESpaceEntityDelta, ESpaceP - EP2));
			C = LengthSq((EP2 - ESpaceP)) - 1.0f;
			if (GetLowestRoot(A, B, C, &NewT))
			{
				Result = true;
				*t = NewT;
				*CollisionP = EP2;
			}

			B = 2.0f*(Dot(ESpaceEntityDelta, ESpaceP - EP3));
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

			A = EdgeSquaredLength*-EntityDeltaSquaredLength +
				EdgeDotdP*EdgeDotdP;
			B = EdgeSquaredLength*(2.0f * Dot(ESpaceEntityDelta, BaseToVertex)) -
				2.0f*EdgeDotdP*EdgeDotBaseToVertex;
			C = EdgeSquaredLength*(1.0f - LengthSq(BaseToVertex)) +
				EdgeDotBaseToVertex*EdgeDotBaseToVertex;
			if (GetLowestRoot(A, B, C, &NewT))
			{
				r32 f = (EdgeDotdP*NewT - EdgeDotBaseToVertex) / EdgeSquaredLength;
				if ((f >= 0.0f) && (f <= 1.0f))
				{
					Result = true;
					*t = NewT;
					*CollisionP = EP1 + f*Edge;
				}
			}

			Edge = EP3 - EP2;
			BaseToVertex = EP2 - ESpaceP;
			EdgeSquaredLength = LengthSq(Edge);
			EdgeDotdP = Dot(Edge, ESpaceEntityDelta);
			EdgeDotBaseToVertex = Dot(BaseToVertex, Edge);

			A = EdgeSquaredLength*-EntityDeltaSquaredLength +
				EdgeDotdP*EdgeDotdP;
			B = EdgeSquaredLength*(2.0f * Dot(ESpaceEntityDelta, BaseToVertex)) -
				2.0f*EdgeDotdP*EdgeDotBaseToVertex;
			C = EdgeSquaredLength*(1.0f - LengthSq(BaseToVertex)) +
				EdgeDotBaseToVertex*EdgeDotBaseToVertex;
			if (GetLowestRoot(A, B, C, &NewT))
			{
				r32 f = (EdgeDotdP*NewT - EdgeDotBaseToVertex) / EdgeSquaredLength;
				if ((f >= 0.0f) && (f <= 1.0f))
				{
					Result = true;
					*t = NewT;
					*CollisionP = EP2 + f*Edge;
				}
			}

			Edge = EP1 - EP3;
			BaseToVertex = EP3 - ESpaceP;
			EdgeSquaredLength = LengthSq(Edge);
			EdgeDotdP = Dot(Edge, ESpaceEntityDelta);
			EdgeDotBaseToVertex = Dot(BaseToVertex, Edge);

			A = EdgeSquaredLength*-EntityDeltaSquaredLength +
				EdgeDotdP*EdgeDotdP;
			B = EdgeSquaredLength*(2.0f * Dot(ESpaceEntityDelta, BaseToVertex)) -
				2.0f*EdgeDotdP*EdgeDotBaseToVertex;
			C = EdgeSquaredLength*(1.0f - LengthSq(BaseToVertex)) +
				EdgeDotBaseToVertex*EdgeDotBaseToVertex;
			if (GetLowestRoot(A, B, C, &NewT))
			{
				r32 f = (EdgeDotdP*NewT - EdgeDotBaseToVertex) / EdgeSquaredLength;
				if ((f >= 0.0f) && (f <= 1.0f))
				{
					Result = true;
					*t = NewT;
					*CollisionP = EP3 + f*Edge;
				}
			}
		}
	}

	return(Result);
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
HandleCollision(game_state *GameState, sim_entity *EntityA, sim_entity *EntityB, entity_type A, entity_type B)
{
	bool32 Result;

	if(EntityA->Type == EntityType_Fireball)
	{
		if(EntityB)
		{
			AddCollisionRule(GameState, EntityA->StorageIndex, EntityB->StorageIndex, false);
		}
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

	return(Result);
}

struct collided_chunk_info
{
	chunk *Chunk;
	vec3 Min, Max;
};
internal void
MoveEntity(game_state *GameState, sim_region *SimRegion, sim_entity *Entity, vec3 ddP, r32 Drag, r32 dt, bool32 Gravity)
{
	mat3 EllipsoidToWorld = Rotate3x3(Entity->Rotation, vec3(0.0f, 1.0f, 0.0f)) * 
							Scale3x3(0.5f*Entity->Collision->Dim);
	mat3 WorldToEllipsoid = Scale3x3(1.0f / (0.5f*Entity->Collision->Dim)) * 
							Rotate3x3(-Entity->Rotation, vec3(0.0f, 1.0f, 0.0f));

	if(Gravity)
	{
		ddP = vec3(0.0f, -9.8f, 0.0f);
	}
	else
	{
		r32 ddPLength = LengthSq(ddP);
		if(ddPLength > 1.0f)
		{
			ddP = Normalize(ddP);
		}
		ddP *= 7.0f;
		vec3 DragV = -Drag*Entity->dP;
		DragV.SetY(0.0f);
		ddP += DragV;
	}

	vec3 CollisionVolumeDisplacement = Entity->Collision->OffsetP;
	vec3 CollisionEntityP = Entity->P + CollisionVolumeDisplacement;
	vec3 OldP = CollisionEntityP;
	vec3 DesiredP = (0.5f*ddP*dt*dt) + (Entity->dP*dt) + CollisionEntityP;

	vec3 EntityDelta = DesiredP - CollisionEntityP;
	Entity->dP += ddP*dt;

	// NOTE(georgy): Collision broad phase
	stored_entity *StoredEntity = GameState->StoredEntities + Entity->StorageIndex;
	world_position OldWorldP = StoredEntity->P;

	u32 CollideChunksCount = 0;
	collided_chunk_info CollideChunks[27];

	u32 CollideEntitiesCount = 0;
	sim_entity *CollideEntities[10];

	{
		// TODO(georgy): 
		//	I guess, we can have a situation, where EntityOldPAABB is totally in Chunk1 and EntityDesiredPAABB is totally on Chunk2
		//	so we can miss all other chunks that could intersect with us when we were moving from Chunk1 to Chunk2
		box EntityBox = ConstructBoxDim(Entity->Collision->Dim);
		mat3 EntityRotationMatrix = Rotate3x3(Entity->Rotation, vec3(0.0f, 1.0f, 0.0f)); 

		box EntityBoxOldP = ConstructBox(&EntityBox, EntityRotationMatrix, OldP);
		rect3 EntityOldPAABB = RectFromBox(&EntityBoxOldP);

		box EntityBoxDesiredP = ConstructBox(&EntityBox, EntityRotationMatrix, DesiredP);
		rect3 EntityDesiredPAABB = RectFromBox(&EntityBoxDesiredP);

		for(i32 ChunkZ = OldWorldP.ChunkZ - 1;
			ChunkZ <= OldWorldP.ChunkZ + 1;
			ChunkZ++)
		{
			for(i32 ChunkY = OldWorldP.ChunkY - 1;
				ChunkY <= OldWorldP.ChunkY + 1;
				ChunkY++)
			{
				for(i32 ChunkX = OldWorldP.ChunkX - 1;
					ChunkX <= OldWorldP.ChunkX + 1;
					ChunkX++)
				{
					chunk *Chunk = GetChunk(SimRegion->World, ChunkX, ChunkY, ChunkZ);
					if(Chunk)
					{
						r32 ChunkDimInMeters = SimRegion->World->ChunkDimInMeters;
						vec3 SimSpaceChunkAABBMin = Chunk->Translation;
						vec3 SimSpaceChunkAABBMax = SimSpaceChunkAABBMin + vec3(ChunkDimInMeters, ChunkDimInMeters, ChunkDimInMeters);
						rect3 SimSpaceChunkAABB = RectMinMax(SimSpaceChunkAABBMin, SimSpaceChunkAABBMax);

						if(RectIntersect(SimSpaceChunkAABB, EntityOldPAABB) || RectIntersect(SimSpaceChunkAABB, EntityDesiredPAABB))
						{
							vec3 HeroMinOffsetOld = EntityOldPAABB.Min - SimSpaceChunkAABB.Min;
							vec3 HeroMaxOffsetOld = EntityOldPAABB.Max - SimSpaceChunkAABB.Min;
							vec3 HeroMinOffsetDesired = EntityDesiredPAABB.Min - SimSpaceChunkAABB.Min;
							vec3 HeroMaxOffsetDesired = EntityDesiredPAABB.Max - SimSpaceChunkAABB.Min;
							CollideChunks[CollideChunksCount].Min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
							CollideChunks[CollideChunksCount].Max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

							CollideChunks[CollideChunksCount].Min = Min(CollideChunks[CollideChunksCount].Min, HeroMinOffsetOld);
							CollideChunks[CollideChunksCount].Min = Min(CollideChunks[CollideChunksCount].Min, HeroMinOffsetDesired);
							CollideChunks[CollideChunksCount].Max = Max(CollideChunks[CollideChunksCount].Max, HeroMaxOffsetOld);
							CollideChunks[CollideChunksCount].Max = Max(CollideChunks[CollideChunksCount].Max, HeroMaxOffsetDesired);

							CollideChunks[CollideChunksCount++].Chunk = Chunk;
						}
					}
				}	
			}	
		}

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
	}

	// NOTE(georgy): Collision narrow phase
	vec3 ESpaceEntityDelta = WorldToEllipsoid * EntityDelta;
	vec3 ESpaceP = WorldToEllipsoid * CollisionEntityP;

	r32 DistanceRemaining = Entity->DistanceLimit;
	if(DistanceRemaining == 0.0f)
	{
		DistanceRemaining = 10000.0f;
	}

	for(u32 Iteration = 0;
		Iteration < 4;
		Iteration++)
	{
		r32 EntityDeltaLength = Length(ESpaceEntityDelta);
		if(EntityDeltaLength > 0.0f)
		{
			vec3 ESpaceDesiredP = ESpaceP + ESpaceEntityDelta;
			vec3 NormalizedESpaceEntityDelta = Normalize(ESpaceEntityDelta);
			r32 t = 1.0f;
			r32 EntityDeltaLengthWorld = Length((EllipsoidToWorld * ESpaceEntityDelta));
			if(EntityDeltaLength > DistanceRemaining)
			{
				t = DistanceRemaining / EntityDeltaLengthWorld;
			}
			vec3 CollisionP;
			sim_entity *HitEntity = 0;
			entity_type HitEntityType = EntityType_Null;
			for(u32 ChunkIndex = 0;
				ChunkIndex < CollideChunksCount;
				ChunkIndex++)
			{
				for(u32 TriangleIndex = 0;
					TriangleIndex < (CollideChunks[ChunkIndex].Chunk->VerticesP.EntriesCount / 3);
					TriangleIndex++)
				{
					chunk *Chunk = CollideChunks[ChunkIndex].Chunk;
					vec3 Min = CollideChunks[ChunkIndex].Min;
					vec3 Max = CollideChunks[ChunkIndex].Max;
					vec3 P1 = Chunk->VerticesP.Entries[TriangleIndex*3];
					vec3 P2 = Chunk->VerticesP.Entries[TriangleIndex*3 + 1];
					vec3 P3 = Chunk->VerticesP.Entries[TriangleIndex*3 + 2];

					if(!((P1.x() > Max.x() + 0.5f) || (P1.x() < Min.x() - 0.5f)) || 
					   !((P2.x() > Max.x() + 0.5f) || (P2.x() < Min.x() - 0.5f)) || 
					   !((P3.x() > Max.x() + 0.5f) || (P3.x() < Min.x() - 0.5f)))
					{
						if(!((P1.y() > Max.y() + 0.5f) || (P1.y() < Min.y() - 0.5f)) || 
					   	   !((P2.y() > Max.y() + 0.5f) || (P2.y() < Min.y() - 0.5f)) || 
					   	   !((P3.y() > Max.y() + 0.5f) || (P3.y() < Min.y() - 0.5f)))
						{
							if(!((P1.z() > Max.z() + 0.5f) || (P1.z() < Min.z() - 0.5f)) || 
					   	   	   !((P2.z() > Max.z() + 0.5f) || (P2.z() < Min.z() - 0.5f)) || 
					   	  	   !((P3.z() > Max.z() + 0.5f) || (P3.z() < Min.z() - 0.5f)))
							{
								P1 += Chunk->Translation;
								P2 += Chunk->Translation;
								P3 += Chunk->Translation;
								if(NarrowPhaseCollisionDetection(P1, P2, P3, WorldToEllipsoid, NormalizedESpaceEntityDelta,
																ESpaceP, ESpaceEntityDelta, &t, &CollisionP))
								{
									HitEntityType = EntityType_Chunk;
								}
							}
						}
					}
				}
			}
			
			for(u32 CollisionEntityIndex = 0;
				CollisionEntityIndex < CollideEntitiesCount;
				CollisionEntityIndex++)
			{
				sim_entity *TestEntity = CollideEntities[CollisionEntityIndex];
				if(CanCollide(GameState, Entity, TestEntity))
				{
					for(u32 TriangleIndex = 0;
						TriangleIndex < (TestEntity->Collision->VerticesP.EntriesCount / 3);
						TriangleIndex++)
					{
						mat3 TestEntityTransformation = Rotate3x3(TestEntity->Rotation, vec3(0.0f, 1.0f, 0.0f));
						vec3 Displacement = TestEntity->Collision->OffsetP;
						vec3 P1 = (TestEntityTransformation*TestEntity->Collision->VerticesP.Entries[TriangleIndex*3]) + TestEntity->P + Displacement;
						vec3 P2 = (TestEntityTransformation*TestEntity->Collision->VerticesP.Entries[TriangleIndex*3 + 1]) + TestEntity->P + Displacement;
						vec3 P3 = (TestEntityTransformation*TestEntity->Collision->VerticesP.Entries[TriangleIndex*3 + 2]) + TestEntity->P + Displacement;

						if(NarrowPhaseCollisionDetection(P1, P2, P3, WorldToEllipsoid, NormalizedESpaceEntityDelta,
														ESpaceP, ESpaceEntityDelta, &t, &CollisionP))
						{
							HitEntity = TestEntity;
							HitEntityType = HitEntity->Type;
						}
					}
				}
			}

			ESpaceP += t*ESpaceEntityDelta;
			DistanceRemaining -= t*EntityDeltaLengthWorld;
			ESpaceEntityDelta = ESpaceDesiredP - ESpaceP;
			bool32 SlidingPlaneNormalIsGround = false;
			if(HitEntityType)
			{
				bool32 StopOnCollision = HandleCollision(GameState, Entity, HitEntity, Entity->Type, HitEntityType);

				if(StopOnCollision)
				{
					vec3 SlidingPlaneNormal = Normalize((ESpaceP - CollisionP));
					if(Gravity && (SlidingPlaneNormal.y() >= 0.95f))
					{
						SlidingPlaneNormalIsGround = true;
						AddFlags(Entity, EntityFlag_OnGround);
					}
					ESpaceEntityDelta = ESpaceEntityDelta - 1.1f*Dot(SlidingPlaneNormal, ESpaceEntityDelta)*SlidingPlaneNormal;
					Entity->dP = Entity->dP - (1.1f*Dot(EllipsoidToWorld * SlidingPlaneNormal, Entity->dP)) *
													  (EllipsoidToWorld * SlidingPlaneNormal);
				}
			}
			
			if((Gravity && Iteration == 0) && (!HitEntityType || !SlidingPlaneNormalIsGround))
			{
				ClearFlags(Entity, EntityFlag_OnGround);
			}
		}
	}

	Entity->P = EllipsoidToWorld * ESpaceP - CollisionVolumeDisplacement;

	if(Entity->DistanceLimit != 0.0f)
	{
		Entity->DistanceLimit = DistanceRemaining;
	}
}