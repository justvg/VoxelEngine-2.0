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

uniform mat4 Model = mat4(1.0);
uniform mat4 ViewProjection = mat4(1.0);

void main()
{
	vec4 SimPos = Model * vec4(aPos, 1.0); 
	Output.FragPosSim = vec3(SimPos);
	Output.Normal = normalize(mat3(Model) * aNormal);
	Output.Color = aColor;

	Output.DirLight = normalize(DirLightDirection);

	gl_Position = ViewProjection * SimPos;
}