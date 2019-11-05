#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out vs_out
{	
	vec3 FragPosSim;
	vec3 Normal;
	vec3 Color;

	vec3 DirLight;
} Output;

const vec3 DirLightDirection = normalize(vec3(0.9, -0.5, -0.3));

uniform mat4 BoneTransformations[7];
uniform int BoneID;

uniform mat4 Model = mat4(1.0);
uniform mat4 ViewProjection = mat4(1.0);

void main()
{
	vec4 LocalP = BoneTransformations[BoneID] * vec4(aPos, 1.0);
	vec3 LocalNormal = normalize(mat3(BoneTransformations[BoneID]) * aNormal);

	vec4 SimPos = Model * LocalP; 
	Output.FragPosSim = vec3(SimPos);
	Output.Normal = normalize(mat3(Model) * LocalNormal);
	Output.Color = aColor;

	Output.DirLight = normalize(DirLightDirection);

	gl_Position = ViewProjection * SimPos;
}