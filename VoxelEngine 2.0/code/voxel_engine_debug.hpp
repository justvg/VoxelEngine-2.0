#include "voxel_engine_debug.h"

internal void
DEBUGRenderTextLine(debug_state *DebugState, char *String, 
					bool32 RenderAtP = false, vec2 P = vec2(0.0f, 0.0f))
{
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

				r32 Py = ScreenP.y - Glyph->AlignPercentageY*FontScale*Glyph->Height;
				if(TextMinY > Py)
				{
					TextMinY = Py;
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
			DebugState->TextP = vec2(ScreenP.x, TextMinY);
		}
		
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glBindVertexArray(0);
	}
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

		r32 GlyphVertices[] = 
		{
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 1.0f,
			1.0f, 0.0f
		};
		glGenVertexArrays(1, &DebugState->GlyphVAO);
		glGenBuffers(1, &DebugState->GlyphVBO);
		glBindVertexArray(DebugState->GlyphVAO);
		glBindBuffer(GL_ARRAY_BUFFER, DebugState->GlyphVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GlyphVertices), GlyphVertices, GL_STATIC_DRAW);
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
		EventIndex < GlobalDebugEventsCounts[EventArrayIndex];
		EventIndex++)
	{
		debug_event *Event = GlobalDebugEventsArrays[EventArrayIndex] + EventIndex;
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
								if(GetRecordFrom(MatchingBlock->Parent) == DebugState->ProfileBlockRecord)
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

	bool32 WasInteracted = false;

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
					WasInteracted = true;

					debug_record *Record = Region->Record;
					char Buffer[256];
					_snprintf_s(Buffer, sizeof(Buffer), "%s(%u): %ucy", 
								Record->BlockName, Record->LineNumber, 
								(u32)(Region->EndCyclesInFrame - Region->StartCyclesInFrame));

					DEBUGRenderTextLine(DebugState, Buffer, 
										true, vec2(ScreenP + vec2(TableWidth - PxStart + BarSpacing, LaneHeight)));

					if(WasDown(&Input->MouseLeft))
					{
						DebugState->ProfileBlockRecord = Record;
					}
				}
			}
		}
	}

	DebugState->TextP.y = MinY;

	SetVec2(DebugState->QuadShader, "ScreenP", vec2(StartP.x, MinY) + vec2(TableWidth, 0.0f));
	SetVec2(DebugState->QuadShader, "Scale", vec2(2.0f, StartP.y - MinY + LaneHeight));
	SetVec3(DebugState->QuadShader, "Color", vec3(0.0f, 0.0f, 0.0f));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	rect2 RegionTableRect = RectMinMax(vec2(StartP.x, MinY), vec2(StartP.x + TableWidth, StartP.y + LaneHeight));
	if(IsInRect(RegionTableRect, MouseP))
	{
		if(WasDown(&Input->MouseRight))
		{
			if(WasInteracted)
			{
				DebugState->ProfileBlockRecord = 0;
			}
			else 
			{
				DebugState->ProfilePause = !DebugState->ProfilePause;	
			}
		}
	}

	glBindVertexArray(0);
}

internal void
DEBUGRenderAllDebugRecords(game_memory *Memory, game_input *Input, r32 BufferWidth, r32 BufferHeight)
{
	u32 EventArrayIndex = (u32)(GlobalEventArrayIndex_EventIndex >> 32) + 1;
	if(EventArrayIndex >= ArrayCount(GlobalDebugEventsArrays))
	{
		EventArrayIndex = 0;
	}
	u64 EventArrayIndex_EventIndex = AtomicExchangeU64(&GlobalEventArrayIndex_EventIndex,
													   (u64)EventArrayIndex << 32);

	EventArrayIndex = EventArrayIndex_EventIndex >> 32;
	u32 EventsCount = EventArrayIndex_EventIndex & 0xFFFFFFFF;
	GlobalDebugEventsCounts[EventArrayIndex] = EventsCount;

	Assert(sizeof(debug_state) <= Memory->DebugStorageSize);
    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
	game_assets *GameAssets = DEBUGGetGameAssets(Memory);
	if(DebugState && GameAssets)
	{
		DEBUGReset(DebugState, Memory, GameAssets, BufferWidth, BufferHeight);

		if(DebugState->FrameCount >= ArrayCount(DebugState->Frames))
		{
			EndTemporaryMemory(DebugState->CollateTemp);
			DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Allocator);

			DebugState->LaneCount = 0;
			DebugState->FirstThread = 0;

			DebugState->FrameCount = 0;
			DebugState->CollationFrame = 0;
		}

		if(!DebugState->ProfilePause)
		{
			DEBUGCollateEvents(DebugState, EventArrayIndex);
		}
	}

	DEBUGRenderRegions(DebugState, Input);

	char Buffer[256];
	if(DebugState->FrameCount > 1)
	{
		_snprintf_s(Buffer, sizeof(Buffer), "FPSy: %.02fms/f", DebugState->Frames[DebugState->FrameCount - 2].MSElapsed);
		DEBUGRenderTextLine(DebugState, Buffer);
	}

#if 0
	for(u32 DebugRecordIndex = 0;
		DebugRecordIndex < ArrayCount(GlobalDebugRecords);
		DebugRecordIndex++)
	{
		debug_record *Record = GlobalDebugRecords + DebugRecordIndex;
		u64 CyclesElapsed_HitCount = AtomicExchangeU64(&Record->CyclesElapsed_HitCount, 0);
		u32 CyclesElapsed = CyclesElapsed_HitCount >> 32;
		u32 HitCount = CyclesElapsed_HitCount & 0xFFFFFFFF;
		if(HitCount > 0)
		{
			u32 CyclesPerHit = CyclesElapsed / HitCount;
			_snprintf_s(Buffer, sizeof(Buffer), "%s(%u): %ucy/h %u", Record->BlockName, Record->LineNumber, CyclesPerHit, HitCount);
			DEBUGRenderTextLine(DebugState, Buffer);
		}
	}
#endif
}