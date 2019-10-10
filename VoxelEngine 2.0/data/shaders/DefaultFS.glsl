#version 330 core
out vec4 FragColor;

in vs_out
{	
	vec3 FragPosView;
	vec3 Normal;
	vec3 DirLight;
} Input;

void main()
{
	vec3 LightDir = normalize(-Input.DirLight);
	vec3 Normal = normalize(Input.Normal);
	
	vec3 Ambient = 0.3 * vec3(0.53, 0.53, 0.53);
	vec3 Diffuse = max(dot(LightDir, Normal), 0.0) * vec3(0.53, 0.53, 0.53);

	vec3 FinalColor = Ambient + Diffuse;
	FragColor = vec4(FinalColor, 1.0);
}