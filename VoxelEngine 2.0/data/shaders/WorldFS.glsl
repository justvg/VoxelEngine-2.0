#version 330 core
out vec4 FragColor;

in vs_out
{	
	vec3 FragPosSim;
	vec3 Normal;
	vec3 Color;

	vec3 DirLight;
} Input;

void main()
{
	vec3 LightDir = normalize(-Input.DirLight);
	vec3 Normal = normalize(Input.Normal);
	
	vec3 Ambient = 0.3 * Input.Color;
	vec3 Diffuse = max(dot(LightDir, Normal), 0.0) * Input.Color;

	vec3 FinalColor = Ambient + Diffuse;
	FragColor = vec4(FinalColor, 1.0);
}