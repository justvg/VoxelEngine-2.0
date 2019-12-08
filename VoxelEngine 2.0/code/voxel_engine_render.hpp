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
SetFloatArray(shader Shader, char *Name, u32 Count, r32 *Values)
{
	glUniform1fv(glGetUniformLocation(Shader.ID, Name), Count, Values);
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
SetMat4Array(shader Shader, char *Name, u32 Count, mat4 *Values)
{
	glUniformMatrix4fv(glGetUniformLocation(Shader.ID, Name), Count, GL_FALSE, (GLfloat *)Values);
}

inline void
SetMat4(shader Shader, char *Name, mat4 Value)
{
	SetMat4Array(Shader, Name, 1, &Value);
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
DrawModel(shader Shader, game_assets *GameAssets, model_id ModelIndex, r32 Rotation = 0.0f, 
		  r32 Scaling = 1.0f, vec3 Right = vec3(0.0f, 0.0f, 0.0f), vec3 Translation = vec3(0.0f, 0.0f, 0.0f))
{
	loaded_model *Model = GetModel(GameAssets, ModelIndex);
	if(Model)
	{
		mat4 ModelMatrix = Translate(Model->Alignment + Model->AlignmentX*Right + Translation) * 
						   Rotate(Rotation, vec3(0.0f, 1.0f, 0.0f)) * Scale(Scaling);
		SetMat4(Shader, "Model", ModelMatrix);
		DrawFromVAO(Model->VAO, Model->VerticesCount);
	}
	else
	{
		LoadModel(GameAssets, ModelIndex);
	}
}


internal void
UpdateParticles(particle_emitter *Particles, camera *Camera, vec3 BaseP, r32 dt)
{
	for(u32 ParticleIndex = 0;
		ParticleIndex < ArrayCount(Particles->Particles);
		ParticleIndex++)
	{
		particle *Particle = Particles->Particles + ParticleIndex;
		if(Particle->LifeTime > 0.0f)
		{
			Particle->P += 0.5f*Particle->ddP*dt*dt + Particle->dP*dt;
			Particle->dP += Particle->ddP*dt;
			Particle->LifeTime -= dt;
			vec3 VecToCamera = (BaseP + Particle->P) - 
							   (Camera->OffsetFromHero + Camera->TargetOffset);
			VecToCamera.SetY(0.0f);
			Particle->DistanceFromCameraSq = LengthSq(VecToCamera);
		}
	}
}

internal void
SpawnParticles(particle_emitter *Particles, camera *Camera, vec3 BaseP, r32 dt)
{
	u32 ParticlesToSpawnCount = (u32)(Particles->Info.SpawnInSecond * dt);
	for(u32 ParticleSpawnIndex = 0;
		ParticleSpawnIndex < ParticlesToSpawnCount;
		ParticleSpawnIndex++)
	{
		particle *Particle = Particles->Particles + Particles->NextParticle++;
		if(Particles->NextParticle >= ArrayCount(Particles->Particles))
		{
			Particles->NextParticle = 0;
		}

		// Particle->P = vec3(RandomBetween(-Particles->Info.StartPRanges.x(), Particles->Info.StartPRanges.x()),
		//                    RandomBetween(-Particles->Info.StartPRanges.y(), Particles->Info.StartPRanges.y()),
		// 				      RandomBetween(-Particles->Info.StartPRanges.z(), Particles->Info.StartPRanges.z()));
		Particle->P = Hadamard(vec3((((rand() % 100) - 50) / 50.0f), (((rand() % 100) - 50) / 50.0f), (((rand() % 100) - 50) / 50.0f)),
							   Particles->Info.StartPRanges);
		Particle->dP = Hadamard(vec3((((rand() % 100) - 50) / 50.0f), (((rand() % 100) - 50) / 50.0f), (((rand() % 100) - 50) / 50.0f)),
							   Particles->Info.StartdPRanges);
		if(Particles->Info.dPY != 0.0f)
		{
			Particle->dP.SetY(Particles->Info.dPY);
		}
		Particle->ddP = Particles->Info.StartddP;
		Particle->Scale = Particles->Info.Scale;
		Particle->LifeTime = Particles->Info.MaxLifeTime;
		vec3 VecToCamera = (BaseP + Particle->P) - 
   						   (Camera->OffsetFromHero + Camera->TargetOffset);
		VecToCamera.SetY(0.0f);
		Particle->DistanceFromCameraSq = LengthSq(VecToCamera);
	}
}

internal void
SortParticles(particle *Particles, u32 ParticlesCount)
{
	for(u32 StartIndex = 0;
		StartIndex < ParticlesCount - 1;
		StartIndex++)
	{
		u32 MaxDistanceIndex = StartIndex;
		for(u32 ParticleIndex = StartIndex + 1;
			ParticleIndex < ParticlesCount;
			ParticleIndex++)
		{
			if(Particles[MaxDistanceIndex].DistanceFromCameraSq < Particles[ParticleIndex].DistanceFromCameraSq)
			{
				MaxDistanceIndex = ParticleIndex;
			}
		}

		particle Temp = Particles[StartIndex];
		Particles[StartIndex] = Particles[MaxDistanceIndex];
		Particles[MaxDistanceIndex] = Temp;
	}
}


internal void
AddBlockParticle(block_particle_generator *Generator, world_position BaseP, vec3 Color)
{
	block_particle *BlockParticle = Generator->Particles + Generator->NextParticle++;
	if(Generator->NextParticle >= ArrayCount(Generator->Particles))
	{
		Generator->NextParticle = 0;
	}
	BlockParticle->BaseP = BaseP;
	BlockParticle->P = vec3((((rand() % 100) - 50) / 50.0f), (((rand() % 100) - 50) / 150.0f), (((rand() % 100) - 50) / 50.0f));
	BlockParticle->dP = vec3((((rand() % 100) - 50) / 50.0f), 6.0f, (((rand() % 100) - 50) / 50.0f));
	BlockParticle->ddP = vec3(0.0f, -9.8f, 0.0f);

	BlockParticle->Color = Color;

	BlockParticle->LifeTime = 1.5f;
}

internal void
BlockParticlesUpdate(block_particle_generator *Generator, r32 dt)
{
	for(u32 BlockParticleIndex = 0;
		BlockParticleIndex < ArrayCount(Generator->Particles);
		BlockParticleIndex++)
	{
		block_particle *BlockParticle = Generator->Particles + BlockParticleIndex;
		if(BlockParticle->LifeTime > 0.0f)
		{
			BlockParticle->P += 0.5f*BlockParticle->ddP*dt*dt + BlockParticle->dP*dt;
			BlockParticle->dP += BlockParticle->ddP*dt;
			BlockParticle->LifeTime -= dt;
		}
	}
}

internal void
RenderBlockParticles(block_particle_generator *Generator, world *World, shader Shader, world_position Origin)
{
	u32 BlockParticlesCount = 0;
	vec3 BlockParticlesPositions[MAX_BLOCK_PARTICLES_COUNT];
	vec3 BlockParticlesColors[MAX_BLOCK_PARTICLES_COUNT];

	for(u32 BlockParticleIndex = 0;
		BlockParticleIndex < ArrayCount(Generator->Particles);
		BlockParticleIndex++)
	{
		block_particle *BlockParticle = Generator->Particles + BlockParticleIndex;
		if(BlockParticle->LifeTime > 0.0f)
		{
			BlockParticlesPositions[BlockParticlesCount] = Substract(World, &BlockParticle->BaseP, &Origin) + BlockParticle->P;
			BlockParticlesColors[BlockParticlesCount] = BlockParticle->Color;
			BlockParticlesCount++;
		}
	}

	UseShader(Shader);
	glBindVertexArray(Generator->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, Generator->SimPVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*BlockParticlesCount, BlockParticlesPositions, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);

	glBindBuffer(GL_ARRAY_BUFFER, Generator->ColorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*BlockParticlesCount, BlockParticlesColors, GL_STATIC_DRAW);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);

	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);

	glDrawArraysInstanced(GL_TRIANGLES, 0, 36, BlockParticlesCount);

	glBindVertexArray(0);
}