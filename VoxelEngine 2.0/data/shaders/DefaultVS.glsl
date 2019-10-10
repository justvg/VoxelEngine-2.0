#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vs_out
{	
	vec3 FragPosView;
	vec3 Normal;
	vec3 DirLight;
} Output;

const vec3 DirLightDirection = normalize(vec3(0.9, -0.5, -0.3));

uniform mat4 Model = mat4(1.0);
uniform mat4 View = mat4(1.0);
uniform mat4 Projection = mat4(1.0);

void main()
{
	vec4 ViewPos = View * Model * vec4(aPos, 1.0); 
	Output.FragPosView = vec3(ViewPos);
	Output.Normal = normalize(mat3(View * Model) * aNormal);
	Output.DirLight = normalize(mat3(View) * DirLightDirection);

	gl_Position = Projection * ViewPos;
}