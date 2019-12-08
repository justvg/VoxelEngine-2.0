#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 SimP;
layout (location = 3) in vec3 Color;

const int CASCADES_COUNT = 3;

out vs_out
{	
	vec3 FragPosSim;
	vec3 Normal;
	vec3 Color;
	vec4 FragPosLightSpace[CASCADES_COUNT];
	float ClipSpacePosZ;
} Output;

uniform mat4 ViewProjection = mat4(1.0);
uniform mat4 LightSpaceMatrices[3];

void main()
{
	vec4 SimPos = vec4(aPos + SimP, 1.0); 
	Output.FragPosSim = vec3(SimPos);
	Output.Normal = normalize(aNormal);
	Output.Color = Color;
	for(int CascadeIndex = 0;
		CascadeIndex < CASCADES_COUNT;
		CascadeIndex++)
	{
		Output.FragPosLightSpace[CascadeIndex] = LightSpaceMatrices[CascadeIndex] * SimPos;
	}

	gl_Position = ViewProjection * SimPos;
	Output.ClipSpacePosZ = gl_Position.z;
}