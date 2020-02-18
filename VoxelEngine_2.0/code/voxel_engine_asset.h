#pragma once
#include "voxel_engine_file_formats.h"

struct loaded_model
{
	GLuint VAO, PVBO, NormalsVBO, ColorsVBO;

	u32 VerticesCount;
	vec3 *Positions;
	vec3 *Normals;
	vec3 *Colors;

	vec3 Alignment;
	r32 AlignmentX; // NOTE(georgy): This is for models like hands, where we need to multiply this by EntityRight vec
};

struct loaded_texture
{
	GLuint TextureID;

	i32 Width, Height, ChannelsCount;

	// NOTE(georgy): For glyphs (to count descent)
	r32 AlignPercentageY;

	union
	{
		u8 *Data;
		void *Free;
	};
};

struct loaded_sound
{
	i16 *Samples[2];
	u32 SampleCount;
	u32 ChannelsCount;

	void *Free;
};

struct loaded_font
{
	loaded_texture *Glyphs;
	r32 *HorizontalAdvances;
	
	u32 LineAdvance;
	u32 AscenderHeight;

	u32 GlyphsCount;
	u32 FirstCodepoint;
	u32 LastCodepoint;
};

struct model_id
{
	u32 Value;
};

struct texture_id
{
	u32 Value;
};

struct font_id
{
	u32 Value;
};

struct sound_id
{
	u32 Value;
};

struct asset_memory_header
{
	asset_memory_header *Next;
	asset_memory_header *Prev;

	u32 AssetIndex;
	u32 TotalSize;
	union
	{
		loaded_model Model;
		loaded_texture Texture;
		loaded_sound Sound;
		loaded_font Font;
	};
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
	asset_data_type DataType;
	asset_memory_header *Header;

	vea_asset VEAAsset;
};

struct asset_tag_vector
{
	r32 E[Tag_Count];
};

struct game_assets
{
	u32 TagCount;
	vea_asset_tag *Tags;

	u32 AssetCount;
	asset *Assets;

	vea_asset_type AssetTypes[AssetType_Count];

	asset_memory_header MemoryHeaderSentinel;

	u64 MemorySizeRestriction;
	u64 UsedMemory;

	platform_file_handle File;

	u32 Lock;

	struct temp_state *TempState;
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

inline asset_memory_header *
GetAsset(game_assets *GameAssets, u32 AssetIndex)
{
	Assert(AssetIndex < GameAssets->AssetCount);
	asset *Asset = GameAssets->Assets + AssetIndex;

	asset_memory_header *Result = 0;
	
	BeginAssetLock(GameAssets);
	if(Asset->State == AssetState_Loaded)
	{
		Result = Asset->Header;

		RemoveAssetHeaderFromList(Result);
		InsertAssetHeaderFront(GameAssets, Result);
	}
	EndAssetLock(GameAssets);

	return(Result);
}

inline loaded_model *
GetModel(game_assets *GameAssets, model_id Index)
{
	asset_memory_header *Header = GetAsset(GameAssets, Index.Value);

	loaded_model *Result = 0;
	if(Header)
	{
		Result = &Header->Model;

		if(!Result->VAO)
		{
			glGenVertexArrays(1, &Result->VAO);
			glGenBuffers(1, &Result->PVBO);
			glGenBuffers(1, &Result->NormalsVBO);
			glGenBuffers(1, &Result->ColorsVBO);
			glBindVertexArray(Result->VAO);
			glBindBuffer(GL_ARRAY_BUFFER, Result->PVBO);
			glBufferData(GL_ARRAY_BUFFER, Result->VerticesCount*sizeof(vec3), Result->Positions, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
			glBindBuffer(GL_ARRAY_BUFFER, Result->NormalsVBO);
			glBufferData(GL_ARRAY_BUFFER, Result->VerticesCount*sizeof(vec3), Result->Normals, GL_STATIC_DRAW);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
			glBindBuffer(GL_ARRAY_BUFFER, Result->ColorsVBO);
			glBufferData(GL_ARRAY_BUFFER, Result->VerticesCount*sizeof(vec3), Result->Colors, GL_STATIC_DRAW);
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
			glBindVertexArray(0);
		}
	}

	return(Result);
}

internal void
InitTexture(loaded_texture *Texture, GLint Addressing)
{
	GLenum Format = 0;
	if(Texture->ChannelsCount == 1) Format = GL_RED;
	else if(Texture->ChannelsCount == 3) Format = GL_RGB;
	else if(Texture->ChannelsCount == 4) Format = GL_RGBA;

	glGenTextures(1, &Texture->TextureID);
	glBindTexture(GL_TEXTURE_2D, Texture->TextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, Format, Texture->Width, Texture->Height, 0, Format, GL_UNSIGNED_BYTE, Texture->Data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Addressing);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Addressing);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if(Addressing == GL_CLAMP_TO_BORDER)
	{
		r32 BorderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, BorderColor);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}

	PLATFORM_FREE_MEMORY_AND_ZERO_POINTER(Texture->Free);
}

inline loaded_texture *
GetTexture(game_assets *GameAssets, texture_id Index)
{
	asset_memory_header *Header = GetAsset(GameAssets, Index.Value);

	loaded_texture *Result = 0;
	if(Header)
	{
		Result = &Header->Texture;

		if(!Result->TextureID)
		{
			InitTexture(Result, GL_REPEAT);
		}
	}

	return(Result);
}

inline loaded_sound *
GetSound(game_assets *GameAssets, sound_id Index)
{
	asset_memory_header* Header = GetAsset(GameAssets, Index.Value);
	loaded_sound *Result = Header ? &Header->Sound : 0;

	return(Result);
}

inline loaded_font *
GetFont(game_assets *GameAssets, font_id Index)
{
	asset_memory_header *Header = GetAsset(GameAssets, Index.Value);
	loaded_font *Result = Header ? &Header->Font : 0;

	return(Result);
}