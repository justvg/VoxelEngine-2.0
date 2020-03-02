#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "voxel_engine_platform.h"
#include "voxel_engine_intrinsics.h"
#include "voxel_engine_math.h"
#include "voxel_engine_file_formats.h"

internal read_entire_file_result
ReadEntireFile(char *Filename)
{
	read_entire_file_result Result = {};

	FILE *File = fopen(Filename, "rb");
	if(File)
	{
		fseek(File, 0, SEEK_END);
		Result.Size = ftell(File);
		fseek(File, 0, SEEK_SET);

		Result.Memory = malloc(Result.Size);
		fread(Result.Memory, Result.Size, 1, File);
		fclose(File);
	}
	else
	{
		printf("ERROR: Cannot open file %s.\n", Filename);
	}

	return(Result);
}

struct packer_loaded_model
{
	packer_loaded_model() {};

	u32 VerticesCount;
	std::vector<vec3> *Positions;
	std::vector<vec3> *Normals;
	std::vector<vec3> *Colors;

	vec3 Alignment;
	r32 AlignmentX; // NOTE(georgy): This is for models like hands, where we need to multiply this by EntityRight vec
	r32 AlignmentZ; // NOTE(georgy): This is for models like sword, where we need to multiply this by EntityForward vec
};

struct packer_loaded_sound
{
	i16 *Samples[2];
	u32 SampleCount;
	u32 ChannelsCount;

	void *Free;
};

inline void
AddTriangle(std::vector<vec3> *Array, vec3 A, vec3 B, vec3 C)
{
	Array->push_back(A);
	Array->push_back(B);
	Array->push_back(C);
}

inline void
AddQuad(std::vector<vec3> *Array, vec3 A, vec3 B, vec3 C, vec3 D)
{
	AddTriangle(Array, A, B, C);
	AddTriangle(Array, C, B, D);
}

internal packer_loaded_model
LoadCUB(char *Filename, r32 AdditionalAlignmentY = 0.0f, r32 AdditionalAlignmentX = 0.0f, r32 AdditionalAlignmentZ = 0.0f)
{
	packer_loaded_model Model;

	Model.VerticesCount = 0;
	Model.Alignment = vec3(0.0f, 0.0f, 0.0f);
	Model.AlignmentX = 0.0f;
	Model.AlignmentZ = 0.0f;

	u32 Width, Height, Depth;
	read_entire_file_result FileData = ReadEntireFile(Filename);
	if(FileData.Size != 0)
	{
		Model.Positions = new std::vector<vec3>;
		Model.Normals = new std::vector<vec3>;
		Model.Colors = new std::vector<vec3>;

		Width = *((u32 *)FileData.Memory);
		Depth = *((u32 *)FileData.Memory + 1);
		Height = *((u32 *)FileData.Memory + 2);

		bool8 *VoxelsStates = (bool8 *)malloc(sizeof(bool8)*Width*Height*Depth);
		memset(VoxelsStates, 0, sizeof(bool8) * Width * Height * Depth);
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
					if((R != 0) || (G != 0) || (B != 0))
					{
						VoxelsStates[VoxelY*Width*Depth + VoxelZ*Width + VoxelX] = true;
					}
				}
			}
		}

		r32 BlockDimInMeters = 0.03f;
		Model.Alignment = vec3(0.0f, 0.5f*Height*BlockDimInMeters + AdditionalAlignmentY, 0.0f);
		Model.AlignmentX = AdditionalAlignmentX;
		Model.AlignmentZ = AdditionalAlignmentZ;
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
						Assert((Red != 0) || (Green != 0) || (Blue != 0));
						vec3 Color = vec3(Red / 255.0f, Green / 255.0f, Blue / 255.0f);

						if ((VoxelX == 0) || !VoxelsStates[VoxelY*Width*Depth + VoxelZ*Width + (VoxelX-1)])
						{
							A = vec3(X, Y, Z);
							B = vec3(X, Y, Z + BlockDimInMeters);
							C = vec3(X, Y + BlockDimInMeters, Z);
							D = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
							vec3 N = vec3(-1.0f, 0.0f, 0.0f);
							AddQuad(Model.Positions, A, B, C, D);
							AddQuad(Model.Normals, N, N, N, N);
							AddQuad(Model.Colors, Color, Color, Color, Color);
						}

						if ((VoxelX == Width - 1) || !VoxelsStates[VoxelY*Width*Depth + VoxelZ*Width + (VoxelX+1)])
						{
							A = vec3(X + BlockDimInMeters, Y, Z);
							B = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
							C = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
							vec3 N = vec3(1.0f, 0.0f, 0.0f);
							AddQuad(Model.Positions, A, B, C, D);
							AddQuad(Model.Normals, N, N, N, N);
							AddQuad(Model.Colors, Color, Color, Color, Color);
						}

						if ((VoxelZ == 0) || !VoxelsStates[VoxelY*Width*Depth + (VoxelZ-1)*Width + VoxelX])
						{
							A = vec3(X, Y, Z);
							B = vec3(X, Y + BlockDimInMeters, Z);
							C = vec3(X + BlockDimInMeters, Y, Z);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
							vec3 N = vec3(0.0f, 0.0f, -1.0f);
							AddQuad(Model.Positions, A, B, C, D);
							AddQuad(Model.Normals, N, N, N, N);
							AddQuad(Model.Colors, Color, Color, Color, Color);
						}

						if ((VoxelZ == Depth - 1) || !VoxelsStates[VoxelY*Width*Depth + (VoxelZ+1)*Width + VoxelX])
						{
							A = vec3(X, Y, Z + BlockDimInMeters);
							B = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							C = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
							vec3 N = vec3(0.0f, 0.0f, 1.0f);
							AddQuad(Model.Positions, A, B, C, D);
							AddQuad(Model.Normals, N, N, N, N);
							AddQuad(Model.Colors, Color, Color, Color, Color);
						}

						if ((VoxelY == 0) || !VoxelsStates[(VoxelY-1)*Width*Depth + VoxelZ*Width + VoxelX])
						{
							A = vec3(X, Y, Z);
							B = vec3(X + BlockDimInMeters, Y, Z);
							C = vec3(X, Y, Z + BlockDimInMeters);
							D = vec3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
							vec3 N = vec3(0.0f, -1.0f, 0.0f);
							AddQuad(Model.Positions, A, B, C, D);
							AddQuad(Model.Normals, N, N, N, N);
							AddQuad(Model.Colors, Color, Color, Color, Color);
						}

						if ((VoxelY == Height - 1) || !VoxelsStates[(VoxelY+1)*Width*Depth + VoxelZ*Width + VoxelX])
						{
							A = vec3(X, Y + BlockDimInMeters, Z);
							B = vec3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
							C = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
							D = vec3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
							vec3 N = vec3(0.0f, 1.0f, 0.0f);
							AddQuad(Model.Positions, A, B, C, D);
							AddQuad(Model.Normals, N, N, N, N);
							AddQuad(Model.Colors, Color, Color, Color, Color);
						}
					}
				}	
			}
		}

		Model.VerticesCount = (u32)Model.Positions->size();

		free(VoxelsStates);
		free(FileData.Memory);
	}

	return(Model);
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

internal packer_loaded_texture
LoadBMP(char *Filename)
{
	packer_loaded_texture Result = {};

	read_entire_file_result FileData = ReadEntireFile(Filename);
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

				u8 Red = (u8)((Color & RedMask) >> RedShiftRight);
				u8 Green = (u8)((Color & GreenMask) >> GreenShiftRight);
				u8 Blue = (u8)((Color & BlueMask) >> BlueShiftRight);
				u8 Alpha = (u8)((Color & AlphaMask) >> AlphaShiftRight);

				*SourceDest++ = ((Alpha << 24) |
								 (Blue << 16) |
								 (Green << 8) |
								 (Red << 0));
			}
		}
	}

	return(Result);
}

#pragma pack(push, 1)
struct wave_header
{
	u32 RIFFID;
	u32 Size;
	u32 WAVEID;
};

#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
enum
{
	WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
	WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
	WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
};

struct wave_chunk
{
    u32 ID;
    u32 Size;
};

struct wave_fmt
{
	u16 FormatTag;
	u16 ChannelsCount;
	u32 SamplesPerSec;
	u32 AvgBytesPerSec;
	u16 BlockAlign;
	u16 BitsPerSample;
	u16 cbSize;
    u16 wValidBitsPerSample;
    u32 dwChannelMask;
    u8 SubFormat[16];
};

struct wave_data
{
	i16 *Samples;
};
#pragma pack(push)

struct wave_chunk_iterator
{
	u8 *At;
	u8 *Stop;
};

inline wave_chunk_iterator
StartWaveChunkIterator(void *At, void *Stop)
{
	wave_chunk_iterator Iter;

	Iter.At = (u8 *)At;
	Iter.Stop = (u8 *)Stop;

	return(Iter);
}

inline bool32 
IsValid(wave_chunk_iterator Iter)
{
	bool32 Result = Iter.At < Iter.Stop;

	return(Result);
}

inline wave_chunk_iterator
NextChunk(wave_chunk_iterator Iter)
{
	wave_chunk *Chunk = (wave_chunk *)Iter.At;
	u32 Size = (Chunk->Size + 1) & ~1;
	Iter.At += sizeof(wave_chunk) + Size;

	return(Iter);
}

inline u32
GetType(wave_chunk_iterator Iter)
{
	wave_chunk *Chunk = (wave_chunk *)Iter.At;
	u32 Result = Chunk->ID;

	return(Result);
}

inline void *
GetChunkData(wave_chunk_iterator Iter)
{
	void *Result = Iter.At + sizeof(wave_chunk);

	return(Result);
}

inline u32
GetChunkDataSize(wave_chunk_iterator Iter)
{
	wave_chunk *Chunk = (wave_chunk *)Iter.At;
	u32 Result = Chunk->Size;

	return(Result);
}

internal packer_loaded_sound
LoadWAV(char *Filename)
{
	packer_loaded_sound Result = {};

	read_entire_file_result FileData = ReadEntireFile(Filename);
	if(FileData.Size != 0)
	{
		Result.Free = FileData.Memory;

		wave_header *Header = (wave_header *)FileData.Memory;
		Assert(Header->RIFFID == WAVE_ChunkID_RIFF);
		Assert(Header->WAVEID == WAVE_ChunkID_WAVE);

		u32 SamplesDataSize = 0;
		i16 *Samples = 0;
		for(wave_chunk_iterator Iter = StartWaveChunkIterator(Header + 1, (u8 *)(Header + 1) + Header->Size - 4);
			IsValid(Iter);
			Iter = NextChunk(Iter))
		{
			switch(GetType(Iter))
			{
				case WAVE_ChunkID_fmt:
				{
					wave_fmt *FMT = (wave_fmt *)GetChunkData(Iter);
					Assert(FMT->FormatTag == 1); // NOTE(georgy): PCM
					Assert(FMT->SamplesPerSec == 44100);
					Assert(FMT->BitsPerSample == 16);
					Assert(FMT->BlockAlign == (sizeof(i16)*FMT->ChannelsCount));
					Result.ChannelsCount = FMT->ChannelsCount;
				} break;

				case WAVE_ChunkID_data:
				{
					Samples = (i16 *)GetChunkData(Iter);
					SamplesDataSize = GetChunkDataSize(Iter);
				} break;
			}
		}

		Result.SampleCount = SamplesDataSize / (Result.ChannelsCount*sizeof(i16));

		if(Result.ChannelsCount == 1)
		{
			Result.Samples[0] = Samples;
			Result.Samples[1] = 0;
		}
		else if(Result.ChannelsCount == 2)
		{
			Result.Samples[0] = Samples;
			Result.Samples[1] = Samples + Result.SampleCount;

			for(u32 SampleIndex = 0;
				SampleIndex < Result.SampleCount;
				SampleIndex++)
			{
				u16 Temp = Samples[2*SampleIndex];
				Samples[2*SampleIndex] = Samples[SampleIndex];
				Samples[SampleIndex] = Temp;
			}
		}
		else
		{
			Assert(!"Invalid channel count!");
		}

		// TODO(georgy): At the moment we use only left channel (the right is the same). Fix this!
		Result.ChannelsCount = 1;
	}

	return(Result);
}

global_variable bool32 GlobalIsFontRenderTargetInitialized;
global_variable HDC GlobalFontDeviceContext;
global_variable HBITMAP GlobalFontBitmap;
global_variable void *GlobalFontBits;
global_variable HFONT GlobalFont;
global_variable TEXTMETRIC GlobalTextMetric;
#define MAX_GLYPH_WIDTH 300
#define MAX_GLYPH_HEIGHT 300
internal void
BeginFont(packer_loaded_font *Font, char *Filename, char *FontName)
{
	if(!GlobalIsFontRenderTargetInitialized)
	{
		GlobalFontDeviceContext = CreateCompatibleDC(GetDC(0));

		BITMAPINFO Info = {};
		Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
		Info.bmiHeader.biWidth = MAX_GLYPH_WIDTH;
		Info.bmiHeader.biHeight = MAX_GLYPH_HEIGHT;
		Info.bmiHeader.biPlanes = 1;
		Info.bmiHeader.biBitCount = 32;
		Info.bmiHeader.biCompression = BI_RGB;
		GlobalFontBitmap = CreateDIBSection(GlobalFontDeviceContext, &Info, DIB_RGB_COLORS, &GlobalFontBits, 0 , 0);
		SelectObject(GlobalFontDeviceContext, GlobalFontBitmap);
		SetBkColor(GlobalFontDeviceContext, RGB(0, 0, 0));

		GlobalIsFontRenderTargetInitialized = true;
	}
	
	AddFontResourceEx(Filename, FR_PRIVATE, 0);
	int PixelHeight = 128;
	GlobalFont = CreateFontA(PixelHeight, 0, 0, 0,
							 FW_NORMAL,
							 FALSE, // NOTE(georgy): Italic
							 FALSE, // NOTE(georgy): Underline,
							 FALSE, // NOTE(georgy): StrikeOut,
							 DEFAULT_CHARSET,
							 OUT_DEFAULT_PRECIS,
							 CLIP_DEFAULT_PRECIS,
							 ANTIALIASED_QUALITY,
							 DEFAULT_PITCH|FF_DONTCARE,
							 FontName);

	SelectObject(GlobalFontDeviceContext, GlobalFont);
	GetTextMetrics(GlobalFontDeviceContext, &GlobalTextMetric);

	Font->HorizontalAdvances = (r32 *)malloc(Font->GlyphsCount*Font->GlyphsCount*sizeof(r32));
	memset(Font->HorizontalAdvances, 0, Font->GlyphsCount*Font->GlyphsCount*sizeof(r32));
	Font->LineAdvance = GlobalTextMetric.tmAscent + GlobalTextMetric.tmDescent + GlobalTextMetric.tmExternalLeading;
	Font->AscenderHeight = GlobalTextMetric.tmAscent;
}

internal void
EndFont(packer_loaded_font *Font)
{
	DWORD KerningPairsCount = GetKerningPairsW(GlobalFontDeviceContext, 0, 0);
	KERNINGPAIR *KerningPairs = (KERNINGPAIR *)malloc(KerningPairsCount*sizeof(KERNINGPAIR));
	GetKerningPairsW(GlobalFontDeviceContext, KerningPairsCount, KerningPairs);
	for(u32 KerningPairIndex = 0;
		KerningPairIndex < KerningPairsCount;
		KerningPairIndex++)
	{
		KERNINGPAIR *Pair = KerningPairs + KerningPairIndex;
		if((Pair->wFirst >= Font->FirstCodepoint) && (Pair->wFirst <= Font->LastCodepoint) &&
		   (Pair->wSecond >= Font->FirstCodepoint) && (Pair->wSecond <= Font->LastCodepoint))
		{
			u32 FirstGlyphIndex = Pair->wFirst - Font->FirstCodepoint;
			u32 SecondGlyphIndex = Pair->wSecond - Font->FirstCodepoint;
			Font->HorizontalAdvances[FirstGlyphIndex*Font->GlyphsCount + SecondGlyphIndex] += (r32)Pair->iKernAmount;
		}
	}
	free(KerningPairs);

	DeleteObject(GlobalFont);
	GlobalFont = 0;
	GlobalTextMetric = {};
}

internal u8 *
LoadCodepointBitmap(packer_loaded_font *Font, u32 Codepoint, u32 *Width, u32 *Height, r32 *AlignPercentageY)
{
	SelectObject(GlobalFontDeviceContext, GlobalFont);

	wchar_t WPoint = (wchar_t) Codepoint;

	SIZE Size;
	GetTextExtentPoint32W(GlobalFontDeviceContext, &WPoint, 1, &Size);

	PatBlt(GlobalFontDeviceContext, 0, 0, MAX_GLYPH_WIDTH, MAX_GLYPH_HEIGHT, BLACKNESS);
	SetTextColor(GlobalFontDeviceContext, RGB(255, 255, 255));
	int PreStepX = 128; 
	TextOutW(GlobalFontDeviceContext, PreStepX, 0, &WPoint, 1);

	i32 MinX = 10000;
	i32 MinY = 10000;
	i32 MaxX = -10000;
	i32 MaxY = -10000;
	u32 *Row = (u32 *)GlobalFontBits;
	for(i32 Y = 0;
		Y < MAX_GLYPH_HEIGHT;
		Y++)
	{
		u32 *Pixel = Row;
		for(i32 X = 0;
			X < MAX_GLYPH_WIDTH;
			X++)
		{
			if (*Pixel != 0)
			{
				if(MinX > X)
				{
					MinX = X;
				}
				if(MinY > Y)
				{
					MinY = Y;
				}
				if(MaxX < X)
				{
					MaxX = X;
				}
				if(MaxY < Y)
				{
					MaxY = Y;
				}
			}

			Pixel++;
		}

		Row += MAX_GLYPH_WIDTH;
	}

	u8* Result = 0;
	r32 KerningChange = 0.0f;
	if(MinX <= MaxX)
	{
		*Width = (MaxX + 1) - MinX;
		*Height = (MaxY + 1) - MinY;

		Result = (u8 *)malloc(*Width * *Height * 1);

		u8 *DestRow = Result;
		u32 *SourceRow = (u32 *)GlobalFontBits + MinY*MAX_GLYPH_WIDTH;
		for(i32 Y = MinY;
			Y <= MaxY;
			Y++)
		{
			u8 *Dest = (u8 *)DestRow;
			u32 *Source = SourceRow + MinX;
			for(i32 X = MinX;
				X <= MaxX;
				X++)
			{
				*Dest = *(u8 *)Source;
				Dest++;
				Source++;
			}
			
			DestRow += *Width*1;
			SourceRow += MAX_GLYPH_WIDTH;
		}

		*AlignPercentageY = ((MAX_GLYPH_HEIGHT - MinY) - (Size.cy - GlobalTextMetric.tmDescent)) / (r32)*Height;

		KerningChange = (r32)(MinX - PreStepX);
	}

	INT ThisCharWidth;
	GetCharWidth32W(GlobalFontDeviceContext, Codepoint, Codepoint, &ThisCharWidth);
	
	u32 ThisGlyphIndex = Codepoint - Font->FirstCodepoint;
	for(u32 OtherGlyphIndex = 0;
		OtherGlyphIndex < Font->GlyphsCount;
		OtherGlyphIndex++)
	{
		Font->HorizontalAdvances[ThisGlyphIndex*Font->GlyphsCount + OtherGlyphIndex] += ThisCharWidth - KerningChange;
		Font->HorizontalAdvances[OtherGlyphIndex*Font->GlyphsCount + ThisGlyphIndex] += KerningChange;
	}

	return(Result);
}

internal packer_loaded_font
LoadFont(char *Filename, char *FontName, u64 *AssetSize)
{
	packer_loaded_font Result = {};
	Result.FirstCodepoint = ' ';
	Result.LastCodepoint = '~';
	Result.GlyphsCount = (Result.LastCodepoint + 1) - Result.FirstCodepoint;

	Result.Glyphs = (packer_loaded_texture *)malloc(Result.GlyphsCount*sizeof(packer_loaded_texture));
	memset(Result.Glyphs, 0, Result.GlyphsCount*sizeof(packer_loaded_texture));

	BeginFont(&Result, Filename, FontName);

	for(u32 Character = Result.FirstCodepoint, GlyphIndex = 0;
		Character <= Result.LastCodepoint;
		Character++, GlyphIndex++)
	{
		packer_loaded_texture *Glyph = Result.Glyphs + GlyphIndex;
		Glyph->Data = LoadCodepointBitmap(&Result, Character, (u32 *)&Glyph->Width, (u32 *)&Glyph->Height, 
										  &Glyph->AlignPercentageY);
		Glyph->Free = Glyph->Data;
		Glyph->ChannelsCount = 1;

		*AssetSize += Glyph->Width*Glyph->Height*Glyph->ChannelsCount;
	}

	EndFont(&Result);

	return(Result);
}

struct asset_to_pack
{
	asset_data_type DataType;

	char *Filename;

	r32 AdditionalAlignmentY;
	r32 AdditionalAlignmentX;	
	r32 AdditionalAlignmentZ;	

	char *FontName;

	u32 FirstTagIndex;
	u32 OnePastLastTagIndex;
};

struct game_assets
{
	u32 TagCount;
	vea_asset_tag Tags[4096];

	u32 AssetCount;
	asset_to_pack AssetsToPack[4096];
	vea_asset Assets[4096];

	vea_asset_type AssetTypes[AssetType_Count];

	vea_asset_type *DEBUGAssetType; 
	asset_to_pack *DEBUGAsset;
};

internal void
BeginAssetType(game_assets *Assets, asset_type_id ID)
{
	Assert(!Assets->DEBUGAssetType);

	Assets->AssetTypes[ID].FirstAssetIndex = Assets->AssetCount;
	Assets->AssetTypes[ID].OnePastLastAssetIndex = Assets->AssetCount;

	Assets->DEBUGAssetType = &Assets->AssetTypes[ID];
}

internal asset_to_pack *
AddAsset(game_assets *Assets, char *Filename, asset_data_type DataType)
{
	Assert(Assets->DEBUGAssetType);

	asset_to_pack *Asset = Assets->AssetsToPack + Assets->AssetCount++;
	Asset->Filename = Filename;
	Asset->DataType = DataType;

	Asset->FirstTagIndex = Assets->TagCount;
	Asset->OnePastLastTagIndex = Assets->TagCount;

	Assets->DEBUGAsset = Asset;

	return(Asset);
}

inline void
AddModelAsset(game_assets *Assets, char *Filename, 
			  r32 AdditionalAlignmentY = 0.0f, r32 AdditionalAlignmentX = 0.0f, r32 AdditionalAlignmentZ = 0.0f)
{
	asset_to_pack *Asset = AddAsset(Assets, Filename, AssetDataType_Model);

	Asset->AdditionalAlignmentY = AdditionalAlignmentY;
	Asset->AdditionalAlignmentX = AdditionalAlignmentX;
	Asset->AdditionalAlignmentZ = AdditionalAlignmentZ;
}

inline void
AddTextureAsset(game_assets *Assets, char *Filename)
{
	asset_to_pack *Asset = AddAsset(Assets, Filename, AssetDataType_Texture);
}

inline void 
AddSoundAsset(game_assets *Assets, char *Filename)
{
	asset_to_pack *Asset = AddAsset(Assets, Filename, AssetDataType_Sound);
}

inline void
AddFontAsset(game_assets *Assets, char *Filename, char *FontName)
{
	asset_to_pack *Asset = AddAsset(Assets, Filename, AssetDataType_Font);
	
	Asset->FontName = FontName;
}

internal void
AddTag(game_assets *Assets, asset_tag_id ID, r32 Value)
{
	Assert(Assets->DEBUGAsset);

	vea_asset_tag *Tag = Assets->Tags + Assets->TagCount++;
	
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

int
main(int ArgCount, char **Args)
{
    game_assets GameAssets_ = {};
    game_assets *GameAssets = &GameAssets_;

	BeginAssetType(GameAssets, AssetType_Null);
	AddAsset(GameAssets, "", AssetDataType_Null);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Head);
	AddModelAsset(GameAssets, "data/models/head1.cub", 0.47f);
	AddTag(GameAssets, Tag_Color, 1.0f);
	AddModelAsset(GameAssets, "data/models/head2.cub", 0.47f);
	AddTag(GameAssets, Tag_Color, 10.0f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Shoulders);
	AddModelAsset(GameAssets, "data/models/shoulders.cub", 0.4f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Body);
	AddModelAsset(GameAssets, "data/models/body.cub", 0.08f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Hand);
	AddModelAsset(GameAssets, "data/models/hand.cub", 0.32f, 0.275f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Foot);
	AddModelAsset(GameAssets, "data/models/foot.cub", 0.0f, 0.15f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Sword);
	AddModelAsset(GameAssets, "data/models/sword.cub", 0.0f, 0.0f, -0.22f);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_UIBar);
	AddTextureAsset(GameAssets, "data/textures/hp_bar.bmp");
	AddTag(GameAssets, Tag_Color, TagColor_Red);
	AddTextureAsset(GameAssets, "data/textures/mp_bar.bmp");
	AddTag(GameAssets, Tag_Color, TagColor_Blue);
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Music);
	AddSoundAsset(GameAssets, "data/music/test1.wav");
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Fireball);
	AddSoundAsset(GameAssets, "data/sounds/Fireball.wav");
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_WaterSplash);
	AddSoundAsset(GameAssets, "data/sounds/water_splash.wav");
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Swing);
	AddSoundAsset(GameAssets, "data/sounds/swing.wav");
	EndAssetType(GameAssets);

	BeginAssetType(GameAssets, AssetType_Font);
	AddFontAsset(GameAssets, "C:/Windows/Fonts/Arial.ttf", "Arial");
	AddTag(GameAssets, Tag_FontType, (r32)FontType_DebugFont);
	AddFontAsset(GameAssets, "C:/Windows/Fonts/Comic Sans MS.ttf", "Comic Sans MS");
	AddTag(GameAssets, Tag_FontType, (r32)FontType_GameFont);
	EndAssetType(GameAssets);

    FILE *File = fopen("data.vea", "wb");
    if(File)
    {
        vea_header Header = {};
        Header.MagicValue = VEA_MAGIC_VALUE;
        Header.TagCount = GameAssets->TagCount;
        Header.AssetCount = GameAssets->AssetCount;

        u32 AssetTypeArraySize = AssetType_Count*sizeof(vea_asset_type);
        u32 TagArraySize = Header.TagCount*sizeof(vea_asset_tag);
        u32 AssetArraySize = Header.AssetCount*sizeof(vea_asset);

        Header.AssetTypes = sizeof(Header);
        Header.Tags = Header.AssetTypes + AssetTypeArraySize;
        Header.Assets = Header.Tags + TagArraySize;

        fwrite(&Header, sizeof(Header), 1, File);
        fwrite(GameAssets->AssetTypes, AssetTypeArraySize, 1, File);
        fwrite(GameAssets->Tags, TagArraySize, 1, File);
		fseek(File, AssetArraySize, SEEK_CUR);
		for(u32 AssetIndex = 1;
			AssetIndex < GameAssets->AssetCount;
			AssetIndex++)
		{
			asset_to_pack *AssetToPack = GameAssets->AssetsToPack + AssetIndex;
			vea_asset *Asset = GameAssets->Assets + AssetIndex;

			Asset->DataOffset = ftell(File);
			Asset->DataType = AssetToPack->DataType;
			Asset->FirstTagIndex = AssetToPack->FirstTagIndex;
			Asset->OnePastLastTagIndex = AssetToPack->OnePastLastTagIndex;

			switch(AssetToPack->DataType)
			{
				case AssetDataType_Model:
				{
					packer_loaded_model Model = LoadCUB(AssetToPack->Filename, 
												  		AssetToPack->AdditionalAlignmentY, AssetToPack->AdditionalAlignmentX, AssetToPack->AdditionalAlignmentZ);
					Asset->Model.VerticesCount = Model.VerticesCount;
					Asset->Model.Alignment = Model.Alignment;
					Asset->Model.AlignmentX = Model.AlignmentX;
					Asset->Model.AlignmentZ = Model.AlignmentZ;

					u32 VerticesSize = Model.VerticesCount*sizeof(vec3);
					fwrite(&Model.Positions->front(), VerticesSize, 1, File);					
					fwrite(&Model.Normals->front(), VerticesSize, 1, File);					
					fwrite(&Model.Colors->front(), VerticesSize, 1, File);	

					delete(Model.Positions);				
					delete(Model.Normals);				
					delete(Model.Colors);				
				} break;

				case AssetDataType_Texture:
				{
					packer_loaded_texture Texture = LoadBMP(AssetToPack->Filename);
					Asset->Texture.Width = Texture.Width;
					Asset->Texture.Height = Texture.Height;
					Asset->Texture.ChannelsCount = Texture.ChannelsCount;
					Asset->Texture.AlignPercentageY = Texture.AlignPercentageY;

					fwrite(Texture.Data, Texture.Width*Texture.Height*Texture.ChannelsCount, 1, File);

					free(Texture.Free);
				} break;

				case AssetDataType_Sound:
				{
					packer_loaded_sound Sound = LoadWAV(AssetToPack->Filename);
					Asset->Sound.SampleCount = Sound.SampleCount;
					Asset->Sound.ChannelsCount = Sound.ChannelsCount;

					for(u32 ChannelIndex = 0;
						ChannelIndex < Sound.ChannelsCount;
						ChannelIndex++)
					{
						fwrite(Sound.Samples[ChannelIndex], Sound.SampleCount*sizeof(i16), 1, File);
					}

					free(Sound.Free);
				} break;

				case AssetDataType_Font:
				{
					u64 AssetSize = 0;
					packer_loaded_font Font = LoadFont(AssetToPack->Filename, AssetToPack->FontName, &AssetSize);

					Asset->Font.LineAdvance = Font.LineAdvance;
					Asset->Font.AscenderHeight = Font.AscenderHeight;
					Asset->Font.GlyphsCount = Font.GlyphsCount;
					Asset->Font.FirstCodepoint = Font.FirstCodepoint;
					Asset->Font.LastCodepoint = Font.LastCodepoint;

					fwrite(Font.Glyphs, Font.GlyphsCount*sizeof(packer_loaded_texture), 1, File);
					fwrite(Font.HorizontalAdvances, Font.GlyphsCount*Font.GlyphsCount*sizeof(r32), 1, File);
					for(u32 GlyphIndex = 0;
						GlyphIndex < Font.GlyphsCount;
						GlyphIndex++)
					{
						packer_loaded_texture *Glyph = Font.Glyphs + GlyphIndex;

						fwrite(Glyph->Data, Glyph->Width*Glyph->Height*1, 1, File);
						free(Glyph->Free);
					}

					free(Font.Glyphs);
					free(Font.HorizontalAdvances);
				} break;
			}
		}
		fseek(File, (u32)Header.Assets, SEEK_SET);
		fwrite(GameAssets->Assets, AssetArraySize, 1, File);

        fclose(File);
    }
    else
    {
        printf("ERROR: Can't open the file!\n");
    }

    return(0);
}