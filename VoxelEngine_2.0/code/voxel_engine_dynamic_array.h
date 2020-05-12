#pragma once

// TODO(georgy): Can I compress this? 

#define INITIAL_MAX_ENTRIES_COUNT 64

struct dynamic_array_vec4
{
	u32 MaxEntriesCount;
	u32 EntriesCount;
	vec4 *Entries;
};

struct dynamic_array_vec3
{
	u32 MaxEntriesCount;
	u32 EntriesCount;
	vec3 *Entries;
};

struct dynamic_array_u32
{
	u32 MaxEntriesCount;
	u32 EntriesCount;
	u32 *Entries;
};

struct i8_normal
{
	i8 X, Y, Z;
};
struct dynamic_array_i8_normal
{
	u32 MaxEntriesCount;
	u32 EntriesCount;
	i8_normal *Entries;
};

struct u8_color
{
	u8 R, G, B, A;
};
struct dynamic_array_u8_color
{
	u32 MaxEntriesCount;
	u32 EntriesCount;
	u8_color *Entries;
};

#define INITIALIZE_DYNAMIC_ARRAY(Array, InitialMaxEntriesCount, type) \
	Array->MaxEntriesCount = InitialMaxEntriesCount; \
	Array->EntriesCount = 0; \
	Array->Entries = (type *)Platform.AllocateMemory(Array->MaxEntriesCount * sizeof(type)); \
	Assert(Array->Entries); \
	Assert(((u64)Array->Entries & 15) == 0); 

#define EXPAND_DYNAMIC_ARRAY(Array, type) \
	Assert(!(Array->EntriesCount < Array->MaxEntriesCount)); \
	\
	u32 NewMaxEntriesCount = Array->MaxEntriesCount ? Array->MaxEntriesCount * 2 : INITIAL_MAX_ENTRIES_COUNT; \
	type *NewMemory = (type *)Platform.AllocateMemory(NewMaxEntriesCount * sizeof(type)); \
	Assert(NewMemory); \
	Assert(((u64)NewMemory & 15) == 0); \
	\
	for(u32 EntryIndex = 0; \
		EntryIndex < Array->MaxEntriesCount; \
		EntryIndex++) \
	{ \
		NewMemory[EntryIndex] = Array->Entries[EntryIndex]; \
	} \
	\
	Platform.FreeMemory(Array->Entries); \
	Array->MaxEntriesCount = NewMaxEntriesCount; \
	Array->Entries = NewMemory; 

#define PUSH_ENTRY(Array, Entry) \
	if(!(Array->EntriesCount < Array->MaxEntriesCount)) \
	{ \
		ExpandDynamicArray(Array); \
	} \
	\
	Array->Entries[Array->EntriesCount++] = Entry;

#define FREE_DYNAMIC_ARRAY(Array) \
	Platform.FreeMemory(Array->Entries); \
	Array->MaxEntriesCount = 0; \
	Array->Entries = 0; \
	Array->EntriesCount = 0; 


#define ADD_TRIANGLE(Array, A, B, C) \
	PushEntry(Array, A); \
	PushEntry(Array, B); \
	PushEntry(Array, C); 

// 
// NOTE(georgy): vec3
// 

inline void
InitializeDynamicArray(dynamic_array_vec3 *Array, u32 InitialMaxEntriesCount = INITIAL_MAX_ENTRIES_COUNT)
{
	INITIALIZE_DYNAMIC_ARRAY(Array, InitialMaxEntriesCount, vec3);
}

internal void
ExpandDynamicArray(dynamic_array_vec3 *Array)
{
	EXPAND_DYNAMIC_ARRAY(Array, vec3);
}

internal void
PushEntry(dynamic_array_vec3 *Array, vec3 Entry)
{
	PUSH_ENTRY(Array, Entry);
}

inline void
FreeDynamicArray(dynamic_array_vec3 *Array)
{
	FREE_DYNAMIC_ARRAY(Array);
}

inline void
AddTriangle(dynamic_array_vec3 *Array, vec3 A, vec3 B, vec3 C)
{
	ADD_TRIANGLE(Array, A, B, C);
}

inline void
AddQuad(dynamic_array_vec3 *Array, vec3 A, vec3 B, vec3 C, vec3 D)
{
	AddTriangle(Array, A, B, C);
	AddTriangle(Array, C, B, D);
}

// 
// NOTE(georgy): vec4
// 

inline void
InitializeDynamicArray(dynamic_array_vec4 *Array, u32 InitialMaxEntriesCount = INITIAL_MAX_ENTRIES_COUNT)
{
	INITIALIZE_DYNAMIC_ARRAY(Array, InitialMaxEntriesCount, vec4);
}

internal void
ExpandDynamicArray(dynamic_array_vec4 *Array)
{
	EXPAND_DYNAMIC_ARRAY(Array, vec4);
}

internal void
PushEntry(dynamic_array_vec4 *Array, vec4 Entry)
{
	PUSH_ENTRY(Array, Entry);
}

inline void
FreeDynamicArray(dynamic_array_vec4 *Array)
{
	FREE_DYNAMIC_ARRAY(Array);
}

inline void
AddTriangle(dynamic_array_vec4 *Array, vec4 A, vec4 B, vec4 C)
{
	ADD_TRIANGLE(Array, A, B, C);
}

inline void
AddQuad(dynamic_array_vec4 *Array, vec4 A, vec4 B, vec4 C, vec4 D)
{
	AddTriangle(Array, A, B, C);
	AddTriangle(Array, C, B, D);
}

// 
// NOTE(georgy): u32
// 

inline void
InitializeDynamicArray(dynamic_array_u32 *Array, u32 InitialMaxEntriesCount = INITIAL_MAX_ENTRIES_COUNT)
{
	INITIALIZE_DYNAMIC_ARRAY(Array, InitialMaxEntriesCount, u32);
}

internal void
ExpandDynamicArray(dynamic_array_u32 *Array)
{
	EXPAND_DYNAMIC_ARRAY(Array, u32);
}

internal void
PushEntry(dynamic_array_u32 *Array, u32 Entry)
{
	PUSH_ENTRY(Array, Entry);
}

inline void
FreeDynamicArray(dynamic_array_u32 *Array)
{
	FREE_DYNAMIC_ARRAY(Array);
}

inline void
AddTriangle(dynamic_array_u32 *Array, u32 A, u32 B, u32 C)
{
	ADD_TRIANGLE(Array, A, B, C);
}

inline void
AddQuad(dynamic_array_u32 *Array, u32 A, u32 B, u32 C, u32 D)
{
	AddTriangle(Array, A, B, C);
	AddTriangle(Array, C, B, D);
}

// 
// NOTE(georgy): i8_normal
// 

inline void
InitializeDynamicArray(dynamic_array_i8_normal *Array, u32 InitialMaxEntriesCount = INITIAL_MAX_ENTRIES_COUNT)
{
	INITIALIZE_DYNAMIC_ARRAY(Array, InitialMaxEntriesCount, i8_normal);
}

internal void
ExpandDynamicArray(dynamic_array_i8_normal *Array)
{
	EXPAND_DYNAMIC_ARRAY(Array, i8_normal);
}

internal void
PushEntry(dynamic_array_i8_normal *Array, i8_normal Entry)
{
	PUSH_ENTRY(Array, Entry);
}

inline void
FreeDynamicArray(dynamic_array_i8_normal *Array)
{
	FREE_DYNAMIC_ARRAY(Array);
}

inline void
AddTriangle(dynamic_array_i8_normal *Array, i8_normal A, i8_normal B, i8_normal C)
{
	ADD_TRIANGLE(Array, A, B, C);
}

inline void
AddQuad(dynamic_array_i8_normal *Array, i8_normal A, i8_normal B, i8_normal C, i8_normal D)
{
	AddTriangle(Array, A, B, C);
	AddTriangle(Array, C, B, D);
}

// 
// NOTE(georgy): u8_color
// 

inline void
InitializeDynamicArray(dynamic_array_u8_color *Array, u32 InitialMaxEntriesCount = INITIAL_MAX_ENTRIES_COUNT)
{
	INITIALIZE_DYNAMIC_ARRAY(Array, InitialMaxEntriesCount, u8_color);
}

internal void
ExpandDynamicArray(dynamic_array_u8_color *Array)
{
	EXPAND_DYNAMIC_ARRAY(Array, u8_color);
}

internal void
PushEntry(dynamic_array_u8_color *Array, u8_color Entry)
{
	PUSH_ENTRY(Array, Entry);
}

inline void
FreeDynamicArray(dynamic_array_u8_color *Array)
{
	FREE_DYNAMIC_ARRAY(Array);
}

inline void
AddTriangle(dynamic_array_u8_color *Array, u8_color A, u8_color B, u8_color C)
{
	ADD_TRIANGLE(Array, A, B, C);
}

inline void
AddQuad(dynamic_array_u8_color *Array, u8_color A, u8_color B, u8_color C, u8_color D)
{
	AddTriangle(Array, A, B, C);
	AddTriangle(Array, C, B, D);
}