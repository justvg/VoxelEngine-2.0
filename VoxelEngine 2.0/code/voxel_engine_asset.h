#pragma once

struct loaded_model
{
	GLuint VAO, PVBO, NormalsVBO, ColorsVBO;
	u32 VerticesCount;

	vec3 Alignment;
	r32 AlignmentX; // NOTE(georgy): This is for models like hands, where we need to multiply this by EntityRight vec

	dynamic_array_vec3 VerticesP;
	dynamic_array_vec3 Normals;
	dynamic_array_vec3 VertexColors;
};

enum asset_type_id
{
	AssetType_Null,
	AssetType_Head,
	AssetType_Shoulders,
	AssetType_Body,
	AssetType_Hand,
	AssetType_Foot,

	AssetType_Tree,

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

enum asset_state
{
	AssetState_Unloaded,
	AssetState_Queued,
	AssetState_Loaded
};

struct asset
{
	asset_state State;

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

	u32 Lock;

	struct temp_state *TempState;

	asset_type *DEBUGAssetType; 
	asset *DEBUGAsset;
};

inline void
BeginAssetLock(game_assets *Assets)
{
	for(;;)
	{
		if(AtomicCompareExchangeU32(&Assets->Lock, 1, 0) == 0)
		{
			break;
		}
	}
}

inline void
EndAssetLock(game_assets *Assets)
{
	CompletePreviousWritesBeforeFutureWrites;
	Assets->Lock = 0;
}

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
	if(Asset->State == AssetState_Loaded)
	{
		BeginAssetLock(GameAssets);
		RemoveAssetHeaderFromList(Asset->Header);
		InsertAssetHeaderFront(GameAssets, Asset->Header);
		EndAssetLock(GameAssets);

		Result = &Asset->Header->Model;

		if(!Result->VAO)
		{
			glGenVertexArrays(1, &Result->VAO);
			glGenBuffers(1, &Result->PVBO);
			glGenBuffers(1, &Result->NormalsVBO);
			glGenBuffers(1, &Result->ColorsVBO);
			glBindVertexArray(Result->VAO);
			glBindBuffer(GL_ARRAY_BUFFER, Result->PVBO);
			glBufferData(GL_ARRAY_BUFFER, Result->VerticesP.EntriesCount*sizeof(vec3), Result->VerticesP.Entries, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
			glBindBuffer(GL_ARRAY_BUFFER, Result->NormalsVBO);
			glBufferData(GL_ARRAY_BUFFER, Result->Normals.EntriesCount*sizeof(vec3), Result->Normals.Entries, GL_STATIC_DRAW);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
			glBindBuffer(GL_ARRAY_BUFFER, Result->ColorsVBO);
			glBufferData(GL_ARRAY_BUFFER, Result->VertexColors.EntriesCount*sizeof(vec3), Result->VertexColors.Entries, GL_STATIC_DRAW);
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
			glBindVertexArray(0);

			FreeDynamicArray(&Result->VerticesP);
			FreeDynamicArray(&Result->Normals);
			FreeDynamicArray(&Result->VertexColors);
		}
	}

	return(Result);
}