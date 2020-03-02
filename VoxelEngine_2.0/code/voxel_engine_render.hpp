#include "voxel_engine_render.h"

internal void
CompileShader(shader *Shader, char *VSSourceCode, char *FSSourceCode)
{
	GLuint VS = glCreateShader(GL_VERTEX_SHADER);
	GLuint FS = glCreateShader(GL_FRAGMENT_SHADER);
	Shader->ID = glCreateProgram();

	i32 Success;
	char InfoLog[1024];
	
	glShaderSource(VS, 1, (char **)&VSSourceCode, 0);
	glCompileShader(VS);
	glGetShaderiv(VS, GL_COMPILE_STATUS, &Success);
	if(!Success)
	{
		glGetShaderInfoLog(VS, sizeof(InfoLog), 0, InfoLog);
		Platform.OutputDebugString("ERROR::SHADER_COMPILATION_ERROR of type: VS\n");
		Platform.OutputDebugString(InfoLog);
		Platform.OutputDebugString("\n");
	}

	glShaderSource(FS, 1, (char **)&FSSourceCode, 0);
	glCompileShader(FS);
	glGetShaderiv(FS, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(FS, sizeof(InfoLog), 0, InfoLog);
		Platform.OutputDebugString("ERROR::SHADER_COMPILATION_ERROR of type: FS\n");
		Platform.OutputDebugString(InfoLog);
		Platform.OutputDebugString("\n");
	}

	glAttachShader(Shader->ID, VS);
	glAttachShader(Shader->ID, FS);
	glLinkProgram(Shader->ID);
	glGetProgramiv(Shader->ID, GL_LINK_STATUS, &Success);
	if(!Success)
	{
		glGetProgramInfoLog(Shader->ID, sizeof(InfoLog), 0, InfoLog);
		Platform.OutputDebugString("ERROR::PROGRAM_LINKING_ERROR of type:: PROGRAM\n");
		Platform.OutputDebugString(InfoLog);
		Platform.OutputDebugString("\n");
	}

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
SetVec2Array(shader Shader, char *Name, u32 Count, vec2 *Values)
{
	glUniform2fv(glGetUniformLocation(Shader.ID, Name), Count, (GLfloat *)Values);
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
GenerateUBO(GLuint *UBO, u32 Size, u32 BindingPoint)
{
	glGenBuffers(1, UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, *UBO);
	glBufferData(GL_UNIFORM_BUFFER, Size, 0, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, BindingPoint, *UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

inline void 
BindUniformBlockToBindingPoint(shader Shader, char *UniformBlockName, u32 BindingPoint)
{
	u32 BlockIndex = glGetUniformBlockIndex(Shader.ID, UniformBlockName);
	glUniformBlockBinding(Shader.ID, BlockIndex, BindingPoint);
}

inline void
RenderQuad(shader Shader, GLuint VAO, bool32 TexturedUI, vec2 ScreenP, vec2 Scale, r32 Alpha)
{
	glBindVertexArray(VAO);
	SetInt(Shader, "TexturedUI", TexturedUI);
	SetVec2(Shader, "ScreenP", ScreenP);
	SetVec2(Shader, "Scale", Scale);
	SetFloat(Shader, "Alpha", Alpha);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

internal void
RenderQuad(shader Shader, GLuint VAO, game_assets *Assets, texture_id ID, 
		   vec2 ScreenP, vec2 Scale, r32 Alpha)
{
	loaded_texture *Texture = GetTexture(Assets, ID);
	if(Texture)
	{
		glBindTexture(GL_TEXTURE_2D, Texture->TextureID);
		RenderQuad(Shader, VAO, true, ScreenP, Scale, Alpha);
	}
	else
	{
		LoadTexture(Assets, ID);
	}
}

internal void
RenderQuad(shader Shader, GLuint VAO, vec2 ScreenP, vec2 Scale, vec4 Color)
{
	SetVec3(Shader, "Color", vec3(Color.m));
	RenderQuad(Shader, VAO, false, ScreenP, Scale, Color.w());
}

internal rect2
RenderTextLine(loaded_font *Font, shader Shader, GLuint VAO, vec2 ScreenP, char *String, r32 FontScale,
			   bool32 GetRect = false)
{
	rect2 Result = RectMinMax(vec2(FLT_MAX, FLT_MAX), vec2(-FLT_MAX, -FLT_MAX));

	if(Font)
	{
		UseShader(Shader);
		glBindVertexArray(VAO);

		for(char *C = String;
			*C;
			C++)
		{
			char Character = *C;
			char NextCharacter = *(C + 1);

			if(Character != ' ')
			{
				loaded_texture *Glyph = GetBitmapForGlyph(Font, Character);

				if(GetRect)
				{
					vec2 GlyphVertices[] =
					{
						vec2(0.0f, 1.0f),
						vec2(0.0f, 0.0f),
						vec2(1.0f, 1.0f),
						vec2(1.0f, 0.0f)
					};

					vec2 LeftBotCornerP = ScreenP - vec2(0.0f, Glyph->AlignPercentageY*FontScale*Glyph->Height);
					for(u32 GlyphVertexIndex = 0;
						GlyphVertexIndex < ArrayCount(GlyphVertices);
						GlyphVertexIndex++)
					{
						vec2 P = LeftBotCornerP + FontScale*Hadamard(vec2i(Glyph->Width, Glyph->Height),
																	 GlyphVertices[GlyphVertexIndex]);
						if(Result.Min.x > P.x)
						{
							Result.Min.x = P.x;
						}
						if(Result.Min.y > P.y)
						{
							Result.Min.y = P.y;
						}
						if(Result.Max.x < P.x)
						{
							Result.Max.x = P.x;
						}
						if(Result.Max.y < P.y)
						{
							Result.Max.y = P.y;
						}
					}
				}
				else
				{
					SetVec2(Shader, "ScreenP", ScreenP);
					SetVec3(Shader, "WidthHeightScale", vec3((r32)Glyph->Width, (r32)Glyph->Height, FontScale));
					SetFloat(Shader, "AlignPercentageY", Glyph->AlignPercentageY);
					glBindTexture(GL_TEXTURE_2D, Glyph->TextureID);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				}
			}

			ScreenP.x += FontScale*GetHorizontalAdvanceFor(Font, Character, NextCharacter);
		}

		glBindVertexArray(0);
		UseShader({0});
	}

	return(Result);
}

inline rect2
GetTextLineRect(loaded_font *Font, vec2 ScreenP, char *String, r32 FontScale)
{
	shader DummyShader = {};
	rect2 Result = RenderTextLine(Font, DummyShader, 0, ScreenP, String, FontScale, true);
	return(Result);
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
		vec3 Forward = Normalize(Cross(Right, vec3(0.0f, 1.0f, 0.0f)));
		mat4 ModelMatrix = Translate(Model->Alignment + Model->AlignmentX*Right + Model->AlignmentZ*Forward + Translation) * 
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
AddParticle(particle_generator *Generator, world_position BaseP, particle_emitter_info Info)
{
	particle *Particle = Generator->Particles + Generator->NextParticle++;
	if(Generator->NextParticle >= ArrayCount(Generator->Particles))
	{
		Generator->NextParticle = 0;
	}
	Particle->BaseP = BaseP;
	Particle->P = Hadamard(vec3((((rand() % 100) - 50) / 50.0f), (((rand() % 100) - 50) / 50.0f), (((rand() % 100) - 50) / 50.0f)),
							Info.StartPRanges);

	Particle->dP = Hadamard(vec3((((rand() % 100) - 50) / 50.0f), (((rand() % 100) - 50) / 50.0f), (((rand() % 100) - 50) / 50.0f)),
							Info.StartdPRanges);
	if(Info.dPY != 0.0f)
	{
		Particle->dP.SetY(Info.dPY);
	}

	Particle->ddP = Info.StartddP;

	Particle->Color = Info.Color;
	Particle->LifeTime = Info.MaxLifeTime;
	Particle->Scale = Info.Scale;
}

internal void
SpawnParticles(particle_generator *Generator, world_position BaseP, particle_emitter_info Info, r32 dt)
{
	u32 ParticlesToSpawnCount = (u32)Round(Info.SpawnInSecond * dt);
	for(u32 ParticleSpawnIndex = 0;
		ParticleSpawnIndex < ParticlesToSpawnCount;
		ParticleSpawnIndex++)
	{
		AddParticle(Generator, BaseP, Info);
	}
}

internal void
AddParticles(particle_generator *Generator, u32 Count, world_position BaseP, particle_emitter_info Info)
{
	for(u32 ParticleIndex = 0;
		ParticleIndex < Count;
		ParticleIndex++)
	{
		AddParticle(Generator, BaseP, Info);
	}
}

internal void
ParticlesUpdate(particle_generator *Generator, r32 dt)
{
	TIME_BLOCK;
	for(u32 ParticleIndex = 0;
		ParticleIndex < ArrayCount(Generator->Particles);
		ParticleIndex++)
	{
		particle *Particle = Generator->Particles + ParticleIndex;
		if(Particle->LifeTime > 0.0f)
		{
			Particle->P += 0.5f*Particle->ddP*dt*dt + Particle->dP*dt;
			Particle->dP += Particle->ddP*dt;
			Particle->LifeTime -= dt;
		}
	}
}

internal void
RenderParticles(particle_generator *Generator, world *World, stack_allocator *Allocator, 
					 shader Shader, world_position Origin)
{
	TIME_BLOCK;
	u32 ParticlesCount = 0;
	vec3 *ParticlesPositions = PushArray(Allocator, MAX_PARTICLES_COUNT, vec3);
	vec3 *ParticlesColors = PushArray(Allocator, MAX_PARTICLES_COUNT, vec3);
	r32 *ParticlesScales = PushArray(Allocator, MAX_PARTICLES_COUNT, r32);

	for(u32 ParticleIndex = 0;
		ParticleIndex < ArrayCount(Generator->Particles);
		ParticleIndex++)
	{
		particle *Particle = Generator->Particles + ParticleIndex;
		if(Particle->LifeTime > 0.0f)
		{
			ParticlesPositions[ParticlesCount] = Substract(World, &Particle->BaseP, &Origin) + Particle->P;
			ParticlesColors[ParticlesCount] = Particle->Color;
			ParticlesScales[ParticlesCount] = Particle->Scale;
			ParticlesCount++;
		}
	}

	UseShader(Shader);
	glBindVertexArray(Generator->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, Generator->SimPVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*ParticlesCount, ParticlesPositions, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);

	glBindBuffer(GL_ARRAY_BUFFER, Generator->ColorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*ParticlesCount, ParticlesColors, GL_STATIC_DRAW);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);

	glBindBuffer(GL_ARRAY_BUFFER, Generator->ScaleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(r32)*ParticlesCount, ParticlesScales, GL_STATIC_DRAW);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(r32), (void *)0);

	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);

	glDrawArraysInstanced(GL_TRIANGLES, 0, 36, ParticlesCount);

	glBindVertexArray(0);
}

// 
// 
// 

#if VOXEL_ENGINE_INTERNAL

global_variable debug_draw_info GlobalDebugDrawInfo;

internal void
InitializeGlobalDrawInfo(void)
{
	CompileShader(&GlobalDebugDrawInfo.Shader, DebugDrawingVS, DebugDrawingFS);

	// NOTE(georgy): Cube data
	{
		r32 CubeVertices[] = 
		{
			// Back face
			-0.5f, -0.5f, -0.5f,
			0.5f,  0.5f, -0.5f,
			0.5f, -0.5f, -0.5f,
			0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			// Front face
			-0.5f, -0.5f,  0.5f,
			0.5f, -0.5f,  0.5f,
			0.5f,  0.5f,  0.5f,
			0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			// Left face
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			// Right face
			0.5f,  0.5f,  0.5f,
			0.5f, -0.5f, -0.5f,
			0.5f,  0.5f, -0.5f,
			0.5f, -0.5f, -0.5f,
			0.5f,  0.5f,  0.5f,
			0.5f, -0.5f,  0.5f,
			// Bottom face
			-0.5f, -0.5f, -0.5f,
			0.5f, -0.5f, -0.5f,
			0.5f, -0.5f,  0.5f,
			0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f, -0.5f,
			// Top face
			-0.5f,  0.5f, -0.5f,
			0.5f,  0.5f , 0.5f,
			0.5f,  0.5f, -0.5f,
			0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f,  0.5f,
		};

		glGenVertexArrays(1, &GlobalDebugDrawInfo.CubeVAO);
		glGenBuffers(1, &GlobalDebugDrawInfo.CubeVBO);
		glBindVertexArray(GlobalDebugDrawInfo.CubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, GlobalDebugDrawInfo.CubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(CubeVertices), CubeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(r32), (void *)0);
		glBindVertexArray(0);
	}

	// NOTE(georgy): Sphere data
	{
		r32 Radius = 0.5f;
		u32 ParallelCount = 10;
		u32 MeridianCount = 10;

		dynamic_array_vec3 Vertices;
		InitializeDynamicArray(&Vertices);
		InitializeDynamicArray(&GlobalDebugDrawInfo.SphereIndices);

		PushEntry(&Vertices, Radius*vec3(0.0f, -1.0f, 0.0f));
		for(u32 Parallel = 0;
			Parallel < ParallelCount;
			Parallel++)
		{
			r32 Phi = ((PI *  (Parallel + 1)) / (ParallelCount + 1)) - 0.5f*PI;
			for(u32 Meridian = 0;
				Meridian < MeridianCount;
				Meridian++)
			{
				r32 Theta = (2.0f*PI *  Meridian) / MeridianCount;
				r32 X = Sin(Theta)*Cos(Phi);
				r32 Y = Sin(Phi);
				r32 Z = -Cos(Theta)*Cos(Phi);
				vec3 P = Radius*Normalize(vec3(X, Y, Z));
				PushEntry(&Vertices, P);
			}
		}
		PushEntry(&Vertices, Radius*vec3(0.0f, 1.0f, 0.0f));

		for(u32 Meridian = 0;
			Meridian < MeridianCount;
			Meridian++)
		{
			AddTriangle(&GlobalDebugDrawInfo.SphereIndices, 0, 
								   							Meridian + 1, 
								   							((Meridian + 1) % MeridianCount) + 1);
		}

		for(u32 Parallel = 0;
			Parallel < ParallelCount - 1;
			Parallel++)
		{
			for(u32 Meridian = 0;
				Meridian < MeridianCount;
				Meridian++)
			{
				u32 A0 = (Parallel*MeridianCount + 1) + Meridian;
				u32 A1 = (Parallel*MeridianCount + 1) + (Meridian + 1) % MeridianCount;
				u32 B0 = ((Parallel+1)*MeridianCount + 1) + Meridian;
				u32 B1 = ((Parallel+1)*MeridianCount + 1) + (Meridian + 1) % MeridianCount;
				AddQuad(&GlobalDebugDrawInfo.SphereIndices, A0, A1, B1, B0);
			}
		}

		for(u32 Meridian = 0;
			Meridian < MeridianCount;
			Meridian++)
		{
			
			AddTriangle(&GlobalDebugDrawInfo.SphereIndices, Vertices.EntriesCount - 1, 
								   							((Meridian + 1) % MeridianCount) + (MeridianCount*(ParallelCount - 1) + 1),
															Meridian + (MeridianCount*(ParallelCount - 1) + 1));
		}

		glGenVertexArrays(1, &GlobalDebugDrawInfo.SphereVAO);
		glGenBuffers(1, &GlobalDebugDrawInfo.SphereVBO);
		glGenBuffers(1, &GlobalDebugDrawInfo.SphereEBO);
		glBindVertexArray(GlobalDebugDrawInfo.SphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, GlobalDebugDrawInfo.SphereVBO);
		glBufferData(GL_ARRAY_BUFFER, Vertices.EntriesCount*sizeof(vec3), Vertices.Entries, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GlobalDebugDrawInfo.SphereEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, GlobalDebugDrawInfo.SphereIndices.EntriesCount*sizeof(u32), 
					 GlobalDebugDrawInfo.SphereIndices.Entries, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
		glBindVertexArray(0);

		FreeDynamicArray(&Vertices);
	}

	// NOTE(georgy): Line data
	{
		glGenVertexArrays(1, &GlobalDebugDrawInfo.LineVAO);
		glGenBuffers(1, &GlobalDebugDrawInfo.LineVBO);
		glBindVertexArray(GlobalDebugDrawInfo.LineVAO);
		glBindBuffer(GL_ARRAY_BUFFER, GlobalDebugDrawInfo.LineVBO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
		glBindVertexArray(0);
	}

	GlobalDebugDrawInfo.IsInitialized = true;
}

inline void 
DEBUGBeginRenderDebugObject(shader *Shader, GLuint *VAO, mat4 Model)
{
	if(!GlobalDebugDrawInfo.IsInitialized)
	{
		InitializeGlobalDrawInfo();
	}
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	UseShader(*Shader);
	glBindVertexArray(*VAO);
	SetMat4(*Shader, "Model", Model);
	SetMat4(*Shader, "ViewProjection", GlobalViewProjection);
	// shader Shaders3D[] = { GlobalDebugDrawInfo.Shader, GlobalDebugDrawInfo.AxesShader};
	// Initialize3DTransforms(Shaders3D, ArrayCount(Shaders3D), GlobalViewProjection);
}

inline void 
DEBUGEndRenderDebugObject(void)
{
	glBindVertexArray(0);
	UseShader({0});

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);;
}

internal void
DEBUGRenderCube(vec3 P, vec3 Scaling, r32 Rotation,
				vec3 Color = vec3(1.0, 0.0, 0.0))
{
	mat4 Model = Translate(P) * Rotate(Rotation, vec3(0.0f, 1.0f, 0.0f)) * Scale(Scaling);
	DEBUGBeginRenderDebugObject(&GlobalDebugDrawInfo.Shader, &GlobalDebugDrawInfo.CubeVAO, Model);

	SetVec3(GlobalDebugDrawInfo.Shader, "Color", Color);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	DEBUGEndRenderDebugObject();
}

internal void
DEBUGRenderSphere(vec3 P, vec3 Scaling, r32 Rotation = 0.0f,
				  vec3 Color = vec3(1.0, 0.0, 0.0))
{
	mat4 Model = Translate(P) * Rotate(Rotation, vec3(0.0f, 1.0f, 0.0f)) * Scale(Scaling);
	DEBUGBeginRenderDebugObject(&GlobalDebugDrawInfo.Shader, &GlobalDebugDrawInfo.SphereVAO, Model);

	SetVec3(GlobalDebugDrawInfo.Shader, "Color", Color);
	glDrawElements(GL_TRIANGLES, GlobalDebugDrawInfo.SphereIndices.EntriesCount, GL_UNSIGNED_INT, 0);

	DEBUGEndRenderDebugObject();
}

internal void
DEBUGRenderLine(vec3 FromP, vec3 ToP, vec3 Color = vec3(1.0f, 0.0f, 0.0f))
{
	DEBUGBeginRenderDebugObject(&GlobalDebugDrawInfo.Shader, &GlobalDebugDrawInfo.LineVAO, Identity());
	
	vec3 LineVertices[] = 
	{
		FromP,
		ToP,
	};

	glBindVertexArray(GlobalDebugDrawInfo.LineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, GlobalDebugDrawInfo.LineVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(LineVertices), LineVertices, GL_STATIC_DRAW);

	glLineWidth(8.0f);

	SetVec3(GlobalDebugDrawInfo.Shader, "Color", Color);
	glDrawArrays(GL_LINES, 0, 2);

	glLineWidth(1.0f);
	DEBUGEndRenderDebugObject();
}

internal void
DEBUGRenderAxes(mat4 Transformation)
{
	DEBUGRenderLine(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	DEBUGRenderLine(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f));
	DEBUGRenderLine(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f));
}

inline void
DEBUGRenderPoint(vec3 P, vec3 Color = vec3(1.0, 0.0, 0.0))
{
	DEBUGRenderSphere(P, vec3(0.1f, 0.1f, 0.1f), 0.0f, Color);
}

#else

#define InitializeGlobalDrawInfo(...)
#define DEBUGBeginRenderDebugObject(...)
#define DEBUGEndRenderDebugObject(...)
#define DEBUGRenderCube(...)
#define DEBUGRenderSphere(...)
#define DEBUGRenderLine(...)
#define DEBUGRenderAxes(...)
#define DEBUGRenderPoint(...)

#endif