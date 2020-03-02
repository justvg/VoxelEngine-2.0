#include "voxel_engine_asset.h"

struct load_asset_job
{
	game_assets *GameAssets;
	asset *Asset;
	u32 AssetIndex;

	asset_state FinalState;

	asset_data_type AssetDataType;
};
internal PLATFORM_JOB_SYSTEM_CALLBACK(LoadAssetJob)
{
	load_asset_job *Job = (load_asset_job *)Data;

	game_assets *GameAssets = Job->GameAssets;
	asset *Asset = Job->Asset;
	Assert(!Asset->Header);

	Asset->Header = (asset_memory_header *)Platform.AllocateMemory(sizeof(asset_memory_header));
	Asset->Header->AssetIndex = Job->AssetIndex;

	u64 AssetSize = 0;
	switch(Job->AssetDataType)
	{
		case AssetDataType_Model:
		{
			Asset->Header->Model = {};

			u32 VerticesSize = Asset->VEAAsset.Model.VerticesCount*sizeof(vec3);
			Asset->Header->Model.Positions = (vec3 *)Platform.AllocateMemory(VerticesSize);
			Asset->Header->Model.Normals = (vec3 *)Platform.AllocateMemory(VerticesSize);
			Asset->Header->Model.Colors = (vec3 *)Platform.AllocateMemory(VerticesSize);
			Platform.ReadDataFromFile(GameAssets->File, Asset->Header->Model.Positions, VerticesSize, Asset->VEAAsset.DataOffset);
			Platform.ReadDataFromFile(GameAssets->File, Asset->Header->Model.Normals, VerticesSize, Asset->VEAAsset.DataOffset + (u64)VerticesSize);
			Platform.ReadDataFromFile(GameAssets->File, Asset->Header->Model.Colors, VerticesSize, Asset->VEAAsset.DataOffset + 2*(u64)VerticesSize);

			// TODO(georgy): I can do these stuff before LoadAssetJob. Should I?
			Asset->Header->Model.VerticesCount = Asset->VEAAsset.Model.VerticesCount;
			Asset->Header->Model.Alignment = Asset->VEAAsset.Model.Alignment;
			Asset->Header->Model.AlignmentX = Asset->VEAAsset.Model.AlignmentX;
			Asset->Header->Model.AlignmentZ = Asset->VEAAsset.Model.AlignmentZ;

			AssetSize = 3*VerticesSize;
		} break;

		case AssetDataType_Texture:
		{
			Asset->Header->Texture = {};

			AssetSize = Asset->VEAAsset.Texture.Width*Asset->VEAAsset.Texture.Height*Asset->VEAAsset.Texture.ChannelsCount;
			Asset->Header->Texture.Data = (u8 *)Platform.AllocateMemory(AssetSize);
			Platform.ReadDataFromFile(GameAssets->File, Asset->Header->Texture.Data, (u32)AssetSize, Asset->VEAAsset.DataOffset);

			Asset->Header->Texture.Width = Asset->VEAAsset.Texture.Width;
			Asset->Header->Texture.Height = Asset->VEAAsset.Texture.Height;
			Asset->Header->Texture.ChannelsCount = Asset->VEAAsset.Texture.ChannelsCount;

			Asset->Header->Texture.AlignPercentageY = Asset->VEAAsset.Texture.AlignPercentageY;
		} break;

		case AssetDataType_Sound:
		{
			Asset->Header->Sound = {};

			u32 OneChannelSize = Asset->VEAAsset.Sound.SampleCount*sizeof(i16);
			AssetSize = Asset->VEAAsset.Sound.ChannelsCount*OneChannelSize;

			Asset->Header->Sound.Samples[0] = (i16 *)Platform.AllocateMemory(AssetSize);
			Platform.ReadDataFromFile(GameAssets->File, Asset->Header->Sound.Samples[0], (u32)AssetSize, Asset->VEAAsset.DataOffset);

			for(u32 ChannelIndex = 0;
				ChannelIndex < Asset->VEAAsset.Sound.ChannelsCount;
				ChannelIndex++)
			{
				Asset->Header->Sound.Samples[ChannelIndex] = (i16 *)((u8 *)Asset->Header->Sound.Samples[0] + ChannelIndex*OneChannelSize);
			}

			Asset->Header->Sound.SampleCount = Asset->VEAAsset.Sound.SampleCount;
			Asset->Header->Sound.ChannelsCount = Asset->VEAAsset.Sound.ChannelsCount;

			Asset->Header->Sound.Free = Asset->Header->Sound.Samples[0];
		} break;

		case AssetDataType_Font:
		{
			u32 GlyphsCount = Asset->VEAAsset.Font.GlyphsCount;
			u32 GlyphsInAssetFileSize = GlyphsCount*sizeof(packer_loaded_texture);
			packer_loaded_texture *GlyphsInAssetFile = (packer_loaded_texture *)Platform.AllocateMemory(GlyphsInAssetFileSize);
			Platform.ReadDataFromFile(GameAssets->File, GlyphsInAssetFile, GlyphsInAssetFileSize, Asset->VEAAsset.DataOffset);

			u32 HorizontalAdvancesTableSize = GlyphsCount*GlyphsCount*sizeof(r32);
			Asset->Header->Font.HorizontalAdvances = (r32 *)Platform.AllocateMemory(HorizontalAdvancesTableSize);
			Platform.ReadDataFromFile(GameAssets->File, Asset->Header->Font.HorizontalAdvances, 
									  HorizontalAdvancesTableSize, Asset->VEAAsset.DataOffset + GlyphsInAssetFileSize);

			Asset->Header->Font.LineAdvance = Asset->VEAAsset.Font.LineAdvance;
			Asset->Header->Font.AscenderHeight = Asset->VEAAsset.Font.AscenderHeight;
			Asset->Header->Font.GlyphsCount = Asset->VEAAsset.Font.GlyphsCount;
			Asset->Header->Font.FirstCodepoint = Asset->VEAAsset.Font.FirstCodepoint;
			Asset->Header->Font.LastCodepoint = Asset->VEAAsset.Font.LastCodepoint;

			u64 OffsetForGlyph = Asset->VEAAsset.DataOffset + GlyphsInAssetFileSize + HorizontalAdvancesTableSize;
			Asset->Header->Font.Glyphs = (loaded_texture *)Platform.AllocateMemory(GlyphsCount*sizeof(loaded_texture));
			for(u32 GlyphIndex = 0;
				GlyphIndex < GlyphsCount;
				GlyphIndex++)
			{
				packer_loaded_texture *GlyphFromAssetFile = GlyphsInAssetFile + GlyphIndex;
				loaded_texture *Glyph = Asset->Header->Font.Glyphs + GlyphIndex;

				Glyph->TextureID = 0;
				Glyph->Width = GlyphFromAssetFile->Width;
				Glyph->Height = GlyphFromAssetFile->Height;
				Glyph->ChannelsCount = GlyphFromAssetFile->ChannelsCount;
				Glyph->AlignPercentageY = GlyphFromAssetFile->AlignPercentageY; 

				u32 GlyphSize = Glyph->Width*Glyph->Height*Glyph->ChannelsCount;
				Glyph->Data = (u8 *)Platform.AllocateMemory(GlyphSize);
				Platform.ReadDataFromFile(GameAssets->File, Glyph->Data, GlyphSize, OffsetForGlyph);
				OffsetForGlyph += GlyphSize;
			}

			Platform.FreeMemory(GlyphsInAssetFile);
		} break;

		InvalidDefaultCase;
	}
	Asset->Header->TotalSize = (u32)(sizeof(asset_memory_header) + AssetSize);

	BeginAssetLock(Job->GameAssets);
	if(Job->FinalState == AssetState_Loaded)
	{
		InsertAssetHeaderFront(Job->GameAssets, Asset->Header);
		Job->GameAssets->UsedMemory += Asset->Header->TotalSize;
	}

    CompletePreviousWritesBeforeFutureWrites;

    Asset->State = Job->FinalState;
	EndAssetLock(Job->GameAssets);

    Platform.FreeMemory(Job);
}

internal void
LoadAsset(game_assets *GameAssets, u32 AssetIndex)
{
	if(AssetIndex)
	{
		asset *Asset = GameAssets->Assets + AssetIndex;
		if(Asset->State == AssetState_Unloaded)
		{
			Assert(!Asset->Header);
			Asset->State = AssetState_Queued;

            load_asset_job *Job = (load_asset_job *)Platform.AllocateMemory(sizeof(load_asset_job));
            Job->GameAssets = GameAssets;
            Job->Asset = Asset;
            Job->AssetIndex = AssetIndex;
            Job->FinalState = AssetState_Loaded;
			Job->AssetDataType = Asset->DataType;

			Platform.AddEntry(GameAssets->TempState->JobSystemQueue, LoadAssetJob, Job);
		}
	}
}

inline void
LoadModel(game_assets *GameAssets, model_id Index)
{
	LoadAsset(GameAssets, Index.Value);
}

inline void
LoadTexture(game_assets *GameAssets, texture_id Index)
{
	LoadAsset(GameAssets, Index.Value);
}

inline void
LoadFont(game_assets *GameAssets, font_id Index)
{
	LoadAsset(GameAssets, Index.Value);
}

inline void
LoadSound(game_assets *GameAssets, sound_id Index)
{
	LoadAsset(GameAssets, Index.Value);
}

inline u32 
GetFirstAssetFromType(game_assets *GameAssets, asset_type_id TypeID)
{
	u32 Result = GameAssets->AssetTypes[TypeID].FirstAssetIndex;
	return(Result);
}

inline model_id 
GetFirstModelFromType(game_assets *GameAssets, asset_type_id TypeID)
{
	model_id Result = { GetFirstAssetFromType(GameAssets, TypeID) };
	return(Result);
}

inline texture_id 
GetFirstTextureFromType(game_assets *GameAssets, asset_type_id TypeID)
{
	texture_id Result = { GetFirstAssetFromType(GameAssets, TypeID) };
	return(Result);
}

inline sound_id 
GetFirstSoundFromType(game_assets *GameAssets, asset_type_id TypeID)
{
	sound_id Result = { GetFirstAssetFromType(GameAssets, TypeID) };
	return(Result);
}

inline font_id 
GetFirstFont(game_assets *GameAssets)
{
	font_id Result = { GetFirstAssetFromType(GameAssets, AssetType_Font) };
	return(Result);
}

internal u32
GetBestMatchAssetFromType(game_assets *GameAssets, asset_type_id TypeID, asset_tag_vector *MatchVector)
{
	u32 Result = 0;

	r32 MinDiff = FLT_MAX;
	vea_asset_type *Type = GameAssets->AssetTypes + TypeID;
	for(u32 AssetIndex = Type->FirstAssetIndex;
		AssetIndex < Type->OnePastLastAssetIndex;
		AssetIndex++)
	{
		asset *Asset = GameAssets->Assets + AssetIndex;

		r32 Diff = 0.0f;
		for(u32 TagIndex = Asset->VEAAsset.FirstTagIndex;
			TagIndex < Asset->VEAAsset.OnePastLastTagIndex;
			TagIndex++)
		{
			vea_asset_tag *Tag = GameAssets->Tags + TagIndex;

			r32 A = MatchVector->E[Tag->ID];
			r32 B = Tag->Value;

			Diff += abs(A - B);
		}

		if(Diff < MinDiff)
		{
			MinDiff = Diff;
			Result = AssetIndex;
		}
	}

	return(Result);
}

inline model_id
GetBestMatchModelFromType(game_assets *GameAssets, asset_type_id TypeID, asset_tag_vector *MatchVector)
{
	model_id Result = { GetBestMatchAssetFromType(GameAssets, TypeID, MatchVector) };
	return(Result);
}

inline texture_id
GetBestMatchTextureFromType(game_assets *GameAssets, asset_type_id TypeID, asset_tag_vector *MatchVector)
{
	texture_id Result = { GetBestMatchAssetFromType(GameAssets, TypeID, MatchVector) };
	return(Result);
}

inline sound_id
GetBestMatchSoundFromType(game_assets *GameAssets, asset_type_id TypeID, asset_tag_vector *MatchVector)
{
	sound_id Result = { GetBestMatchAssetFromType(GameAssets, TypeID, MatchVector) };
	return(Result);
}

inline font_id
GetBestMatchFont(game_assets *GameAssets, asset_tag_vector *MatchVector)
{
	font_id Result = { GetBestMatchAssetFromType(GameAssets, AssetType_Font, MatchVector) };
	return(Result);
}

internal game_assets *
AllocateGameAssets(temp_state *TempState, stack_allocator *Allocator, u64 Size)
{
	game_assets *GameAssets = PushStruct(Allocator, game_assets);
	*GameAssets = {};
	
	GameAssets->MemorySizeRestriction = Size;
	GameAssets->MemoryHeaderSentinel.Next = GameAssets->MemoryHeaderSentinel.Prev = &GameAssets->MemoryHeaderSentinel;
	GameAssets->TempState = TempState;

	GameAssets->File = Platform.OpenFile("data.vea");

	vea_header Header;
	Platform.ReadDataFromFile(GameAssets->File, &Header, sizeof(Header), 0);
	Assert(Header.MagicValue == VEA_MAGIC_VALUE);

	GameAssets->AssetCount = Header.AssetCount;
	GameAssets->TagCount = Header.TagCount;

	GameAssets->Assets = PushArray(Allocator, Header.AssetCount, asset);
	GameAssets->Tags = PushArray(Allocator, Header.TagCount, vea_asset_tag);
	Platform.ReadDataFromFile(GameAssets->File, GameAssets->AssetTypes, AssetType_Count*sizeof(vea_asset_type), Header.AssetTypes);
	Platform.ReadDataFromFile(GameAssets->File, GameAssets->Tags, Header.TagCount*sizeof(vea_asset_tag), Header.Tags);

	ZeroArray(GameAssets->Assets, Header.AssetCount);
	temporary_memory TempMem = BeginTemporaryMemory(Allocator);
	vea_asset *VEAAssets = PushArray(Allocator, Header.AssetCount, vea_asset);
	Platform.ReadDataFromFile(GameAssets->File, VEAAssets, Header.AssetCount*sizeof(vea_asset), Header.Assets);

	for(u32 AssetIndex = 1;
		AssetIndex < GameAssets->AssetCount;
		AssetIndex++)
	{
		vea_asset *Source = VEAAssets + AssetIndex;
		asset *Dest = GameAssets->Assets + AssetIndex;

		Dest->DataType = Source->DataType;
		Dest->VEAAsset = *Source;
	}

	EndTemporaryMemory(TempMem);

	return(GameAssets);
}

inline void 
FreeTexture(loaded_texture *Texture)
{
	PLATFORM_FREE_MEMORY_AND_ZERO_POINTER(Texture->Free);
	glDeleteTextures(1, &Texture->TextureID);
}

inline void
UnloadAssetsIfNecessary(game_assets *GameAssets)
{
	BeginAssetLock(GameAssets);
	while(GameAssets->MemorySizeRestriction < GameAssets->UsedMemory)
	{
		asset_memory_header *LastUsedAsset = GameAssets->MemoryHeaderSentinel.Prev;
		if(LastUsedAsset != &GameAssets->MemoryHeaderSentinel)
		{
			RemoveAssetHeaderFromList(LastUsedAsset);

			switch(GameAssets->Assets[LastUsedAsset->AssetIndex].DataType)
			{
				case AssetDataType_Model:
				{
					glDeleteBuffers(1, &LastUsedAsset->Model.PVBO);
					glDeleteBuffers(1, &LastUsedAsset->Model.NormalsVBO);
					glDeleteBuffers(1, &LastUsedAsset->Model.ColorsVBO);
					glDeleteVertexArrays(1, &LastUsedAsset->Model.VAO);

					Platform.FreeMemory(LastUsedAsset->Model.Positions);
					Platform.FreeMemory(LastUsedAsset->Model.Normals);
					Platform.FreeMemory(LastUsedAsset->Model.Colors);
				} break;
			
				case AssetDataType_Texture:
				{
					FreeTexture(&LastUsedAsset->Texture);
				} break;

				case AssetDataType_Sound:
				{
					PLATFORM_FREE_MEMORY_AND_ZERO_POINTER(LastUsedAsset->Sound.Free);
				} break;

				case AssetDataType_Font:
				{
					for(u32 GlyphIndex = 0;
						GlyphIndex < LastUsedAsset->Font.GlyphsCount;
						GlyphIndex++)
					{
						loaded_texture *Glyph = LastUsedAsset->Font.Glyphs + GlyphIndex; 
						FreeTexture(Glyph);
					}
					PLATFORM_FREE_MEMORY_AND_ZERO_POINTER(LastUsedAsset->Font.Glyphs);
					PLATFORM_FREE_MEMORY_AND_ZERO_POINTER(LastUsedAsset->Font.HorizontalAdvances);
				} break;

				InvalidDefaultCase;
			}

			GameAssets->Assets[LastUsedAsset->AssetIndex].Header = 0;
			GameAssets->Assets[LastUsedAsset->AssetIndex].State = AssetState_Unloaded;

			GameAssets->UsedMemory -= LastUsedAsset->TotalSize;
			Platform.FreeMemory(LastUsedAsset);
		}
	}
	EndAssetLock(GameAssets);
}

inline u32
GlyphIndexFromCodepoint(loaded_font *Font, u32 Codepoint)
{
	u32 GlyphIndex = Codepoint - Font->FirstCodepoint;
	return(GlyphIndex);
}

internal loaded_texture *
GetBitmapForGlyph(loaded_font *Font, u32 Codepoint)
{
	Assert((((i32)Codepoint - (i32)Font->FirstCodepoint) >= 0) && 
		   (((i32)Codepoint - (i32)Font->FirstCodepoint) < (i32)Font->GlyphsCount));

	u32 GlyphIndex = GlyphIndexFromCodepoint(Font, Codepoint);
	loaded_texture *Glyph = Font->Glyphs + GlyphIndex;
	if(!Glyph->TextureID)
	{
		InitTexture(Glyph, GL_CLAMP_TO_BORDER);
	}

	return(Glyph);
}

internal r32
GetHorizontalAdvanceFor(loaded_font *Font, u32 Codepoint, u32 NextCodepoint)
{
	r32 Result = 0.0f;

	if(Codepoint && NextCodepoint)
	{
		Assert((((i32)Codepoint - (i32)Font->FirstCodepoint) >= 0) && 
		   	   (((i32)Codepoint - (i32)Font->FirstCodepoint) < (i32)Font->GlyphsCount));
		Assert((((i32)NextCodepoint - (i32)Font->FirstCodepoint) >= 0) && 
			   (((i32)NextCodepoint - (i32)Font->FirstCodepoint) < (i32)Font->GlyphsCount));

		u32 GlyphIndex = GlyphIndexFromCodepoint(Font, Codepoint);
		u32 NextGlyphIndex = GlyphIndexFromCodepoint(Font, NextCodepoint);
		Result = Font->HorizontalAdvances[GlyphIndex*Font->GlyphsCount + NextGlyphIndex];
	}

	return(Result);
}