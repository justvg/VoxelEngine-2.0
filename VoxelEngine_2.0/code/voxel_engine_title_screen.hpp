#include "voxel_engine_title_screen.h"

internal void
PlayTitleScreen(game_state *GameState)
{
	SetGameMode(GameState, GameMode_TitleScreen);

	game_mode_title_screen *TitleScreen = PushStruct(&GameState->ModeAllocator, game_mode_title_screen);
	TitleScreen->t = 0.0f;

	GameState->TitleScreen = TitleScreen;
}

inline bool32
CheckForMetaInput(game_state *GameState, game_input *Input)
{
	bool32 Result = false;

	if(WasDown(&Input->MoveUp))
	{
		PlayWorld(GameState);
		Result = true;
	}
	else if(WasDown(&Input->Esc))
	{
		Input->QuitRequested = true;
		Result = true;
	}

	return(Result);
}

internal bool32
UpdateAndRenderTitleScreen(game_state *GameState, game_mode_title_screen *TitleScreen, game_input *Input,
						   u32 BufferWidth, u32 BufferHeight)
{
	bool32 ChangeMode = CheckForMetaInput(GameState, Input);

	if(!ChangeMode)
	{
		glViewport(0, 0, BufferWidth, BufferHeight);
		glClearColor(SquareRoot(0.5f), 0.0f, SquareRoot(0.5f), 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	return(ChangeMode);
}
