#include "voxel_engine_asset.h"

internal loaded_model
LoadCub(char *Filename, u64 *ModelSize, r32 AdditionalAlignmentY = 0.0f, r32 AdditionalAlignmentX = 0.0f)
{
	TIME_BLOCK;
	loaded_model Model = {};

	u32 Width, Height, Depth;
	read_entire_file_result FileData = PlatformReadEntireFile(Filename);
	if(FileData.Size != 0)
	{
		InitializeDynamicArray(&Model.VerticesP);
		InitializeDynamicArray(&Model.Normals);
		InitializeDynamicArray(&Model.VertexColors);

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
							AddQuad(&Model.VerticesP, A, B, C, D);
							AddQuad(&Model.Normals, N, N, N, N);
							AddQuad(&Model.VertexColors, Color, Color, Color, Color);
						}

						if ((VoxelX == Width - 1) || !VoxelsStates[VoxelY*Width*Depth + VoxelZ*Width + (VoxelX+1)])
						{
							A = vec3(X + BlockDimInMeters, Y, Z);
							B = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
							C = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
							vec3 N = vec3(1.0f, 0.0f, 0.0f);
							AddQuad(&Model.VerticesP, A, B, C, D);
							AddQuad(&Model.Normals, N, N, N, N);
							AddQuad(&Model.VertexColors, Color, Color, Color, Color);
						}

						if ((VoxelZ == 0) || !VoxelsStates[VoxelY*Width*Depth + (VoxelZ-1)*Width + VoxelX])
						{
							A = vec3(X, Y, Z);
							B = vec3(X, Y + BlockDimInMeters, Z);
							C = vec3(X + BlockDimInMeters, Y, Z);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
							vec3 N = vec3(0.0f, 0.0f, -1.0f);
							AddQuad(&Model.VerticesP, A, B, C, D);
							AddQuad(&Model.Normals, N, N, N, N);
							AddQuad(&Model.VertexColors, Color, Color, Color, Color);
						}

						if ((VoxelZ == Depth - 1) || !VoxelsStates[VoxelY*Width*Depth + (VoxelZ+1)*Width + VoxelX])
						{
							A = vec3(X, Y, Z + BlockDimInMeters);
							B = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							C = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
							vec3 N = vec3(0.0f, 0.0f, 1.0f);
							AddQuad(&Model.VerticesP, A, B, C, D);
							AddQuad(&Model.Normals, N, N, N, N);
							AddQuad(&Model.VertexColors, Color, Color, Color, Color);
						}

						if ((VoxelY == 0) || !VoxelsStates[(VoxelY-1)*Width*Depth + VoxelZ*Width + VoxelX])
						{
							A = vec3(X, Y, Z);
							B = vec3(X + BlockDimInMeters, Y, Z);
							C = vec3(X, Y, Z + BlockDimInMeters);
							D = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							vec3 N = vec3(0.0f, -1.0f, 0.0f);
							AddQuad(&Model.VerticesP, A, B, C, D);
							AddQuad(&Model.Normals, N, N, N, N);
							AddQuad(&Model.VertexColors, Color, Color, Color, Color);
						}

						if ((VoxelY == Height - 1) || !VoxelsStates[(VoxelY+1)*Width*Depth + VoxelZ*Width + VoxelX])
						{
							A = vec3(X, Y + BlockDimInMeters, Z);
							B = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
							C = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
							vec3 N = vec3(0.0f, 1.0f, 0.0f);
							AddQuad(&Model.VerticesP, A, B, C, D);
							AddQuad(&Model.Normals, N, N, N, N);
							AddQuad(&Model.VertexColors, Color, Color, Color, Color);
						}
					}
				}	
			}
		}

		Model.VerticesCount = Model.VerticesP.EntriesCount;
		*ModelSize = Model.VerticesP.EntriesCount*sizeof(vec3) + Model.Normals.EntriesCount*sizeof(vec3) + Model.VertexColors.EntriesCount*sizeof(vec3);

		PlatformFreeMemory(VoxelsStates);
		PlatformFreeMemory(FileData.Memory);
	}

	return (Model);
}

#pragma pack(push, 1)
struct bmp_file_header
{
	u16 FileType;
	u32 FileSize;
	u32 Reserved;
	u32 BitmapOffset;
	u32 InfoHeaderSize;
	u32 Width;
	u32 Height;
	u16 Planes;
	u16 BitsPerPixel;
	u32 Compression;
	u32 SizeOfBitmap;
	u32 HorizontalResolution;
	u32 VerticalResolution;
	u32 ColorsUsed;
	u32 ColorsImportant;

	u32 RedMask;
	u32 GreenMask;
	u32 BlueMask;
};
#pragma pack(pop)

internal loaded_texture
LoadBMP(char *Filename)
{
	TIME_BLOCK;
	loaded_texture Result = {};

	read_entire_file_result FileData = PlatformReadEntireFile(Filename);
	if(FileData.Size != 0)
	{
		bmp_file_header *Header = (bmp_file_header *)FileData.Memory;
		u32 *Pixels = (u32 *)((u8 *)Header + Header->BitmapOffset);
		Result.Data = (u8 *)Pixels;
		Result.Width = Header->Width;
		Result.Height = Header->Height;
		Result.Free = FileData.Memory;
		Result.ChannelsCount = Header->BitsPerPixel / 8;

		Assert(Header->Height >= 0);
		Assert(Header->Compression == 3);

		u32 RedMask = Header->RedMask;
		u32 GreenMask = Header->GreenMask;
		u32 BlueMask = Header->BlueMask;
		u32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

		u32 RedShiftRight = FindLeastSignificantSetBitIndex(RedMask);
		u32 GreenShiftRight = FindLeastSignificantSetBitIndex(GreenMask);
		u32 BlueShiftRight = FindLeastSignificantSetBitIndex(BlueMask);
		u32 AlphaShiftRight = FindLeastSignificantSetBitIndex(AlphaMask);

		u32 *SourceDest = Pixels;
		for(u32 Y = 0;
			Y < Header->Height;
			Y++)
		{
			for(u32 X = 0;
				X < Header->Width;
				X++)
			{
				u32 Color = *SourceDest;

				u8 Red = (Color & RedMask) >> RedShiftRight;
				u8 Green = (Color & GreenMask) >> GreenShiftRight;
				u8 Blue = (Color & BlueMask) >> BlueShiftRight;
				u8 Alpha = (Color & AlphaMask) >> AlphaShiftRight;

				*SourceDest++ = ((Alpha << 24) |
								 (Blue << 16) |
								 (Green << 8) |
								 (Red << 0));
			}
		}
	}

	return(Result);
}

internal loaded_font
LoadFont(char *Filename, char *FontName, u64 *AssetSize)
{
	TIME_BLOCK;
	loaded_font Result = {};
	Result.FirstCodepoint = ' ';
	Result.LastCodepoint = '~';
	Result.GlyphsCount = (Result.LastCodepoint + 1) - Result.FirstCodepoint;

	Result.Glyphs = (loaded_texture *)PlatformAllocateMemory(Result.GlyphsCount*sizeof(loaded_texture));

	PlatformBeginFont(&Result, Filename, FontName);

	for(u32 Character = Result.FirstCodepoint, GlyphIndex = 0;
		Character <= Result.LastCodepoint;
		Character++, GlyphIndex++)
	{
		loaded_texture *Glyph = Result.Glyphs + GlyphIndex;
		Glyph->Data = PlatformLoadCodepointBitmap(&Result, Character, (u32 *)&Glyph->Width, (u32 *)&Glyph->Height, 
												  &Glyph->AlignPercentageY);
		Glyph->Free = Glyph->Data;
		Glyph->ChannelsCount = 1;

		*AssetSize += Glyph->Width*Glyph->Height*Glyph->ChannelsCount;
	}

	PlatformEndFont(&Result);

	return(Result);
}

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

	asset *Asset = Job->Asset;
	Assert(!Asset->Header);

	Asset->Header = (asset_memory_header *)PlatformAllocateMemory(sizeof(asset_memory_header));
	u64 AssetSize = 0;
	Asset->Header->AssetIndex = Job->AssetIndex;
	switch(Job->AssetDataType)
	{
		case AssetDataType_Model:
		{
			Asset->Header->Model = LoadCub(Asset->Filename, &AssetSize, Asset->AdditionalAlignmentY, Asset->AdditionalAlignmentX);
		} break;

		case AssetDataType_Texture:
		{
			Asset->Header->Texture = LoadBMP(Asset->Filename);
			AssetSize = Asset->Header->Texture.Width*Asset->Header->Texture.Height*Asset->Header->Texture.ChannelsCount;
		} break;

		case AssetDataType_Font:
		{
			// NOTE(georgy): This lock is used because we don't want 2 different fonts to be in fly in platform layer in the same time
			BeginAssetLock(Job->GameAssets);
			Asset->Header->Font = LoadFont(Asset->Filename, Asset->FontName, &AssetSize);
			EndAssetLock(Job->GameAssets);
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

    PlatformFreeMemory(Job);
}

inline void
LoadAsset(game_assets *GameAssets, u32 AssetIndex)
{
	if(AssetIndex)
	{
		asset *Asset = GameAssets->Assets + AssetIndex;
		if(Asset->State == AssetState_Unloaded)
		{
			Assert(!Asset->Header);
			Asset->State = AssetState_Queued;

            load_asset_job *Job = (load_asset_job *)PlatformAllocateMemory(sizeof(load_asset_job));
            Job->GameAssets = GameAssets;
            Job->Asset = Asset;
            Job->AssetIndex = AssetIndex;
            Job->FinalState = AssetState_Loaded;
			Job->AssetDataType = Asset->DataType;

			PlatformAddEntry(GameAssets->TempState->JobSystemQueue, LoadAssetJob, Job);
		}
	}
}

internal void
LoadModel(game_assets *GameAssets, model_id Index)
{
	LoadAsset(GameAssets, Index.Value);
}

internal void
LoadTexture(game_assets *GameAssets, texture_id Index)
{
	LoadAsset(GameAssets, Index.Value);
}

internal void
LoadFont(game_assets *GameAssets, font_id Index)
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

inline font_id
GetBestMatchFont(game_assets *GameAssets, asset_tag_vector *MatchVector)
{
	font_id Result = { GetBestMatchAssetFromType(GameAssets, AssetType_Font, MatchVector) };
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
AddAsset(game_assets *Assets, char *Filename, asset_data_type DataType, 
		 r32 AdditionalAlignmentY = 0.0f, r32 AdditionalAlignmentX = 0.0f,
		 char *FontName = 0)
{
	Assert(Assets->DEBUGAssetType);

	asset *Asset = Assets->Assets + Assets->AssetCount++;
	Asset->State = AssetState_Unloaded;
	Asset->DataType = DataType;
	Asset->Filename = Filename;
	Asset->AdditionalAlignmentY = AdditionalAlignmentY;
	Asset->AdditionalAlignmentX = AdditionalAlignmentX;
	Asset->FontName = FontName;
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
AllocateGameAssets(temp_state *TempState, stack_allocator *Allocator, u64 Size)
{
	game_assets *GameAssets = PushStruct(Allocator, game_assets);
	GameAssets->AssetCount = 0;
	GameAssets->TagCount = 0;
	
	GameAssets->MemorySizeRestriction = Size;
	GameAssets->UsedMemory = 0;

	GameAssets->MemoryHeaderSentinel.Next = GameAssets->MemoryHeaderSentinel.Prev = &GameAssets->MemoryHeaderSentinel;

	GameAssets->Lock = 0;

	GameAssets->TempState = TempState;

	GameAssets->DEBUGAssetType = 0;
	GameAssets->DEBUGAsset = 0;

	BeginAssetType(GameAssets, AssetType_Null);
	AddAsset(GameAssets, "", AssetDataType_Null);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Head);
	AddAsset(GameAssets, "data/models/head1.cub", AssetDataType_Model, 0.47f);
	AddTag(GameAssets, Tag_Color, 1.0f);
	AddAsset(GameAssets, "data/models/head2.cub", AssetDataType_Model, 0.47f);
	AddTag(GameAssets, Tag_Color, 10.0f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Shoulders);
	AddAsset(GameAssets, "data/models/shoulders.cub", AssetDataType_Model, 0.4f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Body);
	AddAsset(GameAssets, "data/models/body.cub", AssetDataType_Model, 0.08f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Hand);
	AddAsset(GameAssets, "data/models/hand.cub", AssetDataType_Model, 0.32f, 0.275f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Foot);
	AddAsset(GameAssets, "data/models/foot.cub", AssetDataType_Model, 0.0f, 0.15f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Tree);
	AddAsset(GameAssets, "data/models/tree.cub", AssetDataType_Model, 0.0f, 0.0f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Smoke);
	AddAsset(GameAssets, "data/textures/smoke.bmp", AssetDataType_Texture);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Fire);
	AddAsset(GameAssets, "data/textures/particleAtlas.bmp", AssetDataType_Texture);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Cosmic);
	AddAsset(GameAssets, "data/textures/cosmic.bmp", AssetDataType_Texture);
	EndAssetType(GameAssets);

	// TODO(georgy): It's better to use asset packer I think, because this path may be different on an other machine
	// 				 so we can pre-pack all textures for font on our machine
	// 				 Also asset packer allows us to treat character bitmap as any other bitmap (texture)
	BeginAssetType(GameAssets, AssetType_Font);
	AddAsset(GameAssets, "C:/Windows/Fonts/Arial.ttf", AssetDataType_Font, 0, 0, "Arial");
	AddTag(GameAssets, Tag_FontType, (r32)FontType_DebugFont);
	AddAsset(GameAssets, "C:/Windows/Fonts/Comic Sans MS.ttf", AssetDataType_Font, 0, 0, "Comic Sans MS");
	AddTag(GameAssets, Tag_FontType, (r32)FontType_GameFont);
	EndAssetType(GameAssets);

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

					FreeDynamicArray(&LastUsedAsset->Model.VerticesP);
					FreeDynamicArray(&LastUsedAsset->Model.Normals);
					FreeDynamicArray(&LastUsedAsset->Model.VertexColors);
				} break;
			
				case AssetDataType_Texture:
				{
					FreeTexture(&LastUsedAsset->Texture);
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
			PlatformFreeMemory(LastUsedAsset);
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