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

inline void
InitializeDefaultAnimations(animation *CharacterAnimations)
{
	CharacterAnimations[CharacterAnimation_Idle].DurationInSeconds = 4.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrameCount = 4;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].TimeStampInSeconds = 0.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Translation[CharacterBone_Head] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Translation[CharacterBone_Shoulders] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Translation[CharacterBone_Body] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Translation[CharacterBone_Hand_Right] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Translation[CharacterBone_Hand_Left] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Translation[CharacterBone_Foot_Right] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Translation[CharacterBone_Foot_Left] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Rotation[CharacterBone_Head] = Quaternion(1.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Rotation[CharacterBone_Shoulders] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Rotation[CharacterBone_Body] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Rotation[CharacterBone_Hand_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Rotation[CharacterBone_Hand_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Rotation[CharacterBone_Foot_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Rotation[CharacterBone_Foot_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Scaling[CharacterBone_Head] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Scaling[CharacterBone_Shoulders] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Scaling[CharacterBone_Body] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Scaling[CharacterBone_Hand_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Scaling[CharacterBone_Hand_Left] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Scaling[CharacterBone_Foot_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0].Scaling[CharacterBone_Foot_Left] = 1.0f;

	r32 W = Cos(DEG2RAD(5.0f)/2.0f);
	vec3 V = Sin(DEG2RAD(5.0f)/2.0f)*vec3(0.0f, 1.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].TimeStampInSeconds = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Translation[CharacterBone_Head] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Translation[CharacterBone_Shoulders] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Translation[CharacterBone_Body] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Translation[CharacterBone_Hand_Right] = vec3(0.0f, 0.02f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Translation[CharacterBone_Hand_Left] = vec3(0.0f, 0.02f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Translation[CharacterBone_Foot_Right] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Translation[CharacterBone_Foot_Left] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Rotation[CharacterBone_Head] = Quaternion(W, V);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Rotation[CharacterBone_Shoulders] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Rotation[CharacterBone_Body] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Rotation[CharacterBone_Hand_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Rotation[CharacterBone_Hand_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Rotation[CharacterBone_Foot_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Rotation[CharacterBone_Foot_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Scaling[CharacterBone_Head] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Scaling[CharacterBone_Shoulders] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Scaling[CharacterBone_Body] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Scaling[CharacterBone_Hand_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Scaling[CharacterBone_Hand_Left] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Scaling[CharacterBone_Foot_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[1].Scaling[CharacterBone_Foot_Left] = 1.0f;

	W = Cos(DEG2RAD(-5.0f)/2.0f);
	V = Sin(DEG2RAD(-5.0f)/2.0f)*vec3(0.0f, 1.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].TimeStampInSeconds = 3.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Translation[CharacterBone_Head] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Translation[CharacterBone_Shoulders] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Translation[CharacterBone_Body] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Translation[CharacterBone_Hand_Right] = vec3(0.0f, 0.02f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Translation[CharacterBone_Hand_Left] = vec3(0.0f, 0.02f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Translation[CharacterBone_Foot_Right] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Translation[CharacterBone_Foot_Left] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Rotation[CharacterBone_Head] = Quaternion(W, V);
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Rotation[CharacterBone_Shoulders] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Rotation[CharacterBone_Body] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Rotation[CharacterBone_Hand_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Rotation[CharacterBone_Hand_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Rotation[CharacterBone_Foot_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Rotation[CharacterBone_Foot_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Scaling[CharacterBone_Head] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Scaling[CharacterBone_Shoulders] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Scaling[CharacterBone_Body] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Scaling[CharacterBone_Hand_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Scaling[CharacterBone_Hand_Left] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Scaling[CharacterBone_Foot_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[2].Scaling[CharacterBone_Foot_Left] = 1.0f;

	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[3] = CharacterAnimations[CharacterAnimation_Idle].KeyFrames[0];
	CharacterAnimations[CharacterAnimation_Idle].KeyFrames[3].TimeStampInSeconds = 4.0f;


	r32 PositiveRotW = Cos(DEG2RAD(90.0f)/2.0f);
	vec3 PositiveRotV = Sin(DEG2RAD(90.0f)/2.0f)*vec3(1.0f, 0.0f, 0.0f);
	r32 NegativeRotW = Cos(DEG2RAD(-90.0f)/2.0f);
	vec3 NegativeRotV = Sin(DEG2RAD(-90.0f)/2.0f)*vec3(1.0f, 0.0f, 0.0f);
	W = Cos(DEG2RAD(20.0f)/2.0f);
	V = Sin(DEG2RAD(20.0f)/2.0f)*vec3(1.0f, 0.0f, 0.0f);

	CharacterAnimations[CharacterAnimation_Run].DurationInSeconds = 2.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrameCount = 3;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].TimeStampInSeconds = 0.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Head] = vec3(0.0f, -0.065f, 0.15f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Shoulders] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Body] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Hand_Right] = vec3(0.0f, 0.0f, -0.1f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Hand_Left] = vec3(0.0f, 0.0f, 0.1f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Foot_Right] = vec3(0.0f, 0.0f, 0.15f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Translation[CharacterBone_Foot_Left] = vec3(0.0f, 0.1f, -0.25f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Rotation[CharacterBone_Head] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Rotation[CharacterBone_Shoulders] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Rotation[CharacterBone_Body] = Quaternion(W, V);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Rotation[CharacterBone_Hand_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Rotation[CharacterBone_Hand_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Rotation[CharacterBone_Foot_Right] = Quaternion(PositiveRotW, PositiveRotV);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Rotation[CharacterBone_Foot_Left] = Quaternion(NegativeRotW, NegativeRotV);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Head] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Shoulders] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Body] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Hand_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Hand_Left] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Foot_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[0].Scaling[CharacterBone_Foot_Left] = 1.0f;
	
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].TimeStampInSeconds = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Head] = vec3(0.0f, -0.05f, 0.15f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Shoulders] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Body] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Hand_Right] = vec3(0.0f, 0.0f, 0.1f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Hand_Left] = vec3(0.0f, 0.0f, -0.1f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Foot_Right] = vec3(0.0f, 0.1f, -0.25f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Translation[CharacterBone_Foot_Left] = vec3(0.0f, 0.0f, 0.15f);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Rotation[CharacterBone_Head] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Rotation[CharacterBone_Shoulders] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Rotation[CharacterBone_Body] = Quaternion(W, V);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Rotation[CharacterBone_Hand_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Rotation[CharacterBone_Hand_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Rotation[CharacterBone_Foot_Right] = Quaternion(NegativeRotW, NegativeRotV);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Rotation[CharacterBone_Foot_Left] = Quaternion(PositiveRotW, PositiveRotV);
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Scaling[CharacterBone_Head] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Scaling[CharacterBone_Shoulders] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Scaling[CharacterBone_Body] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Scaling[CharacterBone_Hand_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Scaling[CharacterBone_Hand_Left] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Scaling[CharacterBone_Foot_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[1].Scaling[CharacterBone_Foot_Left] = 1.0f;

	CharacterAnimations[CharacterAnimation_Run].KeyFrames[2] = CharacterAnimations[CharacterAnimation_Run].KeyFrames[0];
	CharacterAnimations[CharacterAnimation_Run].KeyFrames[2].TimeStampInSeconds = 2.0f;

	W = Cos(DEG2RAD(30.0f)/2.0f);
	V = Sin(DEG2RAD(30.0f)/2.0f)*vec3(1.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Jump].DurationInSeconds = 1.0f;
	CharacterAnimations[CharacterAnimation_Jump].KeyFrameCount = 2;
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].TimeStampInSeconds = 0.0f;
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Translation[CharacterBone_Head] = vec3(0.0f, -0.1f, 0.17f);
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Translation[CharacterBone_Shoulders] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Translation[CharacterBone_Body] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Translation[CharacterBone_Hand_Right] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Translation[CharacterBone_Hand_Left] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Translation[CharacterBone_Foot_Right] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Translation[CharacterBone_Foot_Left] = vec3(0.0f, 0.0f, 0.0f);
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Rotation[CharacterBone_Head] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Rotation[CharacterBone_Shoulders] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Rotation[CharacterBone_Body] = Quaternion(W, V);
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Rotation[CharacterBone_Hand_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Rotation[CharacterBone_Hand_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Rotation[CharacterBone_Foot_Right] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Rotation[CharacterBone_Foot_Left] = Quaternion(0.0f, vec3(0.0f, 0.0f, 0.0f));
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Scaling[CharacterBone_Head] = 1.0f;
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Scaling[CharacterBone_Shoulders] = 1.0f;
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Scaling[CharacterBone_Body] = 1.0f;
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Scaling[CharacterBone_Hand_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Scaling[CharacterBone_Hand_Left] = 1.0f;
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Scaling[CharacterBone_Foot_Right] = 1.0f;
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0].Scaling[CharacterBone_Foot_Left] = 1.0f;

	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[1] = CharacterAnimations[CharacterAnimation_Jump].KeyFrames[0];
	CharacterAnimations[CharacterAnimation_Jump].KeyFrames[1].TimeStampInSeconds = 1.0f;
}
