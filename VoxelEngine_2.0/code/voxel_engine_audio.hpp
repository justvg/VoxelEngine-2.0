#include "voxel_engine_audio.h"

internal void
InitializeAudioState(audio_state *AudioState, stack_allocator *Allocator)
{
	AudioState->Allocator = Allocator;
	AudioState->FirstPlayingSound = 0;
	AudioState->FirstFreePlayingSound = 0;
	
	AudioState->GlobalVolume = 1.0f;
}

internal playing_sound *
PlaySound(audio_state *AudioState, sound_id SoundID, world_position WorldP = InvalidPosition())
{
	if(!AudioState->FirstFreePlayingSound)
	{
		// TODO(georgy): Add audio allocator??
		AudioState->FirstFreePlayingSound = PushStruct(AudioState->Allocator, playing_sound);
	}

	playing_sound *PlayingSound = AudioState->FirstFreePlayingSound;
	AudioState->FirstFreePlayingSound = AudioState->FirstFreePlayingSound->NextFree;

	PlayingSound->Next = AudioState->FirstPlayingSound;
	AudioState->FirstPlayingSound = PlayingSound;

	PlayingSound->ID = SoundID;
	PlayingSound->Volume = { 1.0f, 1.0f };
	PlayingSound->SamplesPlayed = 0;

	PlayingSound->P = WorldP;

	return(PlayingSound);
}

internal void
ChangeVolume(playing_sound *Sound, vec2 Volume, r32 VolumeChangeInSeconds)
{
	if(VolumeChangeInSeconds <= 0.0f)
	{
		Sound->Volume = Sound->TargetVolume = Volume;
	}
	else
	{
		Sound->dVolume = (Sound->TargetVolume - Sound->Volume) * (1.0f / VolumeChangeInSeconds);
		Sound->TargetVolume = Volume;
	}
}

internal void
GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
{
	TIME_BLOCK;

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	audio_state *AudioState = &GameState->AudioState;
	temp_state *TempState = (temp_state *)Memory->TemporaryStorage;

	temporary_memory SoundMixerMemory = BeginTemporaryMemory(&TempState->Allocator);
	r32 *Real32Channel0 = PushArray(&TempState->Allocator, SoundBuffer->SampleCount, r32);
	r32 *Real32Channel1 = PushArray(&TempState->Allocator, SoundBuffer->SampleCount, r32);

#define ChannelsCount 2

	// NOTE(georgy): Clear the mixer channels
	{
		r32 *Dest0 = Real32Channel0;
		r32 *Dest1 = Real32Channel1;
		for(u32 SampleIndex = 0;
			SampleIndex < SoundBuffer->SampleCount;
			SampleIndex++)
		{
			*Dest0++ = 0.0f;
			*Dest1++ = 0.0f;
		}
	}

	for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound;
		*PlayingSoundPtr;
		)
	{
		playing_sound *PlayingSound = *PlayingSoundPtr;
		bool32 SoundFinished = false;

		loaded_sound *LoadedSound = GetSound(TempState->GameAssets, PlayingSound->ID);
		if(LoadedSound)
		{
			r32 *Dest0 = Real32Channel0;
			r32 *Dest1 = Real32Channel1;

			u32 TotalSamplesToMix = SoundBuffer->SampleCount;
			u32 SamplesRemainingInSound = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
			if(TotalSamplesToMix > SamplesRemainingInSound)
			{
				TotalSamplesToMix = SamplesRemainingInSound;
			}

			while(TotalSamplesToMix > 0)
			{
				u32 SamplesToMix = TotalSamplesToMix;
				vec2 Volume = PlayingSound->Volume;
				vec2 dVolume = (1.0f / SoundBuffer->SamplesPerSecond)*PlayingSound->dVolume;

				bool32 VolumeEnded[ChannelsCount] = {};
				u32 VolumeSampleCount[ChannelsCount] = {U32_MAX, U32_MAX};
				for(u32 ChannelIndex = 0;
					ChannelIndex < 2;
					ChannelIndex++)
				{
					if(dVolume.E[ChannelIndex] != 0.0f)
					{
						r32 VolumeDelta = PlayingSound->TargetVolume.E[ChannelIndex] - PlayingSound->Volume.E[ChannelIndex];
						VolumeSampleCount[ChannelIndex] = (u32)((VolumeDelta / dVolume.E[ChannelIndex]) + 0.5f);
					}
				}

				if((SamplesToMix >= VolumeSampleCount[0]) || (SamplesToMix >= VolumeSampleCount[1]))
				{
					if(VolumeSampleCount[0] == VolumeSampleCount[1])
					{
						VolumeEnded[0] = VolumeEnded[1] = true;
						SamplesToMix = VolumeSampleCount[0];
					}
					else
					{
						if(VolumeSampleCount[0] < VolumeSampleCount[1])
						{
							VolumeEnded[0] = true;
							SamplesToMix = VolumeSampleCount[0];
						}
						else
						{
							VolumeEnded[1] = true;
							SamplesToMix = VolumeSampleCount[1];
						}
					}
				}
				
                r32 DistanceVolume = 1.0f;
                if(GameState->GameMode == GameMode_World)
                {
                    // TODO(georgy): This is not "real" 3D audio and it is not physically correct
                    if(IsValid(PlayingSound->P))
                    {
                        game_mode_world *WorldMode = GameState->WorldMode;
                        r32 Distance = Length(Substract(&WorldMode->World, &WorldMode->Hero.Entity->P, &PlayingSound->P));
                        r32 ConstantTerm = 1.0f;
                        r32 LinearTerm = 0.09f;
                        r32 QuadraticTerm = 0.032f;
                        DistanceVolume = 1.0f / (ConstantTerm + LinearTerm*Distance + QuadraticTerm*Distance*Distance);;
                    }
                }

				for(u32 SampleIndex = 0;
					SampleIndex < SamplesToMix;
					SampleIndex++)
				{
					r32 SampleValue = LoadedSound->Samples[0][PlayingSound->SamplesPlayed + SampleIndex];
					*Dest0++ += DistanceVolume*AudioState->GlobalVolume*Volume.E[0]*SampleValue;
					*Dest1++ += DistanceVolume*AudioState->GlobalVolume*Volume.E[1]*SampleValue;

					Volume += dVolume;
				}

				PlayingSound->Volume = Volume;

				for(u32 ChannelIndex = 0;
					ChannelIndex < 2;
					ChannelIndex++)
				{
					if(VolumeEnded[ChannelIndex])
					{
						PlayingSound->Volume.E[ChannelIndex] = PlayingSound->TargetVolume.E[ChannelIndex];
						PlayingSound->dVolume.E[ChannelIndex] = 0.0f;
					}
				}

				PlayingSound->SamplesPlayed += SamplesToMix;

				TotalSamplesToMix -= SamplesToMix;
			}

			SoundFinished = (PlayingSound->SamplesPlayed == LoadedSound->SampleCount);
		}
		else
		{
			LoadSound(TempState->GameAssets, PlayingSound->ID);
		}

		if(SoundFinished)
		{
			*PlayingSoundPtr = PlayingSound->Next;

			PlayingSound->NextFree = AudioState->FirstFreePlayingSound;
			AudioState->FirstFreePlayingSound = PlayingSound;
		}
		else
		{
			PlayingSoundPtr = &PlayingSound->Next;
		}
	}

	// NOTE(georgy): Convert r32 buffers to i16
	{
		i16 *SampleDest = (i16 *)SoundBuffer->Samples;
		for(u32 SampleIndex = 0;
			SampleIndex < SoundBuffer->SampleCount;
			SampleIndex++)
		{
			*SampleDest++ = (i16)(Round(*Real32Channel0++));
			*SampleDest++ = (i16)(Round(*Real32Channel1++));
		}
	}

	EndTemporaryMemory(SoundMixerMemory);
}