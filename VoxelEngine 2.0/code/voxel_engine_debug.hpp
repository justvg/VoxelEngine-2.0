#include "voxel_engine_debug.h"

enum debug_text_op
{
	DEBUGTextOp_Render,
	DEBUGTextOp_GetRect,
};
internal rect2
DEBUGTextLineOperation(debug_state *DebugState, debug_text_op Op, char *String, 
					   vec2 ScreenP)
{
	rect2 Result = RectMinMax(vec2(FLT_MAX, FLT_MAX), vec2(-FLT_MAX, -FLT_MAX));
	if(DebugState->Font)
	{
		loaded_font *Font = DebugState->Font;
		UseShader(DebugState->GlyphShader);
		glBindVertexArray(DebugState->GlyphVAO);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		r32 FontScale = DebugState->FontScale;

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

				if(Op == DEBUGTextOp_GetRect)
				{
					vec2 LeftBotCornerP = ScreenP - vec2(0.0f, Glyph->AlignPercentageY*FontScale*Glyph->Height);
					for(u32 GlyphVertexIndex = 0;
						GlyphVertexIndex < ArrayCount(DebugState->GlyphVertices);
						GlyphVertexIndex++)
					{
						vec2 P = LeftBotCornerP + FontScale*Hadamard(vec2i(Glyph->Width, Glyph->Height),
																	 DebugState->GlyphVertices[GlyphVertexIndex]);
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
					Assert(Op == DEBUGTextOp_Render);

					SetVec2(DebugState->GlyphShader, "ScreenP", ScreenP);
					SetVec3(DebugState->GlyphShader, "WidthHeightScale", vec3((r32)Glyph->Width, (r32)Glyph->Height, FontScale));
					SetFloat(DebugState->GlyphShader, "AlignPercentageY", Glyph->AlignPercentageY);
					glBindTexture(GL_TEXTURE_2D, Glyph->TextureID);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				}
			}

			ScreenP.x += FontScale*GetHorizontalAdvanceFor(Font, Character, NextCharacter);
		}

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glBindVertexArray(0);
	}

	return(Result);
}

inline rect2
DEBUGGetTextRect(debug_state *DebugState, char *String, vec2 ScreenP)
{
	rect2 Result = DEBUGTextLineOperation(DebugState, DEBUGTextOp_GetRect, String, ScreenP);

	return(Result);
}

inline void
DEBUGTextLineAt(debug_state *DebugState, char *String, vec2 ScreenP)
{
	DEBUGTextLineOperation(DebugState, DEBUGTextOp_Render, String, ScreenP); 
}

inline void 
DEBUGTextLineToMenu(debug_state *DebugState, char *String, vec3 Color)
{
	UseShader(DebugState->GlyphShader);
	SetVec3(DebugState->GlyphShader, "Color", Color);
	vec2 ScreenP = vec2(-0.5f*DebugState->BufferDim.x, DebugState->TextP.y);
	DEBUGTextLineAt(DebugState, String, ScreenP);
}

inline void
DEBUGTextLine(debug_state *DebugState, char *String, r32 TextLineMinY, vec3 Color = vec3(1.0f, 1.0f, 1.0f))
{
	DEBUGTextLineToMenu(DebugState, String, Color);
	DebugState->TextP.y = TextLineMinY;
}

inline void
DEBUGTextLine(debug_state *DebugState, char *String, vec3 Color = vec3(1.0f, 1.0f, 1.0f))
{
	DEBUGTextLineToMenu(DebugState, String, Color);
	rect2 TextRegion = DEBUGGetTextRect(DebugState, String, vec2(-0.5f*DebugState->BufferDim.x, DebugState->TextP.y));
	DebugState->TextP.y = TextRegion.Min.y;
}

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
		SubAllocator(&DebugState->FrameAllocator, &DebugState->Allocator, Megabytes(5));

		DebugState->FontScale = 0.2f;

		CompileShader(&DebugState->GlyphShader, "data/shaders/GlyphVS.glsl", "data/shaders/GlyphFS.glsl");
		CompileShader(&DebugState->QuadShader, "data/shaders/2DQuadDebugVS.glsl", "data/shaders/2DQuadDebugFS.glsl");

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

		DebugState->FramesSentinel.Next = DebugState->FramesSentinel.Prev = &DebugState->FramesSentinel;

		for(u32 GroupIndex = 0;
			GroupIndex < DebugValueEventGroup_Count;
			GroupIndex++)
		{
			DebugState->Groups[GroupIndex] = {};
			DebugState->Groups[GroupIndex].Type = DebugEvent_Group;
			DebugState->Groups[GroupIndex].Value_bool32 = false;
		}
		DebugState->Groups[DebugValueEventGroup_Rendering].Name = "Rendering";
		DebugState->Groups[DebugValueEventGroup_World].Name = "World";
		DebugState->Groups[DebugValueEventGroup_DebugTools].Name = "DebugTools";

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

internal void
FreeOldestFrame(debug_state *DebugState)
{
	while(DebugState->OldestStoredEvent && (DebugState->OldestStoredEvent->FrameIndex <= DebugState->FramesSentinel.Prev->Index))
	{
		debug_stored_event *FreeEvent = DebugState->OldestStoredEvent;
		DebugState->OldestStoredEvent = DebugState->OldestStoredEvent->Next;
		if(DebugState->MostRecentStoredEvent == FreeEvent)
		{
			Assert(FreeEvent->Next == 0);
			DebugState->MostRecentStoredEvent = 0;
		}

		FreeEvent->NextFree = DebugState->FirstFreeStoredEvent;
		DebugState->FirstFreeStoredEvent = FreeEvent;
	}

	debug_frame *FrameToFree = DebugState->FramesSentinel.Prev;
	FrameToFree->Prev->Next = &DebugState->FramesSentinel;
	DebugState->FramesSentinel.Prev = FrameToFree->Prev;

	FrameToFree->Next = FrameToFree->Prev = 0;

	FrameToFree->NextFree = DebugState->FirstFreeFrame;
	DebugState->FirstFreeFrame = FrameToFree;
}

internal debug_frame *
NewFrame(debug_state *DebugState, u64 BeginClock)
{
	if(DebugState->CollationFrame)
	{
		DebugState->CollationFrame->Next = DebugState->FramesSentinel.Next;
		DebugState->CollationFrame->Prev = &DebugState->FramesSentinel;

		DebugState->FramesSentinel.Next->Prev = DebugState->CollationFrame;
		DebugState->FramesSentinel.Next = DebugState->CollationFrame;
	}

	debug_frame *Frame = 0;
	while(!Frame)
	{
		Frame = DebugState->FirstFreeFrame;
		if(Frame)
		{
			DebugState->FirstFreeFrame = DebugState->FirstFreeFrame->NextFree;
		}
		else
		{
			if(AllocatorHasRoomFor(&DebugState->FrameAllocator, sizeof(debug_frame) + MAX_REGIONS_PER_FRAME*sizeof(debug_region)))
			{
				Frame = PushStruct(&DebugState->FrameAllocator, debug_frame);
				Frame->Regions = PushArray(&DebugState->FrameAllocator, MAX_REGIONS_PER_FRAME, debug_region);
			}
			else
			{
				FreeOldestFrame(DebugState);
			}
		}
	}
	
	debug_region *Temp = Frame->Regions;
	*Frame = {};
	Frame->Regions = Temp;
	Frame->BeginClock = BeginClock;
	Frame->Index = DebugState->TotalFrameCount++;

	return(Frame);
}

internal debug_stored_event *
StoreEvent(debug_state *DebugState, debug_event *Event)
{
	debug_stored_event *StoredEvent = 0;
	while(!StoredEvent)
	{
		StoredEvent = DebugState->FirstFreeStoredEvent;
		if(StoredEvent)
		{
			DebugState->FirstFreeStoredEvent = DebugState->FirstFreeStoredEvent->NextFree;
		}
		else
		{
			if(AllocatorHasRoomFor(&DebugState->FrameAllocator, sizeof(debug_stored_event)))
			{
				StoredEvent = PushStruct(&DebugState->FrameAllocator, debug_stored_event);
			}
			else
			{
				FreeOldestFrame(DebugState);
			}
		}
	}

	StoredEvent->Event = *Event;
	StoredEvent->FrameIndex = DebugState->CollationFrame->Index;
	StoredEvent->Next = 0;

	if(!DebugState->MostRecentStoredEvent)
	{
		DebugState->MostRecentStoredEvent = DebugState->OldestStoredEvent = StoredEvent;
	}
	else
	{
		DebugState->MostRecentStoredEvent = DebugState->MostRecentStoredEvent->Next = StoredEvent;
	}

	return(StoredEvent);
}

internal void
DEBUGCollateEvents(debug_state *DebugState, u32 EventArrayIndex)
{
	for(u32 EventIndex = 0;
		EventIndex < GlobalDebugTable.EventCount;
		EventIndex++)
	{
		debug_event *Event = GlobalDebugTable.EventsArrays[EventArrayIndex] + EventIndex;

		if(!DebugState->CollationFrame)
		{
			DebugState->CollationFrame = NewFrame(DebugState, Event->Clock);
		}

		if(Event->Type == DebugEvent_SaveDebugVariable)
		{
			Assert(DebugState->VariablesCount < ArrayCount(DebugState->VariablesEvents));
			DebugState->VariablesEvents[DebugState->VariablesCount++] = Event->Value_debug_event;
		}
		else if(Event->Type == DebugEvent_FrameMarker)
		{
			if(DebugState->CollationFrame)
			{
				DebugState->CollationFrame->EndClock = Event->Clock;
				DebugState->CollationFrame->MSElapsed = Event->Value_r32;
			}

			DebugState->CollationFrame = NewFrame(DebugState, Event->Clock);
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
						if(!DebugState->FirstFreeDebugBlock)
						{
							DebugState->FirstFreeDebugBlock = PushStruct(&DebugState->Allocator, open_debug_block);
						}

						open_debug_block *DebugBlock = DebugState->FirstFreeDebugBlock;
						DebugState->FirstFreeDebugBlock = DebugState->FirstFreeDebugBlock->NextFree;

						DebugBlock->Event = &StoreEvent(DebugState, Event)->Event;

						DebugBlock->Parent = Thread->OpenDebugBlocks;
						Thread->OpenDebugBlocks = DebugBlock;
					} break;

					case DebugEvent_EndBlock: 
					{
						if(Thread->OpenDebugBlocks)
						{
							open_debug_block *MatchingBlock = Thread->OpenDebugBlocks;
							if(MatchingBlock->Event->ThreadID == Event->ThreadID)
							{
								char *MatchName = MatchingBlock->Parent ? MatchingBlock->Parent->Event->Name : 0;
								// if(MatchName == GlobalDebugTable.ProfileBlockName)
								{
									debug_region *Region = DebugState->CollationFrame->Regions + DebugState->CollationFrame->RegionsCount++;
									Region->ParentRegionName = MatchName;
									Region->Event = MatchingBlock->Event;
									Region->StartCyclesInFrame = (r32)(MatchingBlock->Event->Clock - DebugState->CollationFrame->BeginClock);
									Region->EndCyclesInFrame = (r32)(Event->Clock - DebugState->CollationFrame->BeginClock);
									Region->LaneIndex = Thread->LaneIndex;
									Region->ColorIndex = (u32)MatchingBlock->Event->Name >> 4;
								}

								Thread->OpenDebugBlocks = MatchingBlock->Parent;
								
								MatchingBlock->NextFree = DebugState->FirstFreeDebugBlock;
								DebugState->FirstFreeDebugBlock = MatchingBlock;
							}
						}
					} break;

					default:
					{
						Assert(DebugState->CollationFrame->ValuesCount < ArrayCount(DebugState->CollationFrame->ValuesEvents));
						DebugState->CollationFrame->ValuesEvents[DebugState->CollationFrame->ValuesCount++] = 
								&StoreEvent(DebugState, Event)->Event;
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

	// r32 CyclesPerFrame = 0.5f*86666666.0f;
	r32 CyclesPerFrame = 86666666.0f;
	r32 TableWidth = 300.0f;
	r32 BarSpacing = 5.0f;
	u32 LaneCount = DebugState->LaneCount;
	r32 LaneHeight = 10.0f;

	vec2 StartP = vec2(0.5f*-DebugState->BufferDim.x, DebugState->TextP.y - LaneHeight - BarSpacing);
	r32 MinY = StartP.y;

	debug_event *HotBlock = 0;

	debug_frame *Frame = &DebugState->FramesSentinel;
	for(u32 FrameIndex = 0;
		FrameIndex < 5;
		FrameIndex++)
	{
		Frame = Frame->Next;

		vec2 P = StartP - (r32)FrameIndex*vec2(0.0f, LaneCount*LaneHeight + BarSpacing);
		if(Frame->RegionsCount > 0)
		{
			for(u32 RegionIndex = 0;
				RegionIndex < Frame->RegionsCount;
				RegionIndex++)
			{
				debug_region *Region = Frame->Regions + RegionIndex;
				if(Region->ParentRegionName == GlobalDebugTable.ProfileBlockName)
				{
					UseShader(DebugState->QuadShader);
					glBindVertexArray(DebugState->GlyphVAO);

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
						debug_event *Block = Region->Event;
						char Buffer[256];
						_snprintf_s(Buffer, sizeof(Buffer), "%s(%u): %ucy", 
									Block->Name, Block->LineNumber,
									(u32)(Region->EndCyclesInFrame - Region->StartCyclesInFrame));

						DEBUGTextLineAt(DebugState, Buffer, 
										vec2(ScreenP + vec2(TableWidth - PxStart + BarSpacing, LaneHeight)));

						HotBlock = Block;
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

	rect2 ProfilingTableRect = RectMinMax(vec2(StartP.x, MinY), 
										  vec2(StartP.x + TableWidth, StartP.y + LaneHeight));
	if(IsInRect(ProfilingTableRect, MouseP))
	{
		if(WasDown(&Input->MouseRight))
		{
			GlobalDebugTable.ProfilePause = !GlobalDebugTable.ProfilePause || GlobalGamePause;
		}

		if(WasDown(&Input->MouseLeft))
		{
			if(HotBlock)
			{
				GlobalDebugTable.ProfileBlockName = HotBlock->Name;
			}
			else 
			{
				GlobalDebugTable.ProfileBlockName = 0;	
			}
		}
	}

	glBindVertexArray(0);
}

internal void
DEBUGBeginInteraction(debug_state *DebugState)
{
	DebugState->ActiveInteraction = DebugState->HotInteraction;
}

internal void
DEBUGEndInteraction(debug_state *DebugState)
{
	if(DebugState->ActiveInteraction == DebugState->NextHotInteraction)
	{
		switch(DebugState->ActiveInteraction->Type)
		{
			case DebugEvent_Group:
			{
				DebugState->ActiveInteraction->Value_bool32 = !DebugState->ActiveInteraction->Value_bool32;
			} break;

			case DebugEvent_bool32:
			{
				DebugState->ActiveInteraction->Value_bool32 = !DebugState->ActiveInteraction->Value_bool32;
			} break;
		}
	}

	DebugState->ActiveInteraction = 0;
}

internal void
DEBUGEventToText(debug_event *Event, char *Buffer, u32 BufferSize, u32 Depth = 0)
{
	char *Name = Event->Name;

	char *At = Buffer;
	for(u32 DepthIndex = 0;
		DepthIndex < Depth;
		DepthIndex++)
	{
		*At++ = ' ';
		*At++ = ' ';
		*At++ = ' ';
		*At++ = ' ';
	}

	if(Event->Type == DebugEvent_Group)
	{
		At += _snprintf_s(At, BufferSize - (At - Buffer), BufferSize - (At - Buffer), "%s%s", Event->Value_bool32 ? "-" : "+", Name);
	}
	else
	{
		At += _snprintf_s(At, BufferSize - (At - Buffer), BufferSize - (At - Buffer), "%s: ", Name);
	}
	switch(Event->Type)
	{
		case DebugEvent_bool32:
		{
			_snprintf_s(At, BufferSize - (At - Buffer), BufferSize - (At - Buffer), 
				"%s", Event->Value_bool32 ? "true" : "false");
		} break;

		case DebugEvent_r32:
		{
			_snprintf_s(At, BufferSize - (At - Buffer), BufferSize - (At - Buffer), 
						"%.6f", Event->Value_r32);
		} break;

		case DebugEvent_u32:
		{
			_snprintf_s(At, BufferSize - (At - Buffer), BufferSize - (At - Buffer), 
						"%u", Event->Value_u32);
		} break;	

		case DebugEvent_vec3:
		{
			_snprintf_s(At, BufferSize - (At - Buffer), BufferSize - (At - Buffer), 
						"vec3(%.2f, %.2f, %.2f)", Event->Value_vec3.x(), Event->Value_vec3.y(), Event->Value_vec3.z());
		} break;
	}
}

internal void
DEBUGAddMainMenuElement(debug_state *DebugState, debug_event *Event, vec2 MouseP, u32 Depth = 0)
{
	char Buffer[128];
	DEBUGEventToText(Event, Buffer, sizeof(Buffer), Depth);

	rect2 TextRegion = DEBUGGetTextRect(DebugState, Buffer, vec2(-0.5f*DebugState->BufferDim.x, DebugState->TextP.y));
	if(IsInRect(TextRegion, MouseP))
	{
		DebugState->NextHotInteraction = Event;
	}

	vec3 Color = vec3(1.0f, 1.0f, 1.0f);
	if(DebugState->HotInteraction == Event)
	{
		Color = vec3(1.0f, 1.0f, 0.0f);
	}
	DEBUGTextLine(DebugState, Buffer, TextRegion.Min.y, Color);
}

internal void
DEBUGRenderMainMenu(debug_state *DebugState, game_input *Input)
{
	vec2 MouseP = vec2(Input->MouseX, Input->MouseY);
	DebugState->NextHotInteraction = 0;

	debug_frame *MostRecentFrame = DebugState->FramesSentinel.Next;

	for(u32 GroupIndex = 0;
		GroupIndex < DebugValueEventGroup_Count;
		GroupIndex++)
	{
		DEBUGAddMainMenuElement(DebugState, &DebugState->Groups[GroupIndex], MouseP);

		if(DebugState->Groups[GroupIndex].Value_bool32)
		{
			u32 Depth = 1;

			for(u32 VariableIndex = 0;
				VariableIndex < DebugState->VariablesCount;
				VariableIndex++)
			{
				debug_event *Event = DebugState->VariablesEvents[VariableIndex];
				if(Event->Group == GroupIndex)
				{
					DEBUGAddMainMenuElement(DebugState, Event, MouseP, Depth);
				}
			}

			for(u32 ValueIndex = 0;
				ValueIndex < MostRecentFrame->ValuesCount;
				ValueIndex++)
			{
				debug_event *Event = MostRecentFrame->ValuesEvents[ValueIndex];
				if(Event->Group == GroupIndex)
				{
					char Buffer[64];
					DEBUGEventToText(Event, Buffer, sizeof(Buffer), Depth);
					DEBUGTextLine(DebugState, Buffer);
				}
			}
		}
	}

	if(DebugState->ActiveInteraction)
	{
		switch(DebugState->ActiveInteraction->Type)
		{
			case DebugEvent_r32:
			{
				r32 DisplacementY = MouseP.y - DebugState->LastMouseP.y;
				DebugState->ActiveInteraction->Value_r32 += 0.1f*DisplacementY;
			} break;
		}

		if(!Input->MouseLeft.EndedDown)
		{
			DEBUGEndInteraction(DebugState);
		}
	}
	else
	{
		DebugState->HotInteraction = DebugState->NextHotInteraction;
		if(WasDown(&Input->MouseLeft))
		{
			DEBUGBeginInteraction(DebugState);
		}
	}

	DEBUG_IF(Profiling, DebugTools)
	{
		if(DebugState->Groups[DebugValueEventGroup_DebugTools].Value_bool32)
		{
			DEBUGRenderRegions(DebugState, Input);
		}
	}

	DebugState->LastMouseP = MouseP;
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
		GlobalDebugTable.CurrentEventArrayIndex = !GlobalDebugTable.CurrentEventArrayIndex;
		u64 EventArrayIndex_EventIndex = AtomicExchangeU64(&GlobalDebugTable.EventArrayIndex_EventIndex,
													 	   (u64)GlobalDebugTable.CurrentEventArrayIndex << 32);

		EventArrayIndex = EventArrayIndex_EventIndex >> 32;
		GlobalDebugTable.EventCount = EventArrayIndex_EventIndex & 0xFFFFFFFF;
	}

	// if(!GlobalDebugTable.ProfilePause)
	// {
	// 	DebugState->ValuesCount = 0;
	// }
	
	if(DebugState && GameAssets)
	{
		DEBUGReset(DebugState, Memory, GameAssets, BufferWidth, BufferHeight);

		if(!GlobalDebugTable.ProfilePause)
		{
			DEBUGCollateEvents(DebugState, EventArrayIndex);
		}
	}

	DEBUGRenderMainMenu(DebugState, Input);

	debug_frame *MostRecentFrame = DebugState->FramesSentinel.Next;
	if(MostRecentFrame != &DebugState->FramesSentinel)
	{
		char Buffer[64];
		_snprintf_s(Buffer, sizeof(Buffer), "%.02fms/f", MostRecentFrame->MSElapsed);
		DEBUGTextLine(DebugState, Buffer);
	}

	u32 FrameAllocatorRemaining = (u32)((DebugState->FrameAllocator.Size - DebugState->FrameAllocator.Used) / 1024);
	DEBUG_VALUE(u32, FrameAllocatorRemainingKb, DebugTools, FrameAllocatorRemaining);

	game_state* GameState = (game_state*)Memory->PermanentStorage;
	u32 WorldAllocatorRemaining = (u32)((GameState->WorldAllocator.Size - GameState->WorldAllocator.Used) / 1024);
	DEBUG_VALUE(u32, WorldAllocatorRemainingKb, World, WorldAllocatorRemaining);
}