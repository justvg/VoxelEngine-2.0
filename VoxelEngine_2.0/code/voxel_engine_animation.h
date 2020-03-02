#pragma once

enum character_bone_id
{
	CharacterBone_Head,
	CharacterBone_Shoulders,
	CharacterBone_Body,
	CharacterBone_Hand_Right,
	CharacterBone_Hand_Left,
	CharacterBone_Foot_Right,
	CharacterBone_Foot_Left,

	CharacterBone_Count
};

enum character_animation_type
{
	CharacterAnimation_Null,

	CharacterAnimation_Idle,
	CharacterAnimation_Run,
	CharacterAnimation_Jump,
	CharacterAnimation_SwordAttack,

	CharacterAnimation_Count
};

struct animation_key_frame
{
	r32 TimeStampInSeconds;
	vec3 Translation[CharacterBone_Count];
	quaternion Rotation[CharacterBone_Count];
	// NOTE(georgy): If I want vec3 here, don't forget that normals should be transformed differently when nonuniform scale is used 
	r32 Scaling[CharacterBone_Count];
};

struct animation
{
	r32 DurationInSeconds;
	u32 KeyFrameCount;
	animation_key_frame KeyFrames[4];
};

struct entity_animation_state
{
	character_animation_type Type;
	r32 Time;
};

internal mat4 
GetBoneForEntity(animation *Animations, entity_animation_state *EntityAnimationState, 
				 character_bone_id BoneID)
{
	mat4 BoneTransform;
	if(EntityAnimationState->Type)
	{
		animation *Animation = Animations + EntityAnimationState->Type;

		r32 DurationInSeconds = Animation->DurationInSeconds;
		r32 TimeInAnimation = Real32Modulo(EntityAnimationState->Time, DurationInSeconds);

		u32 FirstKeyFrameIndex = 0;
		for(u32 KeyFrameIndex = 0;
			KeyFrameIndex < Animation->KeyFrameCount - 1;
			KeyFrameIndex++)
		{
			if(TimeInAnimation < Animation->KeyFrames[KeyFrameIndex + 1].TimeStampInSeconds)
			{
				FirstKeyFrameIndex = KeyFrameIndex;
				break;
			}
		}
		u32 NextKeyFrameIndex = FirstKeyFrameIndex + 1;

		animation_key_frame *FirstKeyFrame = &Animation->KeyFrames[FirstKeyFrameIndex];
		animation_key_frame *NextKeyFrame = &Animation->KeyFrames[NextKeyFrameIndex];

		r32 DeltaTime = TimeInAnimation - FirstKeyFrame->TimeStampInSeconds;
		r32 t = DeltaTime / (NextKeyFrame->TimeStampInSeconds - FirstKeyFrame->TimeStampInSeconds);
		vec3 Translation = Lerp(FirstKeyFrame->Translation[BoneID], NextKeyFrame->Translation[BoneID], t);
		quaternion Rotation = Slerp(FirstKeyFrame->Rotation[BoneID], NextKeyFrame->Rotation[BoneID], t);
		r32 Scaling = Lerp(FirstKeyFrame->Scaling[BoneID], NextKeyFrame->Scaling[BoneID], t);
		
		BoneTransform = Translate(Translation) * QuaternionToMatrix(Rotation) * Scale(Scaling);
	}
	else
	{
		BoneTransform = Identity(1.0f);
	}

	return(BoneTransform);
}

internal animation_key_frame
DefaultCharacterKeyFrame(r32 TimeStampInSeconds = 0.0f)
{
	animation_key_frame KeyFrame = {};

	KeyFrame.TimeStampInSeconds = TimeStampInSeconds;
	KeyFrame.Translation[CharacterBone_Head] = vec3(0.0f, 0.0f, 0.0f);
	KeyFrame.Translation[CharacterBone_Shoulders] = vec3(0.0f, 0.0f, 0.0f);
	KeyFrame.Translation[CharacterBone_Body] = vec3(0.0f, 0.0f, 0.0f);
	KeyFrame.Translation[CharacterBone_Hand_Right] = vec3(0.0f, 0.0f, 0.0f);
	KeyFrame.Translation[CharacterBone_Hand_Left] = vec3(0.0f, 0.0f, 0.0f);
	KeyFrame.Translation[CharacterBone_Foot_Right] = vec3(0.0f, 0.0f, 0.0f);
	KeyFrame.Translation[CharacterBone_Foot_Left] = vec3(0.0f, 0.0f, 0.0f);
	KeyFrame.Rotation[CharacterBone_Head] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	KeyFrame.Rotation[CharacterBone_Shoulders] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	KeyFrame.Rotation[CharacterBone_Body] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	KeyFrame.Rotation[CharacterBone_Hand_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	KeyFrame.Rotation[CharacterBone_Hand_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	KeyFrame.Rotation[CharacterBone_Foot_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	KeyFrame.Rotation[CharacterBone_Foot_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	KeyFrame.Scaling[CharacterBone_Head] = 1.0f;
	KeyFrame.Scaling[CharacterBone_Shoulders] = 1.0f;
	KeyFrame.Scaling[CharacterBone_Body] = 1.0f;
	KeyFrame.Scaling[CharacterBone_Hand_Right] = 1.0f;
	KeyFrame.Scaling[CharacterBone_Hand_Left] = 1.0f;
	KeyFrame.Scaling[CharacterBone_Foot_Right] = 1.0f;
	KeyFrame.Scaling[CharacterBone_Foot_Left] = 1.0f;

	return(KeyFrame);
}

inline void
InitializeDefaultCharacterAnimations(animation *CharacterAnimations)
{
	CharacterAnimations[CharacterAnimation_Idle].DurationInSeconds = 4.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrameCount = 4;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0] = DefaultCharacterKeyFrame();

	quaternion IdleHeadRotation = QuaternionAngleAxis(5.0f, vec3(0.0f, 1.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1] = DefaultCharacterKeyFrame(1.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Translation[CharacterBone_Hand_Right] = vec3(0.0f, 0.02f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Translation[CharacterBone_Hand_Left] = vec3(0.0f, 0.02f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Rotation[CharacterBone_Head] = IdleHeadRotation;

	IdleHeadRotation = QuaternionAngleAxis(-5.0f, vec3(0.0f, 1.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2] = DefaultCharacterKeyFrame(3.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Translation[CharacterBone_Hand_Right] = vec3(0.0f, 0.02f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Translation[CharacterBone_Hand_Left] = vec3(0.0f, 0.02f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Rotation[CharacterBone_Head] = IdleHeadRotation;

	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[3] = CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0];
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[3].TimeStampInSeconds = 4.0f;



	quaternion RunFootPositiveRotation = QuaternionAngleAxis(90.0f, vec3(1.0f, 0.0f, 0.0f));
	quaternion RunFootNegativeRotation = QuaternionAngleAxis(-90.0f, vec3(1.0f, 0.0f, 0.0f));
	quaternion RunBodyRotation = QuaternionAngleAxis(20.0f, vec3(1.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Run].DurationInSeconds = 2.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrameCount = 3;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0] = DefaultCharacterKeyFrame();
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Head] = vec3(0.0f, -0.065f, 0.15f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Hand_Right] = vec3(0.0f, 0.0f, -0.1f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Hand_Left] = vec3(0.0f, 0.0f, 0.1f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Foot_Right] = vec3(0.0f, 0.0f, 0.15f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Foot_Left] = vec3(0.0f, 0.1f, -0.25f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Rotation[CharacterBone_Body] = RunBodyRotation;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Rotation[CharacterBone_Foot_Right] = RunFootPositiveRotation;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Rotation[CharacterBone_Foot_Left] = RunFootNegativeRotation;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Head] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Shoulders] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Body] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Hand_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Hand_Left] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Foot_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Foot_Left] = 1.0f;
	
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1] = DefaultCharacterKeyFrame(1.0f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Head] = vec3(0.0f, -0.05f, 0.15f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Hand_Right] = vec3(0.0f, 0.0f, 0.1f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Hand_Left] = vec3(0.0f, 0.0f, -0.1f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Foot_Right] = vec3(0.0f, 0.1f, -0.25f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Foot_Left] = vec3(0.0f, 0.0f, 0.15f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Rotation[CharacterBone_Body] = RunBodyRotation;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Rotation[CharacterBone_Foot_Right] = RunFootNegativeRotation;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Rotation[CharacterBone_Foot_Left] = RunFootPositiveRotation;
	
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[2] = CharacterAnimations[CharacterAnimation_Run].KeyFrames[0];
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[2].TimeStampInSeconds = 2.0f;



	quaternion JumpHeadRotation = QuaternionAngleAxis(30.0f, vec3(1.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Jump].DurationInSeconds = 1.0f;
	CharacterAnimations[CharacterAnimation_Jump].KeyFrameCount = 2;
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0] = DefaultCharacterKeyFrame();
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Translation[CharacterBone_Head] = vec3(0.0f, -0.1f, 0.17f);
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Rotation[CharacterBone_Body] = JumpHeadRotation;
	
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[1] = CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0];
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[1].TimeStampInSeconds = 1.0f;



	quaternion SwordAttackBodyStartRotation = QuaternionAngleAxis(15.0f, vec3(0.0f, 1.0f, 0.0f));
	quaternion SwordAttackBodyEndRotation = QuaternionAngleAxis(-80.0f, vec3(0.0f, 1.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_SwordAttack].DurationInSeconds = 0.4f;
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrameCount = 4;
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[0] = DefaultCharacterKeyFrame();
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[0].Rotation[CharacterBone_Body] = SwordAttackBodyStartRotation;

	quaternion SwordRotation = QuaternionAngleAxis(90.0f, vec3(1.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[1] = DefaultCharacterKeyFrame(0.1f);
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[1].Rotation[CharacterBone_Hand_Left] = SwordRotation;

	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[2] = DefaultCharacterKeyFrame(0.25f);
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[2].Translation[CharacterBone_Hand_Right] = vec3(0.2f, 0.0f, -0.33f);
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[2].Translation[CharacterBone_Hand_Left] = vec3(-0.5f, 0.0f, 0.3f);
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[2].Translation[CharacterBone_Foot_Right] = 0.5f*vec3(0.2f, 0.0f, -0.33f);
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[2].Translation[CharacterBone_Foot_Left] = 0.5f*vec3(-0.2f, 0.0f, 0.33f);
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[2].Rotation[CharacterBone_Body] = SwordAttackBodyEndRotation;
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[2].Rotation[CharacterBone_Hand_Left] = SwordRotation;

	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[3] = CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[0];
	CharacterAnimations[CharacterAnimation_SwordAttack].KeyFrames[3].TimeStampInSeconds = 0.4f;
}