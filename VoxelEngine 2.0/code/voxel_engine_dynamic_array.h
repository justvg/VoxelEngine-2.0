#pragma once

#define INITIAL_MAX_ENTRIES_COUNT 64

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

inline void
InitializeDynamicArray(dynamic_array_vec3 *Array, u32 InitialMaxEntriesCount = INITIAL_MAX_ENTRIES_COUNT)
{
	Array->MaxEntriesCount = InitialMaxEntriesCount;
	Array->EntriesCount = 0;
	Array->Entries = (vec3 *)PlatformAllocateMemory(Array->MaxEntriesCount * sizeof(vec3));
	Assert(Array->Entries);
	Assert(((u64)Array->Entries & 15) == 0);
}

internal void
ExpandDynamicArray(dynamic_array_vec3 *Array)
{
	Assert(!(Array->EntriesCount < Array->MaxEntriesCount));

	u32 NewMaxEntriesCount = Array->MaxEntriesCount ? Array->MaxEntriesCount * 2 : INITIAL_MAX_ENTRIES_COUNT;
	vec3 *NewMemory = (vec3 *)PlatformAllocateMemory(NewMaxEntriesCount * sizeof(vec3));
	Assert(NewMemory);
	Assert(((u64)NewMemory & 15) == 0);

	for(u32 EntryIndex = 0;
		EntryIndex < Array->MaxEntriesCount;
		EntryIndex++)
	{
		NewMemory[EntryIndex] = Array->Entries[EntryIndex];
	}

	PlatformFreeMemory(Array->Entries);
	Array->MaxEntriesCount = NewMaxEntriesCount;
	Array->Entries = NewMemory;
}

internal void
PushEntry(dynamic_array_vec3 *Array, vec3 Entry)
{
	if (!(Array->EntriesCount < Array->MaxEntriesCount))
	{
		ExpandDynamicArray(Array);
	}

	Array->Entries[Array->EntriesCount++] = Entry;
}

inline void
FreeDynamicArray(dynamic_array_vec3 *Array)
{
	PlatformFreeMemory(Array->Entries);
	Array->MaxEntriesCount = 0;
	Array->Entries = 0;
	Array->EntriesCount = 0;
}

inline void
AddTriangle(dynamic_array_vec3 *Array, vec3 A, vec3 B, vec3 C)
{
	PushEntry(Array, A);
	PushEntry(Array, B);
	PushEntry(Array, C);
}

inline void
AddQuad(dynamic_array_vec3 *Array, vec3 A, vec3 B, vec3 C, vec3 D)
{
	AddTriangle(Array, A, B, C);
	AddTriangle(Array, C, B, D);
}


inline void
InitializeDynamicArray(dynamic_array_u32 *Array, u32 InitialMaxEntriesCount = INITIAL_MAX_ENTRIES_COUNT)
{
	Array->MaxEntriesCount = InitialMaxEntriesCount;
	Array->EntriesCount = 0;
	Array->Entries = (u32 *)PlatformAllocateMemory(Array->MaxEntriesCount * sizeof(u32));
	Assert(Array->Entries);
	Assert(((u64)Array->Entries & 15) == 0);
}

internal void
ExpandDynamicArray(dynamic_array_u32 *Array)
{
	Assert(!(Array->EntriesCount < Array->MaxEntriesCount));

	u32 NewMaxEntriesCount = Array->MaxEntriesCount ? Array->MaxEntriesCount * 2 : INITIAL_MAX_ENTRIES_COUNT;
	u32 *NewMemory = (u32 *)PlatformAllocateMemory(NewMaxEntriesCount * sizeof(u32));
	Assert(NewMemory);
	Assert(((u64)NewMemory & 15) == 0);

	for(u32 EntryIndex = 0;
		EntryIndex < Array->MaxEntriesCount;
		EntryIndex++)
	{
		NewMemory[EntryIndex] = Array->Entries[EntryIndex];
	}

	PlatformFreeMemory(Array->Entries);
	Array->MaxEntriesCount = NewMaxEntriesCount;
	Array->Entries = NewMemory;
}

internal void
PushEntry(dynamic_array_u32 *Array, u32 Entry)
{
	if (!(Array->EntriesCount < Array->MaxEntriesCount))
	{
		ExpandDynamicArray(Array);
	}

	Array->Entries[Array->EntriesCount++] = Entry;
}

inline void
FreeDynamicArray(dynamic_array_u32 *Array)
{
	PlatformFreeMemory(Array->Entries);
	Array->MaxEntriesCount = 0;
	Array->Entries = 0;
	Array->EntriesCount = 0;
}

inline void
AddTriangle(dynamic_array_u32 *Array, u32 A, u32 B, u32 C)
{
	PushEntry(Array, A);
	PushEntry(Array, B);
	PushEntry(Array, C);
}

inline void
AddQuad(dynamic_array_u32 *Array, u32 A, u32 B, u32 C, u32 D)
{
	AddTriangle(Array, A, B, C);
	AddTriangle(Array, C, B, D);
}