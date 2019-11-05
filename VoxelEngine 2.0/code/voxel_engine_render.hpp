#include "voxel_engine_render.h"

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
SetFloat(shader Shader, char *Name, r32 Value)
{
	glUniform1f(glGetUniformLocation(Shader.ID, Name), Value);
}

inline void
SetInt(shader Shader, char *Name, int32_t Value)
{
	glUniform1i(glGetUniformLocation(Shader.ID, Name), Value);
}

inline void
SetVec2(shader Shader, char *Name, vec2 Value)
{
	glUniform2f(glGetUniformLocation(Shader.ID, Name), Value.x, Value.y);
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


inline void
Initialize3DTransforms(shader *Shaders, u32 ShaderCount, mat4 ViewProjection)
{
	for(u32 ShaderIndex = 0;
		ShaderIndex < ShaderCount;
		ShaderIndex++)
	{
		UseShader(Shaders[ShaderIndex]);	
		SetMat4(Shaders[ShaderIndex], "ViewProjection", ViewProjection);
	}
}

inline void
DrawFromVAO(GLuint VAO, u32 VerticesCount)
{
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, VerticesCount);
	glBindVertexArray(0);
}

internal void
DrawModel(shader Shader, game_assets *GameAssets, u32 AssetIndex, r32 Rotation, r32 Scaling, vec3 Right)
{
	loaded_model *Model = GetModel(GameAssets, AssetIndex);
	if(Model)
	{
		mat4 ModelMatrix = Translate(Model->Alignment + Model->AlignmentX*Right) * 
						   Rotate(Rotation, vec3(0.0f, 1.0f, 0.0f)) * Scale(Scaling);
		SetMat4(Shader, "Model", ModelMatrix);
		DrawFromVAO(Model->VAO, Model->VerticesCount);
	}
	else
	{
		LoadModel(GameAssets, AssetIndex);
	}
}