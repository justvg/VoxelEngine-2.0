#pragma once

#pragma pack(push, 1)
struct vea_header
{
#define VEA_MAGIC_VALUE (((u32)('v') << 0) | ((u32)('e') << 8) | ((u32)('a') << 16) | ((u32)('f') << 24))
   u32 MagicValue;

   u32 AssetCount;
   u32 TagCount;

   u64 AssetTypes;
   u64 Tags;
   u64 Assets;
};
#pragma pack(pop)

enum asset_type_id
{
	AssetType_Null,
	
	// 
	// NOTE(georgy): Models! 
	// 

	AssetType_Head,
	AssetType_Shoulders,
	AssetType_Body,
	AssetType_Hand,
	AssetType_Foot,

	AssetType_Tree,

	// 
	// NOTE(georgy): Textures! 
	// 

	AssetType_UIBar,

	// 
	// NOTE(georgy): Sounds!
	// 

	AssetType_Music,
	AssetType_WaterSplash,
	AssetType_Fireball,

	// 
	// NOTE(georgy): Fonts!
	// 

	AssetType_Font,
	AssetType_FontGlyph,

	AssetType_Count
};

enum asset_data_type
{
	AssetDataType_Null,

	AssetDataType_Model,
	AssetDataType_Texture,
	AssetDataType_Sound,
	AssetDataType_Font,
};

enum tag_color
{
	TagColor_Red = 1,
	TagColor_Green = 2,
	TagColor_Blue = 3,
};
enum asset_font_type
{
	FontType_DebugFont = 1,
	FontType_GameFont = 5,
};
enum asset_tag_id
{
	Tag_Color,
	Tag_FontType,

	Tag_Count
};

struct vea_asset_tag
{
	asset_tag_id ID;
	r32 Value;
};

struct vea_asset_type
{
    u32 FirstAssetIndex;
	u32 OnePastLastAssetIndex;
};

struct vea_model
{
	u32 VerticesCount;

	vec3 Alignment;
	r32 AlignmentX; // NOTE(georgy): This is for models like hands, where we need to multiply this by EntityRight vec

	/* NOTE(georgy): Data is:
			vec3 Positions[VerticesCount];
			vec3 Normals[VerticesCount];
			vec3 Colors[VerticesCount];
	*/
};

struct vea_texture
{
	i32 Width, Height, ChannelsCount;

	// NOTE(georgy): For glyphs (to count descent)
	r32 AlignPercentageY;

	/* NOTE(georgy): Data is:
			uint8*ChannelsCount Pixels[Width*Height]
	*/
};

struct vea_sound
{
    u32 SampleCount;
	u32 ChannelsCount;

	/* NOTE(georgy): Data is:
			i16 Samples[ChannelCount][SampleCount]
	*/
};

struct vea_font
{
	u32 LineAdvance;
	u32 AscenderHeight;

	u32 GlyphsCount;
	u32 FirstCodepoint;
	u32 LastCodepoint;

	/* NOTE(georgy): Data is:
			packer_loaded_texture GlyphsInAssetFile[GlyphsCount]
			r32 HorizontalAdvances[GlyphsCount][GlyphsCount]
			uint8 Pixels[GlyphCount][Width*Height]
	*/
};

struct vea_asset
{
	vea_asset() {}

    u64 DataOffset;
	asset_data_type DataType;

    u32 FirstTagIndex;
	u32 OnePastLastTagIndex;
    union
    {
        vea_model Model;
        vea_texture Texture;
        vea_sound Sound;
        vea_font Font;
    };
};


struct packer_loaded_texture
{
	i32 Width, Height, ChannelsCount;

	// NOTE(georgy): For glyphs (to count descent)
	r32 AlignPercentageY;

	u8 *Data;
	void *Free;
};

struct packer_loaded_font
{
	packer_loaded_texture *Glyphs;
	r32 *HorizontalAdvances;
	
	u32 LineAdvance;
	u32 AscenderHeight;

	u32 GlyphsCount;
	u32 FirstCodepoint;
	u32 LastCodepoint;
};
