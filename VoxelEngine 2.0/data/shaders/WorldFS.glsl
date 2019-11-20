#version 330 core
out vec4 FragColor;

in vs_out
{	
	vec3 FragPosSim;
	vec3 Normal;
	vec3 Color;
	float Occlusion;
} Input;

const vec3 DirectionalLightDir = vec3(-0.8, -0.5, -0.3);
// const vec3 DirectionalLightColor = vec3(0.0, 0.333333, 0.6470588);
const vec3 DirectionalLightColor = vec3(0.666666, 0.788235, 0.79215);

const vec3 FogColor = vec3(0.0, 0.175, 0.375);

vec3 Fog(vec3 SourceColor, float Distance, vec3 RayDir, vec3 MoonDir)
{
	const float e = 2.71828182845904523536028747135266249;

	float MoonAmount = max(dot(RayDir, MoonDir), 0.0);
	vec3 FogFinalColor = mix(FogColor, DirectionalLightColor, pow(MoonAmount, 9.0)*pow(e, -pow(Distance*0.01, 2)));
	float FogAmount = 1.0 - pow(e, -pow(Distance*0.0125, 2));
	vec3 Result = mix(SourceColor, FogFinalColor, FogAmount);
	return(Result);
}

void main()
{
	vec3 LightDir = normalize(-DirectionalLightDir);
	vec3 RayDir = normalize(Input.FragPosSim);
	vec3 Normal = normalize(Input.Normal);
	
	vec3 Ambient = 0.3 * DirectionalLightColor * Input.Color * Input.Occlusion;
	vec3 Diffuse = 0.5 * max(dot(LightDir, Normal), 0.0) * DirectionalLightColor * Input.Color;

	vec3 FinalColor = Fog(Ambient + Diffuse, length(Input.FragPosSim), RayDir, LightDir);
	FinalColor = sqrt(FinalColor);
	FragColor = vec4(FinalColor, 1.0);
}