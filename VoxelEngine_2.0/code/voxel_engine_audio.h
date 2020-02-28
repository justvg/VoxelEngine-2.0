#pragma once

struct playing_sound
{
	sound_id ID;
	vec2 Volume;
	vec2 dVolume;
	vec2 TargetVolume;
	u32 SamplesPlayed;

	world_position P;

	union
	{
		playing_sound *Next;
		playing_sound *NextFree;
	};
};

struct audio_state
{
	stack_allocator *Allocator;
	playing_sound *FirstPlayingSound;
	playing_sound *FirstFreePlayingSound;
	
	r32 GlobalVolume;
};
