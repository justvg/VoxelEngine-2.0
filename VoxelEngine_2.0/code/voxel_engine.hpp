#include "voxel_engine.h"
#include "voxel_engine_asset.hpp"
#include "voxel_engine_render.hpp"
#include "voxel_engine_world.hpp"
#include "voxel_engine_audio.hpp"
#include "voxel_engine_sim_region.hpp"
#include "voxel_engine_world_mode.hpp"
#include "voxel_engine_title_screen.hpp"

internal void
SetGameMode(game_state *GameState, game_mode GameMode)
{
	if((GameState->GameMode == GameMode_World) &&
	   (GameMode != GameMode_World))
	{
		game_mode_world *WorldMode = GameState->WorldMode;
		UnloadAllChunks(&WorldMode->World);

		// TODO(georgy): All of these deallocations should be more convenient to use!!
		// I don't want to delete shaders? Store them anywhere?

		FreeDynamicArray(&WorldMode->HeroCollision->VerticesP);
		FreeDynamicArray(&WorldMode->FireballCollision->VerticesP);
		FreeDynamicArray(&WorldMode->TreeCollision->VerticesP);
		FreeDynamicArray(&WorldMode->TESTCubeCollision->VerticesP);

		glDeleteProgram(WorldMode->CharacterShader.ID);
		glDeleteProgram(WorldMode->WorldShader.ID);
		glDeleteProgram(WorldMode->WaterShader.ID);
		glDeleteProgram(WorldMode->HitpointsShader.ID);
		glDeleteProgram(WorldMode->BlockParticleShader.ID);
		glDeleteProgram(WorldMode->WorldDepthShader.ID);
		glDeleteProgram(WorldMode->CharacterDepthShader.ID);
		glDeleteProgram(WorldMode->BlockParticleDepthShader.ID);
		glDeleteProgram(WorldMode->UIQuadShader.ID);
		glDeleteProgram(WorldMode->UIGlyphShader.ID);
		glDeleteProgram(WorldMode->FramebufferScreenShader.ID);

		glDeleteBuffers(ArrayCount(WorldMode->UBOs), WorldMode->UBOs);;

		glDeleteFramebuffers(1, &WorldMode->ShadowMapFBO);

		glDeleteTextures(1, &WorldMode->ShadowMapsArray);
		glDeleteTextures(1, &WorldMode->ShadowNoiseTexture);

		glDeleteBuffers(1, &WorldMode->ParticleGenerator.VAO);
		glDeleteBuffers(1, &WorldMode->ParticleGenerator.VBO);
		glDeleteBuffers(1, &WorldMode->ParticleGenerator.SimPVBO);
		glDeleteBuffers(1, &WorldMode->ParticleGenerator.ColorVBO);
		glDeleteBuffers(1, &WorldMode->ParticleGenerator.ScaleVBO);

		glDeleteBuffers(1, &WorldMode->CubeVAO);
		glDeleteBuffers(1, &WorldMode->CubeVBO);
		glDeleteBuffers(1, &WorldMode->QuadVAO);
		glDeleteBuffers(1, &WorldMode->QuadVBO);
		glDeleteBuffers(1, &WorldMode->UIQuadVAO);
		glDeleteBuffers(1, &WorldMode->UIQuadVBO);
		glDeleteBuffers(1, &WorldMode->UIGlyphVAO);
		glDeleteBuffers(1, &WorldMode->UIGlyphVBO);
	}

	GameState->ModeAllocator.Used = 0;
	GameState->GameMode = GameMode;
}

internal void
GameUpdate(game_memory *Memory, game_input *Input, int BufferWidth, int BufferHeight,  
		   bool32 GameProfilingPause, bool32 DebugCamera, game_input *DebugCameraInput)
{
	Platform = Memory->PlatformAPI;

	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!GameState->IsInitialized)
	{
		stack_allocator TotalAllocator;
		InitializeStackAllocator(&TotalAllocator, Memory->PermanentStorageSize - sizeof(game_state),
								 (u8 *)Memory->PermanentStorage + sizeof(game_state));
		
		SubAllocator(&GameState->AudioAllocator, &TotalAllocator, Megabytes(16));
		SubAllocator(&GameState->ModeAllocator, &TotalAllocator, GetAllocatorSizeRemaining(&TotalAllocator));

		InitializeAudioState(&GameState->AudioState, &GameState->AudioAllocator);

		PlayTitleScreen(GameState);

		GameState->IsInitialized = true;
	}

	Assert(sizeof(temp_state) <= Memory->TemporaryStorageSize);
	temp_state *TempState = (temp_state *)Memory->TemporaryStorage;
	if(!TempState->IsInitialized)
	{
		InitializeStackAllocator(&TempState->Allocator, Memory->TemporaryStorageSize - sizeof(temp_state),
														(u8 *)Memory->TemporaryStorage + sizeof(temp_state));

		TempState->JobSystemQueue = Memory->JobSystemQueue;
		// TempState->GameAssets = AllocateGameAssets(TempState, &TempState->Allocator, 60000);
		TempState->GameAssets = AllocateGameAssets(TempState, &TempState->Allocator, Megabytes(64));

		world_position TESTSOUNDP = {};
		TESTSOUNDP.ChunkY = 1;
		TESTSOUNDP.Offset = vec3(0.3f, 5.0f, 3.0f);
		playing_sound *Music = PlaySound(&GameState->AudioState, GetFirstSoundFromType(TempState->GameAssets, AssetType_Music));
		// ChangeVolume(Music, vec2(0.5f, 0.5f), 10.0f);

		TempState->IsInitialized = true;
	}

	DEBUG_VARIABLE(r32, GlobalVolume, DebugTools);
	GameState->AudioState.GlobalVolume = GlobalVolume;

	temporary_memory SimulationAndRenderMemory = BeginTemporaryMemory(&TempState->Allocator);

	bool32 ChangeMode = false;
	do
	{
		switch(GameState->GameMode)
		{
			case GameMode_TitleScreen:
			{
				ChangeMode = UpdateAndRenderTitleScreen(GameState, GameState->TitleScreen, Input, 
														BufferWidth, BufferHeight);
			} break;

			case GameMode_World:
			{
				ChangeMode = UpdateAndRenderWorld(GameState, GameState->WorldMode, TempState, Input,
				  					 			  BufferWidth, BufferHeight, GameProfilingPause, DebugCamera, DebugCameraInput);
			} break;
		}
	} while(ChangeMode);

	EndTemporaryMemory(SimulationAndRenderMemory);

	UnloadAssetsIfNecessary(TempState->GameAssets);
}
