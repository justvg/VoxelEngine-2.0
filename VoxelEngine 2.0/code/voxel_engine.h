#pragma once

global_variable read_entire_file *PlatformReadEntireFile;
global_variable free_file_memory *PlatformFreeFileMemory;
global_variable allocate_memory *PlatformAllocateMemory;
global_variable free_memory *PlatformFreeMemory;
global_variable output_debug_string *PlatformOutputDebugString;

struct stack_allocator
{
	u64 Size;
	u8 *Base;
	u64 Used;
};

struct temporary_memory
{
	stack_allocator *Allocator;
	u64 Used;
};

inline void
InitializeStackAllocator(stack_allocator *Allocator, u64 Size, void *Base)
{
	Allocator->Size = Size;
	Allocator->Base = (u8 *)Base;
	Allocator->Used = 0;
}

#define PushStruct(Allocator, type) (type *)PushSize(Allocator, sizeof(type))
#define PushArray(Allocator, Count, type) (type *)PushSize(Allocator, (Count)*sizeof(type))
inline void *
PushSize(stack_allocator *Allocator, u64 Size)
{
	Assert((Allocator->Used + Size) <= Allocator->Size);
	void *Result = Allocator->Base + Allocator->Used;
	Allocator->Used += Size;

	return(Result);
}

inline temporary_memory
BeginTemporaryMemory(stack_allocator *Allocator)
{
	temporary_memory Result;
	Result.Allocator = Allocator;
	Result.Used = Allocator->Used;

	return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMemory)
{
	stack_allocator *Allocator = TempMemory.Allocator;
	Assert(Allocator->Used >= TempMemory.Used);
	Allocator->Used = TempMemory.Used;
}

#include "voxel_engine_math.h"

struct camera
{
	r32 DistanceFromHero;
	r32 Pitch, Head;

	r32 RotSensetivity;

	vec3 OffsetFromHero;
};

struct dynamic_array_vec3
{
	u32 MaxEntriesCount;
	u32 EntriesCount;
	vec3 *Entries;
};
#define INITIAL_MAX_ENTRIES_COUNT 64
inline void
ExpandDynamicArray(dynamic_array_vec3 *Array)
{
	Assert(!(Array->EntriesCount < Array->MaxEntriesCount));

	u32 NewMaxEntriesCount = Array->MaxEntriesCount ? Array->MaxEntriesCount * 2 : INITIAL_MAX_ENTRIES_COUNT;
	vec3 *NewMemory = (vec3 *)PlatformAllocateMemory(NewMaxEntriesCount * sizeof(vec3));
	Assert(NewMemory);

	for (u32 EntryIndex = 0;
		EntryIndex < Array->MaxEntriesCount;
		EntryIndex++)
	{
		NewMemory[EntryIndex] = Array->Entries[EntryIndex];
	}

	PlatformFreeMemory(Array->Entries);
	Array->MaxEntriesCount = NewMaxEntriesCount;
	Array->Entries = NewMemory;
}

inline void
InitializeDynamicArray(dynamic_array_vec3 *Array, u32 InitialMaxEntriesCount = INITIAL_MAX_ENTRIES_COUNT)
{
	Array->MaxEntriesCount = InitialMaxEntriesCount;
	Array->EntriesCount = 0;
	Array->Entries = (vec3 *)PlatformAllocateMemory(Array->MaxEntriesCount * sizeof(vec3));
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

internal void
AddQuad(dynamic_array_vec3 *Array, vec3 A, vec3 B, vec3 C, vec3 D)
{
	PushEntry(Array, A);
	PushEntry(Array, B);
	PushEntry(Array, C);
	PushEntry(Array, C);
	PushEntry(Array, B);
	PushEntry(Array, D);
}

struct shader
{
	u32 ID;
};
internal void
CompileShader(shader *Shader, char *VertexPath, char *FragmentPath)
{
	u32 VS = glCreateShader(GL_VERTEX_SHADER);
	u32 FS = glCreateShader(GL_FRAGMENT_SHADER);
	Shader->ID = glCreateProgram();

	read_entire_file_result VSSourceCode = PlatformReadEntireFile(VertexPath);
	read_entire_file_result FSSourceCode = PlatformReadEntireFile(FragmentPath);

	i32 Success;
	char InfoLog[1024];

	glShaderSource(VS, 1, (char **)&VSSourceCode.Memory, (GLint *)&VSSourceCode.Size);
	glCompileShader(VS);
	glGetShaderiv(VS, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(VS, sizeof(InfoLog), 0, InfoLog);
		PlatformOutputDebugString("ERROR::SHADER_COMPILATION_ERROR of type: VS\n");
		PlatformOutputDebugString(InfoLog);
		PlatformOutputDebugString("\n");
	}

	glShaderSource(FS, 1, (char **)&FSSourceCode.Memory, (GLint *)&FSSourceCode.Size);
	glCompileShader(FS);
	glGetShaderiv(FS, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(FS, sizeof(InfoLog), 0, InfoLog);
		PlatformOutputDebugString("ERROR::SHADER_COMPILATION_ERROR of type: FS\n");
		PlatformOutputDebugString(InfoLog);
		PlatformOutputDebugString("\n");
	}

	glAttachShader(Shader->ID, VS);
	glAttachShader(Shader->ID, FS);
	glLinkProgram(Shader->ID);
	glGetProgramiv(Shader->ID, GL_LINK_STATUS, &Success);
	if (!Success)
	{
		glGetProgramInfoLog(Shader->ID, sizeof(InfoLog), 0, InfoLog);
		PlatformOutputDebugString("ERROR::PROGRAM_LINKING_ERROR of type:: PROGRAM\n");
		PlatformOutputDebugString(InfoLog);
		PlatformOutputDebugString("\n");
	}

	PlatformFreeFileMemory(VSSourceCode.Memory);
	PlatformFreeFileMemory(FSSourceCode.Memory);
	glDeleteShader(VS);
	glDeleteShader(FS);
}

inline void
UseShader(shader Shader)
{
	glUseProgram(Shader.ID);
}

inline void
SetFloat(shader Shader, char *Name, real32 Value)
{
	glUniform1f(glGetUniformLocation(Shader.ID, Name), Value);
}

inline void
SetInt(shader Shader, char *Name, int32_t Value)
{
	glUniform1i(glGetUniformLocation(Shader.ID, Name), Value);
}

inline void
SetVec3(shader Shader, char *Name, vec3 Value)
{
	glUniform3fv(glGetUniformLocation(Shader.ID, Name), 1, (GLfloat *)&Value.m);
}

inline void
SetVec4(shader Shader, char *Name, vec4 Value)
{
	glUniform4fv(glGetUniformLocation(Shader.ID, Name), 1, (GLfloat *)&Value.m);
}

inline void
SetMat4(shader Shader, char *Name, mat4 Value)
{
	glUniformMatrix4fv(glGetUniformLocation(Shader.ID, Name), 1, GL_FALSE, (GLfloat *)&Value.FirstColumn);
}

#include "voxel_engine_world.h"
#include "voxel_engine_sim_region.h"

struct stored_entity
{
	world_position P;
	sim_entity Sim;
};

struct game_state
{
	bool32 IsInitialized;

	camera Camera;

	stack_allocator WorldAllocator;
	world World;	

	shader DefaultShader;

	stored_entity *Hero;
	world_position CubeP;
	r32 CubeAdditionalRotation;
	GLuint CubeVAO, CubeVBO;

	u32 StoredEntityCount;
	stored_entity StoredEntities[10000];
};

struct temp_state
{
	bool32 IsInitialized;

	stack_allocator Allocator;
};