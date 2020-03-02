#include "voxel_engine_animation.h"

internal void
UpdateCharacterAnimations(sim_entity *Entity, move_spec *MoveSpec, r32 dt)
{
	r32 AnimationTimeStep = 0.0f;
	
	character_animation_type DesiredAnimation = CharacterAnimation_Idle;

	if(Length(vec3(Entity->dP.x(), 0.0f, Entity->dP.z())) > 0.12f)
	{
		DesiredAnimation = CharacterAnimation_Run;
	}

	if(!IsSet(Entity, EntityFlag_OnGround))
	{
		// DesiredAnimation = CharacterAnimation_Run;
		DesiredAnimation = CharacterAnimation_Jump;
	}

	if(Entity->Sword.SimPtr && (Entity->Sword.SimPtr->TimeLimit > 0.0f))
	{
		DesiredAnimation = CharacterAnimation_SwordAttack;

		MoveSpec->Speed *= 0.35f;
	}

	if(Entity->AnimationState.Type != DesiredAnimation)
	{
		Entity->AnimationState.Type = DesiredAnimation;
		Entity->AnimationState.Time = 0.0f;
	}
	else
	{
		AnimationTimeStep = dt;
		if(Entity->AnimationState.Type == CharacterAnimation_Run)
		{
			AnimationTimeStep = 1.5f*Length(vec3(Entity->dP.x(), 0.0f, Entity->dP.z()))*dt;
		}
	}

	Entity->AnimationState.Time += AnimationTimeStep;
}

internal void
GetCharacterTransformationsForEntity(game_mode_world *WorldMode, sim_entity *Entity, mat4 *BoneTransformations)
{
	for(u32 BoneIndex = 0;
		BoneIndex < CharacterBone_Count;
		BoneIndex++)
	{
		BoneTransformations[BoneIndex] = GetBoneForEntity(WorldMode->CharacterAnimations, &Entity->AnimationState, 
														  (character_bone_id)BoneIndex);
	}

	for(u32 BoneIndex = 0;
		BoneIndex < CharacterBone_Count;
		BoneIndex++)
	{
		if(BoneIndex != CharacterBone_Body)
		{
			BoneTransformations[BoneIndex] = BoneTransformations[BoneIndex] * BoneTransformations[CharacterBone_Body];
		}
	}
}
