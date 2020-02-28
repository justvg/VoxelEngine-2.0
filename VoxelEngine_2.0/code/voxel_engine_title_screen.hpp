#include "voxel_engine_title_screen.h"

internal void
PlayTitleScreen(game_state *GameState)
{
	SetGameMode(GameState, GameMode_TitleScreen);

	game_mode_title_screen *TitleScreen = PushStruct(&GameState->ModeAllocator, game_mode_title_screen);
	TitleScreen->t = 0.0f;

	GameState->TitleScreen = TitleScreen;
}

internal void
UpdateAndRenderTitleScreen(game_state *GameState, game_mode_title_screen *TitleScreen, game_input *Input)
{
	if(WasDown(&Input->MoveUp))
	{
		PlayWorld(GameState);
	}
	else
	{
		glClearColor(0.5f, 0.0f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}
