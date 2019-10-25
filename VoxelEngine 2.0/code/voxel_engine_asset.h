#pragma once

struct loaded_model
{
	GLuint VAO, PVBO, NormalsVBO, ColorsVBO;
	u32 VerticesCount;

	vec3 Alignment;
	r32 AlignmentX; // NOTE(georgy): This is for models like hands, where we need to multiply this by EntityRight vec
};

internal loaded_model
LoadCub(char *Filename, u64 *ModelSize, r32 AdditionalAlignmentY = 0.0f, r32 AdditionalAlignmentX = 0.0f)
{
	loaded_model Model = {};

	u32 Width, Height, Depth;
	read_entire_file_result FileData = PlatformReadEntireFile(Filename);
	if(FileData.Memory)
	{
		dynamic_array_vec3 VerticesP, Normals, VertexColors;
		InitializeDynamicArray(&VerticesP);
		InitializeDynamicArray(&Normals);
		InitializeDynamicArray(&VertexColors);

		Width = *((u32 *)FileData.Memory);
		Depth = *((u32 *)FileData.Memory + 1);
		Height = *((u32 *)FileData.Memory + 2);

		bool8 *VoxelsStates = (bool8 *)PlatformAllocateMemory(sizeof(bool8)*Width*Height*Depth);
		u8 *Voxels = (u8 *)((u32 *)FileData.Memory + 3);
		u8 *Colors = Voxels;
		for(u32 VoxelY = 0;
			VoxelY < Height;
			VoxelY++)
		{
			for(u32 VoxelZ = 0;
				VoxelZ < Depth;
				VoxelZ++)
			{
				for(u32 VoxelX = 0;
					VoxelX < Width;
					VoxelX++)
				{
					u8 R, G, B;
					R = *Voxels++;
					G = *Voxels++;
					B = *Voxels++;					
					if(R != 0 || G != 0 || B != 0)
					{
						VoxelsStates[VoxelY*Width*Depth + VoxelZ*Width + VoxelX] = true;
					}
				}
			}
		}

		r32 BlockDimInMeters = 0.03f;
		Model.Alignment = vec3(0.0f, 0.5f*Height*BlockDimInMeters + AdditionalAlignmentY, 0.0f);
		Model.AlignmentX = AdditionalAlignmentX;
		for(u32 VoxelY = 0;
			VoxelY < Height;
			VoxelY++)
		{
			for(u32 VoxelZ = 0;
				VoxelZ < Depth;
				VoxelZ++)
			{
				for(u32 VoxelX = 0;
					VoxelX < Width;
					VoxelX++)
				{
					if(VoxelsStates[VoxelY*Width*Depth + VoxelZ*Width + VoxelX])
					{
						r32 X = VoxelX*BlockDimInMeters - 0.5f*Width*BlockDimInMeters;
						r32 Y = VoxelY*BlockDimInMeters - 0.5f*Height*BlockDimInMeters;
						r32 Z = VoxelZ*BlockDimInMeters - 0.5f*Depth*BlockDimInMeters;

						vec3 A, B, C, D;
						u8 Red = Colors[(VoxelY*Width*Depth + VoxelZ*Width + VoxelX) * 3];
						u8 Green = Colors[(VoxelY*Width*Depth + VoxelZ*Width + VoxelX) * 3 + 1];
						u8 Blue = Colors[(VoxelY*Width*Depth + VoxelZ*Width + VoxelX) * 3 + 2];
						vec3 Color = vec3(Red / 255.0f, Green / 255.0f, Blue / 255.0f);

						if ((VoxelX == 0) || !VoxelsStates[VoxelY*Width*Depth + VoxelZ*Width + (VoxelX-1)])
						{
							A = vec3(X, Y, Z);
							B = vec3(X, Y, Z + BlockDimInMeters);
							C = vec3(X, Y + BlockDimInMeters, Z);
							D = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
							vec3 N = vec3(-1.0f, 0.0f, 0.0f);
							AddQuad(&VerticesP, A, B, C, D);
							AddQuad(&Normals, N, N, N, N);
							AddQuad(&VertexColors, Color, Color, Color, Color);
						}

						if ((VoxelX == Width - 1) || !VoxelsStates[VoxelY*Width*Depth + VoxelZ*Width + (VoxelX+1)])
						{
							A = vec3(X + BlockDimInMeters, Y, Z);
							B = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
							C = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
							vec3 N = vec3(1.0f, 0.0f, 0.0f);
							AddQuad(&VerticesP, A, B, C, D);
							AddQuad(&Normals, N, N, N, N);
							AddQuad(&VertexColors, Color, Color, Color, Color);
						}

						if ((VoxelZ == 0) || !VoxelsStates[VoxelY*Width*Depth + (VoxelZ-1)*Width + VoxelX])
						{
							A = vec3(X, Y, Z);
							B = vec3(X, Y + BlockDimInMeters, Z);
							C = vec3(X + BlockDimInMeters, Y, Z);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
							vec3 N = vec3(0.0f, 0.0f, -1.0f);
							AddQuad(&VerticesP, A, B, C, D);
							AddQuad(&Normals, N, N, N, N);
							AddQuad(&VertexColors, Color, Color, Color, Color);
						}

						if ((VoxelZ == Depth - 1) || !VoxelsStates[VoxelY*Width*Depth + (VoxelZ+1)*Width + VoxelX])
						{
							A = vec3(X, Y, Z + BlockDimInMeters);
							B = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							C = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
							vec3 N = vec3(0.0f, 0.0f, 1.0f);
							AddQuad(&VerticesP, A, B, C, D);
							AddQuad(&Normals, N, N, N, N);
							AddQuad(&VertexColors, Color, Color, Color, Color);
						}

						if ((VoxelY == 0) || !VoxelsStates[(VoxelY-1)*Width*Depth + VoxelZ*Width + VoxelX])
						{
							A = vec3(X, Y, Z);
							B = vec3(X + BlockDimInMeters, Y, Z);
							C = vec3(X, Y, Z + BlockDimInMeters);
							D = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							vec3 N = vec3(0.0f, -1.0f, 0.0f);
							AddQuad(&VerticesP, A, B, C, D);
							AddQuad(&Normals, N, N, N, N);
							AddQuad(&VertexColors, Color, Color, Color, Color);
						}

						if ((VoxelY == Height - 1) || !VoxelsStates[(VoxelY+1)*Width*Depth + VoxelZ*Width + VoxelX])
						{
							A = vec3(X, Y + BlockDimInMeters, Z);
							B = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
							C = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
							vec3 N = vec3(0.0f, 1.0f, 0.0f);
							AddQuad(&VerticesP, A, B, C, D);
							AddQuad(&Normals, N, N, N, N);
							AddQuad(&VertexColors, Color, Color, Color, Color);
						}
					}
				}	
			}
		}

		glGenVertexArrays(1, &Model.VAO);
		glGenBuffers(1, &Model.PVBO);
		glGenBuffers(1, &Model.NormalsVBO);
		glGenBuffers(1, &Model.ColorsVBO);
		glBindVertexArray(Model.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, Model.PVBO);
		glBufferData(GL_ARRAY_BUFFER, VerticesP.EntriesCount*sizeof(vec3), VerticesP.Entries, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, Model.NormalsVBO);
		glBufferData(GL_ARRAY_BUFFER, Normals.EntriesCount*sizeof(vec3), Normals.Entries, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
		glBindBuffer(GL_ARRAY_BUFFER, Model.ColorsVBO);
		glBufferData(GL_ARRAY_BUFFER, VertexColors.EntriesCount*sizeof(vec3), VertexColors.Entries, GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
		glBindVertexArray(0);

		Model.VerticesCount = VerticesP.EntriesCount;
		*ModelSize = VerticesP.EntriesCount*sizeof(vec3) + Normals.EntriesCount*sizeof(vec3) + VertexColors.EntriesCount*sizeof(vec3);

		FreeDynamicArray(&VerticesP);
		FreeDynamicArray(&Normals);
		FreeDynamicArray(&VertexColors);
		PlatformFreeMemory(VoxelsStates);
		PlatformFreeFileMemory(FileData.Memory);
	}

	return (Model);
}

enum asset_type_id
{
	AssetType_Null,
	AssetType_Head,
	AssetType_Shoulders,
	AssetType_Body,
	AssetType_Hand,
	AssetType_Foot,

	AssetType_Count
};

struct asset_memory_header
{
	asset_memory_header *Next;
	asset_memory_header *Prev;

	u32 AssetIndex;
	u32 TotalSize;
	loaded_model Model;
};

struct asset
{
	char *Filename;
	r32 AdditionalAlignmentY;
	r32 AdditionalAlignmentX;
	
	u32 FirstTagIndex;
	u32 OnePastLastTagIndex;
	asset_memory_header *Header;
};

struct asset_type 
{
	u32 FirstAssetIndex;
	u32 OnePastLastAssetIndex;
};

enum asset_tag_id
{
	Tag_Color,

	Tag_Count
};

struct asset_tag
{
	asset_tag_id ID;
	r32 Value;
};

struct asset_tag_vector
{
	r32 E[Tag_Count];
};

struct game_assets
{
	asset_type AssetTypes[AssetType_Count];

	u32 AssetCount;
	asset Assets[256];

	u32 TagCount;
	asset_tag Tags[256];

	asset_memory_header MemoryHeaderSentinel;

	u64 MemorySizeRestriction;
	u64 UsedMemory;

	asset_type *DEBUGAssetType; 
	asset *DEBUGAsset;
};

inline void
InsertAssetHeaderFront(game_assets *GameAssets, asset_memory_header *Header)
{
	Header->Next = GameAssets->MemoryHeaderSentinel.Next;
	Header->Prev = &GameAssets->MemoryHeaderSentinel;

	Header->Next->Prev = Header;
	Header->Prev->Next = Header;
}

inline void
RemoveAssetHeaderFromList(asset_memory_header *Header)
{
	Header->Next->Prev = Header->Prev;
	Header->Prev->Next = Header->Next;
}

inline loaded_model *
GetModel(game_assets *GameAssets, u32 AssetIndex)
{
	Assert(AssetIndex < GameAssets->AssetCount);

	loaded_model *Result = 0;
	asset *Asset = GameAssets->Assets + AssetIndex;
	if(Asset->Header)
	{
		RemoveAssetHeaderFromList(Asset->Header);
		InsertAssetHeaderFront(GameAssets, Asset->Header);

		Result = &Asset->Header->Model;
	}

	return(Result);
}

internal void
LoadModel(game_assets *GameAssets, u32 AssetIndex)
{
	asset *Asset = GameAssets->Assets + AssetIndex;
	Assert(!Asset->Header);

	Asset->Header = (asset_memory_header *)PlatformAllocateMemory(sizeof(asset_memory_header));
	u64 ModelSize = 0;
	Asset->Header->AssetIndex = AssetIndex;
	Asset->Header->Model = LoadCub(Asset->Filename, &ModelSize, Asset->AdditionalAlignmentY, Asset->AdditionalAlignmentX);
	Asset->Header->TotalSize = sizeof(asset_memory_header) + ModelSize;

	InsertAssetHeaderFront(GameAssets, Asset->Header);
	
	GameAssets->UsedMemory += Asset->Header->TotalSize;
}

internal u32
GetBestMatchAsset(game_assets *GameAssets, asset_type_id TypeID, asset_tag_vector *MatchVector)
{
	u32 Result = 0;

	r32 MinDiff = FLT_MAX;
	asset_type *Type = GameAssets->AssetTypes + TypeID;
	for(u32 AssetIndex = Type->FirstAssetIndex;
		AssetIndex < Type->OnePastLastAssetIndex;
		AssetIndex++)
	{
		asset *Asset = GameAssets->Assets + AssetIndex;

		r32 Diff = 0.0f;
		for(u32 TagIndex = Asset->FirstTagIndex;
			TagIndex < Asset->OnePastLastTagIndex;
			TagIndex++)
		{
			asset_tag *Tag = GameAssets->Tags + TagIndex;

			r32 A = MatchVector->E[Tag->ID];
			r32 B = Tag->Value;

			Diff += fabs(A - B);
		}

		if(Diff < MinDiff)
		{
			MinDiff = Diff;
			Result = AssetIndex;
		}
	}

	return(Result);
}

internal void
BeginAssetType(game_assets *Assets, asset_type_id ID)
{
	Assert(!Assets->DEBUGAssetType);

	Assets->AssetTypes[ID].FirstAssetIndex = Assets->AssetCount;
	Assets->AssetTypes[ID].OnePastLastAssetIndex = Assets->AssetCount;

	Assets->DEBUGAssetType = &Assets->AssetTypes[ID];
}

internal void
AddAsset(game_assets *Assets, stack_allocator *Allocator, 
		 char *Filename, r32 AdditionalAlignmentY = 0.0f, r32 AdditionalAlignmentX = 0.0f)
{
	Assert(Assets->DEBUGAssetType);

	asset *Asset = Assets->Assets + Assets->AssetCount++;
	Asset->Filename = Filename;
	Asset->AdditionalAlignmentY = AdditionalAlignmentY;
	Asset->AdditionalAlignmentX = AdditionalAlignmentX;
	Asset->FirstTagIndex = Assets->TagCount;
	Asset->OnePastLastTagIndex = Assets->TagCount;
	Asset->Header = 0;

	Assets->DEBUGAsset = Asset;
}

internal void
AddTag(game_assets *Assets, asset_tag_id ID, r32 Value)
{
	Assert(Assets->DEBUGAsset);

	asset_tag *Tag = Assets->Tags + Assets->TagCount++;
	
	Tag->ID = ID;
	Tag->Value = Value;

	Assets->DEBUGAsset->OnePastLastTagIndex = Assets->TagCount;
}

internal void
EndAssetType(game_assets *Assets)
{
	Assert(Assets->DEBUGAssetType);
	Assert(Assets->DEBUGAsset);

	Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->AssetCount;
	Assets->DEBUGAssetType = 0;
	Assets->DEBUGAsset = 0;
}

internal game_assets *
AllocateGameAssets(stack_allocator *Allocator, u64 Size)
{
	game_assets *GameAssets = PushStruct(Allocator, game_assets);
	GameAssets->AssetCount = 0;
	GameAssets->TagCount = 0;
	
	GameAssets->MemorySizeRestriction = Size;
	GameAssets->UsedMemory = 0;

	GameAssets->MemoryHeaderSentinel.Next = &GameAssets->MemoryHeaderSentinel;
	GameAssets->MemoryHeaderSentinel.Prev = &GameAssets->MemoryHeaderSentinel;

	GameAssets->DEBUGAssetType = 0;
	GameAssets->DEBUGAsset = 0;

	BeginAssetType(GameAssets, AssetType_Head);
	AddAsset(GameAssets, Allocator, "data/models/head1.cub", 0.47f);
	AddTag(GameAssets, Tag_Color, 1.0f);
	AddAsset(GameAssets, Allocator, "data/models/head2.cub", 0.47f);
	AddTag(GameAssets, Tag_Color, 10.0f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Shoulders);
	AddAsset(GameAssets, Allocator, "data/models/shoulders.cub", 0.4f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Body);
	AddAsset(GameAssets, Allocator, "data/models/body.cub", 0.08f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Hand);
	AddAsset(GameAssets, Allocator, "data/models/hand.cub", 0.32f, 0.275f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Foot);
	AddAsset(GameAssets, Allocator, "data/models/foot.cub", 0.0f, 0.15f);
	EndAssetType(GameAssets);

	return(GameAssets);
}

inline void
UnloadAssetsIfNecessary(game_assets *GameAssets)
{
	while(GameAssets->MemorySizeRestriction < GameAssets->UsedMemory)
	{
		asset_memory_header *LastUsedAsset = GameAssets->MemoryHeaderSentinel.Prev;
		RemoveAssetHeaderFromList(LastUsedAsset);

		glDeleteBuffers(1, &LastUsedAsset->Model.PVBO);
		glDeleteBuffers(1, &LastUsedAsset->Model.NormalsVBO);
		glDeleteBuffers(1, &LastUsedAsset->Model.ColorsVBO);
		glDeleteVertexArrays(1, &LastUsedAsset->Model.VAO);

		GameAssets->Assets[LastUsedAsset->AssetIndex].Header = 0;

		GameAssets->UsedMemory -= LastUsedAsset->TotalSize;
		PlatformFreeMemory(LastUsedAsset);
	}
}