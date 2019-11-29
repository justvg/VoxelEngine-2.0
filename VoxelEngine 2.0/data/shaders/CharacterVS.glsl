#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

const int CASCADES_COUNT = 3;

out vs_out
{	
	vec3 FragPosSim;
	vec3 Normal;
	vec3 Color;
	vec4 FragPosLightSpace[CASCADES_COUNT];
	float ClipSpacePosZ;
} Output;

uniform mat4 BoneTransformations[7];
uniform int BoneID;

uniform mat4 Model = mat4(1.0);
uniform mat4 ViewProjection = mat4(1.0);
uniform mat4 LightSpaceMatrices[3];

void main()
{
	vec4 LocalP = BoneTransformations[BoneID] * vec4(aPos, 1.0);
	vec3 LocalNormal = normalize(mat3(BoneTransformations[BoneID]) * aNormal);

	vec4 SimPos = Model * LocalP; 
	Output.FragPosSim = vec3(SimPos);
	Output.Normal = normalize(mat3(Model) * LocalNormal);
	Output.Color = aColor;
	for(int CascadeIndex = 0;
		CascadeIndex < CASCADES_COUNT;
		CascadeIndex++)
	{
		Output.FragPosLightSpace[CascadeIndex] = LightSpaceMatrices[CascadeIndex] * SimPos;
	}

	gl_Position = ViewProjection * SimPos;
	Output.ClipSpacePosZ = gl_Position.z;
}