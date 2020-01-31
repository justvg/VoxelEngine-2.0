#version 330 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aColor;

const int CASCADES_COUNT = 3;

out vs_out
{	
	vec3 FragPosSim;
    float FragZView;
	vec3 Normal;
	vec4 Color;
	float Occlusion;
	vec4 FragPosLightSpace[CASCADES_COUNT];
	float ClipSpacePosZ;
} Output;

uniform mat4 Model = mat4(1.0);
uniform mat4 ViewProjection = mat4(1.0);
uniform mat4 LightSpaceMatrices[3];

uniform mat4 View = mat4(1.0);

// NOTE(georgy): This is for the situation when we use debug camera. Even if we use debug camera,
//  			 we want Output.ClipSpacePosZ to be as it is from our default camera, not debug one
uniform mat4 ViewProjectionForClipSpacePosZ = mat4(1.0);

void main()
{
	vec4 SimPos = Model * vec4(aPos.xyz, 1.0); 
	Output.FragPosSim = vec3(SimPos);
    Output.FragZView = -(View * SimPos).z;
	Output.Normal = vec3(0.0, 1.0, 0.0);
	Output.Color = aColor;
	Output.Occlusion = aPos.w;
	for(int CascadeIndex = 0;
		CascadeIndex < CASCADES_COUNT;
		CascadeIndex++)
	{
		Output.FragPosLightSpace[CascadeIndex] = LightSpaceMatrices[CascadeIndex] * SimPos;
	}

	gl_Position = ViewProjection * SimPos;
	// Output.ClipSpacePosZ  = gl_Position.z;
	Output.ClipSpacePosZ = (ViewProjectionForClipSpacePosZ * SimPos).z;
}