#include "voxel_engine_debug.h"

struct debug_render_text_line_result
{
	rect2 ScreenRegion;
};
internal debug_render_text_line_result
DEBUGRenderTextLine(debug_state *DebugState, char *String, 
					bool32 RenderAtP = false, vec2 P = vec2(0.0f, 0.0f))
{
	debug_render_text_line_result Result = {};
	Result.ScreenRegion = RectMinMax(vec2(FLT_MAX, FLT_MAX), vec2(-FLT_MAX, -FLT_MAX));
	if(DebugState->Font)
	{
		loaded_font *Font = DebugState->Font;
		UseShader(DebugState->GlyphShader);
		glBindVertexArray(DebugState->GlyphVAO);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		r32 FontScale = DebugState->FontScale;

		vec2 ScreenP = RenderAtP ? P : vec2(-0.5f*DebugState->BufferDim.x, DebugState->TextP.y);
		ScreenP.y -= FontScale * Font->AscenderHeight;

		r32 TextMinY = ScreenP.y;
		for(char *C = String;
			*C;
			C++)
		{
			char Character = *C;
			char NextCharacter = *(C + 1);

			if(Character != ' ')
			{
				loaded_texture *Glyph = GetBitmapForGlyph(Font, Character);

				vec2 LeftBotCornerP = ScreenP - vec2(0.0f, Glyph->AlignPercentageY*FontScale*Glyph->Height);
				for(u32 GlyphVertexIndex = 0;
					GlyphVertexIndex < ArrayCount(DebugState->GlyphVertices);
					GlyphVertexIndex++)
				{
					vec2 P = LeftBotCornerP + FontScale*Hadamard(vec2i(Glyph->Width, Glyph->Height),
															 DebugState->GlyphVertices[GlyphVertexIndex]);
					if(Result.ScreenRegion.Min.x > P.x)
					{
						Result.ScreenRegion.Min.x = P.x;
					}
					if(Result.ScreenRegion.Min.y > P.y)
					{
						Result.ScreenRegion.Min.y = P.y;
					}
					if(Result.ScreenRegion.Max.x < P.x)
					{
						Result.ScreenRegion.Max.x = P.x;
					}
					if(Result.ScreenRegion.Max.y < P.y)
					{
						Result.ScreenRegion.Max.y = P.y;
					}
				}

				SetVec2(DebugState->GlyphShader, "ScreenP", ScreenP);
				SetVec3(DebugState->GlyphShader, "WidthHeightScale", vec3((r32)Glyph->Width, (r32)Glyph->Height, FontScale));
				SetFloat(DebugState->GlyphShader, "AlignPercentageY", Glyph->AlignPercentageY);
				glBindTexture(GL_TEXTURE_2D, Glyph->TextureID);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}

			ScreenP.x += FontScale*GetHorizontalAdvanceFor(Font, Character, NextCharacter);
		}

		if(!RenderAtP)
		{
			DebugState->TextP = vec2(-0.5f*DebugState->BufferDim.x, Result.ScreenRegion.Min.y);
		}

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glBindVertexArray(0);
	}

	return(Result);
}

global_variable debug_record GlobalDebugRecords[__COUNTER__]; 

inline game_assets *
DEBUGGetGameAssets(game_memory *Memory)
{
	game_assets *GameAssets = 0;

	temp_state *TempState = (temp_state *)Memory->TemporaryStorage;
	if(TempState->IsInitialized)
	{
		GameAssets = TempState->GameAssets;
	}

	return(GameAssets);
}

internal void
DEBUGReset(debug_state *DebugState, game_memory *Memory, game_assets *GameAssets, r32 BufferWidth, r32 BufferHeight)
{
	if (!DebugState->IsInitialized)
	{
		InitializeStackAllocator(&DebugState->Allocator, Memory->DebugStorageSize - sizeof(debug_state), 
														 (u8 *)Memory->DebugStorage + sizeof(debug_state));
		DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Allocator);

		DebugState->FontScale = 0.25f;

		CompileShader(&DebugState->GlyphShader, "data/shaders/GlyphVS.glsl", "data/shaders/GlyphFS.glsl");
		CompileShader(&DebugState->QuadShader, "data/shaders/2DQuadVS.glsl", "data/shaders/2DQuadFS.glsl");

		DebugState->GlyphVertices[0] = { 0.0f, 1.0f };
		DebugState->GlyphVertices[1] = { 0.0f, 0.0f };
		DebugState->GlyphVertices[2] = { 1.0f, 1.0f };
		DebugState->GlyphVertices[3] = { 1.0f, 0.0f };

		glGenVertexArrays(1, &DebugState->GlyphVAO);
		glGenBuffers(1, &DebugState->GlyphVBO);
		glBindVertexArray(DebugState->GlyphVAO);
		glBindBuffer(GL_ARRAY_BUFFER, DebugState->GlyphVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(DebugState->GlyphVertices), DebugState->GlyphVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glBindVertexArray(0);

		DebugState->IsInitialized = true;
	}

	asset_tag_vector MatchVector = { };
	MatchVector.E[Tag_FontType] = FontType_DebugFont;
	DebugState->FontID = GetBestMatchFont(GameAssets, &MatchVector);
	DebugState->Font = GetFont(GameAssets, DebugState->FontID);
	if(!DebugState->Font)
	{
		LoadFont(GameAssets, DebugState->FontID);
	}

	DebugState->TextP = vec2(-0.5f*BufferWidth, 0.5f*BufferHeight);
	DebugState->BufferDim = vec2(BufferWidth, BufferHeight);

	DebugState->Orthographic = Ortho(-0.5f*BufferHeight, 0.5f*BufferHeight, 
									 -0.5f*BufferWidth, 0.5f*BufferWidth, 
									  0.1f, 1000.0f);
	UseShader(DebugState->GlyphShader);
	SetMat4(DebugState->GlyphShader, "Projection", DebugState->Orthographic);
	SetInt(DebugState->GlyphShader, "Texture", 0);
	glActiveTexture(GL_TEXTURE0);
	UseShader(DebugState->QuadShader);
	SetMat4(DebugState->QuadShader, "Projection", DebugState->Orthographic);
}

inline debug_thread *
GetDebugThread(debug_state *DebugState, u32 ID)
{
	debug_thread *Result = 0;
	for(debug_thread *Thread = DebugState->FirstThread;
		Thread;
		Thread = Thread->Next)
	{
		if(Thread->ID == ID)
		{
			Result = Thread;
			break;
		}
	}

	if(!Result)
	{
		Result = PushStruct(&DebugState->Allocator, debug_thread);
		Result->ID = ID;
		Result->LaneIndex = DebugState->LaneCount++;
		Result->OpenDebugBlocks = 0;

		Result->Next = DebugState->FirstThread;
		DebugState->FirstThread = Result;
	}

	return(Result);
}

inline debug_record *
GetRecordFrom(open_debug_block *Block)
{
	debug_record *Result = Block ? Block->Record : 0;
	return(Result);
}

internal void
DEBUGCollateEvents(debug_state *DebugState, u32 EventArrayIndex)
{
	for(u32 EventIndex = 0;
		EventIndex < GlobalDebugTable.EventsCounts[EventArrayIndex];
		EventIndex++)
	{
		debug_event *Event = GlobalDebugTable.EventsArrays[EventArrayIndex] + EventIndex;
		debug_record *Record = GlobalDebugRecords + Event->DebugRecordIndex;

		if(Event->Type == DebugEvent_FrameMarker)
		{
			if(DebugState->CollationFrame)
			{
				DebugState->CollationFrame->EndClock = Event->Clock;
				DebugState->CollationFrame->MSElapsed = Event->MSElapsed;
			}
			DebugState->CollationFrame = DebugState->Frames + DebugState->FrameCount++;
			DebugState->CollationFrame->Regions = PushArray(&DebugState->Allocator, MAX_REGIONS_PER_FRAME, debug_region);
			DebugState->CollationFrame->RegionsCount = 0;
			DebugState->CollationFrame->BeginClock = Event->Clock;
			DebugState->CollationFrame->MSElapsed = 0.0f;
		}
		else
		{
			if(DebugState->CollationFrame)
			{
				debug_thread *Thread = GetDebugThread(DebugState, Event->ThreadID);	

				switch(Event->Type)
				{
					case DebugEvent_BeginBlock:
					{
						open_debug_block *DebugBlock = PushStruct(&DebugState->Allocator, open_debug_block);
						DebugBlock->Record = Record;
						DebugBlock->Event = Event;

						DebugBlock->Parent = Thread->OpenDebugBlocks;
						Thread->OpenDebugBlocks = DebugBlock;
					} break;

					case DebugEvent_EndBlock: 
					{
						if(Thread->OpenDebugBlocks)
						{
							open_debug_block *MatchingBlock = Thread->OpenDebugBlocks;
							if((MatchingBlock->Event->DebugRecordIndex == Event->DebugRecordIndex) &&
							   (MatchingBlock->Event->ThreadID == Event->ThreadID))
							{
								if(GetRecordFrom(MatchingBlock->Parent) == GlobalDebugTable.ProfileBlockRecord)
								{
									debug_region *Region = DebugState->CollationFrame->Regions + DebugState->CollationFrame->RegionsCount++;
									Region->Record = Record;
									Region->StartCyclesInFrame = (r32)(MatchingBlock->Event->Clock - DebugState->CollationFrame->BeginClock);
									Region->EndCyclesInFrame = (r32)(Event->Clock - DebugState->CollationFrame->BeginClock);
									Region->LaneIndex = Thread->LaneIndex;
									Region->ColorIndex = Event->DebugRecordIndex;
								}

								Thread->OpenDebugBlocks = MatchingBlock->Parent;
							}
						}
					} break;

					default:
					{
						Assert(!"Invalid event type!");
					} break;
				}
			}
		}
	}
}

inline void
RestartCollation(debug_state *DebugState)
{
	EndTemporaryMemory(DebugState->CollateTemp);
	DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Allocator);

	DebugState->LaneCount = 0;
	DebugState->FirstThread = 0;

	DebugState->FrameCount = 0;
	DebugState->CollationFrame = 0;
}

internal void
DEBUGRenderRegions(debug_state *DebugState, game_input *Input)
{
	vec2 MouseP = vec2(Input->MouseX, Input->MouseY);

	UseShader(DebugState->QuadShader);
	glBindVertexArray(DebugState->GlyphVAO);

	vec3 Colors[] = 
	{
		vec3(1.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 0.0f, 1.0f),
		vec3(1.0f, 1.0f, 0.0f),
		vec3(1.0f, 0.0f, 1.0f),
		vec3(0.0f, 1.0f, 1.0f),
	};

	r32 CyclesPerFrame = 86666666.0f;
	r32 TableWidth = 300.0f;
	r32 BarSpacing = 5.0f;
	u32 LaneCount = DebugState->LaneCount;
	r32 LaneHeight = 10.0f;

	vec2 StartP = vec2(0.5f*-DebugState->BufferDim.x, DebugState->TextP.y - LaneHeight - BarSpacing);
	// vec2 StartP = vec2(0.5f*-DebugState->BufferDim.x, 0.5f*DebugState->BufferDim.y - LaneHeight);
	r32 MinY = StartP.y;

	debug_record *HotRecord = 0;

	u32 MaxFrameCount = DebugState->FrameCount > 0 ? DebugState->FrameCount - 1 : 0;
	u32 FrameCount = MaxFrameCount > 8 ? 8 : MaxFrameCount;
	for(u32 FrameIndex = 0;
		FrameIndex < FrameCount;
		FrameIndex++)
	{
		debug_frame *CollationFrame = DebugState->Frames + MaxFrameCount - (FrameIndex + 1);

		vec2 P = StartP - (r32)FrameIndex*vec2(0.0f, LaneCount*LaneHeight + BarSpacing);
		if(CollationFrame->RegionsCount > 0)
		{
			for(u32 RegionIndex = 0;
				RegionIndex < CollationFrame->RegionsCount;
				RegionIndex++)
			{
				UseShader(DebugState->QuadShader);
				glBindVertexArray(DebugState->GlyphVAO);
				debug_region *Region = CollationFrame->Regions + RegionIndex;

				r32 PxStart = (Region->StartCyclesInFrame / CyclesPerFrame) * TableWidth;
				r32 PxEnd = (Region->EndCyclesInFrame / CyclesPerFrame) * TableWidth;
				r32 Py = -(Region->LaneIndex*LaneHeight);

				vec2 ScreenP = P + vec2(PxStart, Py);
				MinY = ScreenP.y < MinY ? ScreenP.y : MinY;
				SetVec2(DebugState->QuadShader, "ScreenP", ScreenP);
				SetVec2(DebugState->QuadShader, "Scale", vec2(PxEnd - PxStart, LaneHeight));
				SetVec3(DebugState->QuadShader, "Color", Colors[Region->ColorIndex % ArrayCount(Colors)]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				rect2 RegionRect = RectMinMax(ScreenP, ScreenP + vec2(PxEnd - PxStart, LaneHeight));
				if(IsInRect(RegionRect, MouseP))
				{
					debug_record *Record = Region->Record;
					char Buffer[256];
					_snprintf_s(Buffer, sizeof(Buffer), "%s(%u): %ucy", 
								Record->BlockName, Record->LineNumber, 
								(u32)(Region->EndCyclesInFrame - Region->StartCyclesInFrame));

					DEBUGRenderTextLine(DebugState, Buffer, 
										true, vec2(ScreenP + vec2(TableWidth - PxStart + BarSpacing, LaneHeight)));

					HotRecord = Record;
				}
			}
		}
	}

	DebugState->TextP.y = MinY;

	SetVec2(DebugState->QuadShader, "ScreenP", vec2(StartP.x, MinY) + vec2(TableWidth, 0.0f));
	SetVec2(DebugState->QuadShader, "Scale", vec2(2.0f, StartP.y - MinY + LaneHeight));
	SetVec3(DebugState->QuadShader, "Color", vec3(0.0f, 0.0f, 0.0f));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	rect2 ProfilingTableRect = RectMinMax(vec2(StartP.x, MinY), 
										  vec2(StartP.x + TableWidth, StartP.y + LaneHeight));
	if(IsInRect(ProfilingTableRect, MouseP))
	{
		if(WasDown(&Input->MouseRight))
		{
			GlobalDebugTable.ProfilePause = !GlobalDebugTable.ProfilePause || GlobalGamePause;
			if(!GlobalDebugTable.ProfilePause)
			{
				RestartCollation(DebugState);
			}
		}

		if(WasDown(&Input->MouseLeft))
		{
			if(HotRecord)
			{
				GlobalDebugTable.ProfileBlockRecord = HotRecord;
			}
			else 
			{
				GlobalDebugTable.ProfileBlockRecord = 0;	
			}

			RestartCollation(DebugState);

			for(u32 EventArrayIndex = 0;
				EventArrayIndex < ArrayCount(GlobalDebugTable.EventsArrays) - 1;
				EventArrayIndex++)
			{
				DEBUGCollateEvents(DebugState, 
								((GlobalDebugTable.CurrentEventArrayIndex + 1) + EventArrayIndex) % ArrayCount(GlobalDebugTable.		EventsArrays));
			}
		}
	}

	glBindVertexArray(0);
}

inline void
DEBUGAddMainMenuElement(debug_state *DebugState, game_input* Input, bool32 *ElementState, char *Name)
{
	char Buffer[256];
	char *At = Buffer;
	At += _snprintf_s(Buffer, sizeof(Buffer), "%s: ", Name);
	_snprintf_s(At, sizeof(Buffer) - (At - Buffer), sizeof(Buffer) - (At - Buffer), 
				"%s", *ElementState ? "true" : "false");

	debug_render_text_line_result TextRegion = DEBUGRenderTextLine(DebugState, Buffer);
	if(IsInRect(TextRegion.ScreenRegion, vec2(Input->MouseX, Input->MouseY)))
	{
		if(WasDown(&Input->MouseLeft))
		{
			*ElementState = !(*ElementState);
		}
	}
}

internal void
DEBUGRenderMainMenu(debug_state *DebugState, game_input *Input)
{
	DEBUGAddMainMenuElement(DebugState, Input, &DEBUGGlobalShowDebugDrawings, "ShowDebugDrawings");
	DEBUGAddMainMenuElement(DebugState, Input, &DEBUGGlobalRenderShadows, "RenderShadows");
	DEBUGAddMainMenuElement(DebugState, Input, &DEBUGGlobalShowProfiling, "ShowProfiling");

	if(DEBUGGlobalShowProfiling)
	{
		DEBUGRenderRegions(DebugState, Input);
	}
}

internal void
DEBUGEndDebugFrameAndRender(game_memory *Memory, game_input *Input, r32 BufferWidth, r32 BufferHeight)
{
	Assert(sizeof(debug_state) <= Memory->DebugStorageSize);
	debug_state* DebugState = (debug_state*)Memory->DebugStorage;
	game_assets* GameAssets = DEBUGGetGameAssets(Memory);

	u32 EventArrayIndex = 0;
	if(!GlobalDebugTable.ProfilePause)
	{
		GlobalDebugTable.CurrentEventArrayIndex++;
		if(GlobalDebugTable.CurrentEventArrayIndex >= ArrayCount(GlobalDebugTable.EventsArrays))
		{
			GlobalDebugTable.CurrentEventArrayIndex = 0;
		}
		u64 EventArrayIndex_EventIndex = AtomicExchangeU64(&GlobalDebugTable.EventArrayIndex_EventIndex,
													 	   (u64)GlobalDebugTable.CurrentEventArrayIndex << 32);

		EventArrayIndex = EventArrayIndex_EventIndex >> 32;
		u32 EventsCount = EventArrayIndex_EventIndex & 0xFFFFFFFF;
		GlobalDebugTable.EventsCounts[EventArrayIndex] = EventsCount;
	}
	
	if(DebugState && GameAssets)
	{
		DEBUGReset(DebugState, Memory, GameAssets, BufferWidth, BufferHeight);

		if(DebugState->FrameCount >= ArrayCount(DebugState->Frames))
		{
			RestartCollation(DebugState);
		}

		if(!GlobalDebugTable.ProfilePause)
		{
			DEBUGCollateEvents(DebugState, EventArrayIndex);
		}
	}

	DEBUGRenderMainMenu(DebugState, Input);

	char Buffer[256];
	if(DebugState->FrameCount > 1)
	{
		_snprintf_s(Buffer, sizeof(Buffer), "%.02fms/f", DebugState->Frames[DebugState->FrameCount - 2].MSElapsed);
		DEBUGRenderTextLine(DebugState, Buffer);
	}
}