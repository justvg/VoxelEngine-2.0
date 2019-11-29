#version 330 core
out vec4 FragColor;

const int CASCADES_COUNT = 3;

in vs_out
{	
	vec3 FragPosSim;
	vec3 Normal;
	vec3 Color;
	float Occlusion;
	vec4 FragPosLightSpace[CASCADES_COUNT];
	float ClipSpacePosZ;
} Input;

uniform sampler2DArray ShadowMaps;
uniform float CascadesDistances[CASCADES_COUNT + 1];

// const vec3 DirectionalLightDir = -vec3(0.0, 1.0, 0.0);
uniform vec3 DirectionalLightDir = vec3(0.0);
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

float ShadowCalc(float ShadowMapIndex, vec4 FragPosLightSpace, vec3 Normal, vec3 LightDir)
{
	vec3 ProjectedCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
	ProjectedCoords = ProjectedCoords * 0.5 + 0.5;
	float DepthInShadowMap = texture(ShadowMaps, vec3(ProjectedCoords.xy, ShadowMapIndex)).r;
	// float Bias = max(0.002 * (1.0 - max(dot(Normal, LightDir), 0.0)), 0.0015);
	float Bias = 0.0015;
	float CurrentFragmentDepth = ProjectedCoords.z - Bias;

	float Result = 0.0;
	vec2 TexelSize = 1.0 / textureSize(ShadowMaps, 0).xy;
	for(int X = -1; X <= 1; ++X)
	{
		for(int Y = -1; Y <= 1; ++Y)
		{
			float Depth = texture(ShadowMaps, vec3(ProjectedCoords.xy + vec2(X, Y) * TexelSize, ShadowMapIndex)).r; 
			Result += CurrentFragmentDepth > Depth ? 1.0 : 0.0;        
		}    
	}
	
	Result /= 9.0;

	return(Result);
}

void main()
{
	vec3 LightDir = normalize(-DirectionalLightDir);
	vec3 RayDir = normalize(Input.FragPosSim);
	vec3 Normal = normalize(Input.Normal);
	
	vec3 Ambient = 0.3 * DirectionalLightColor * Input.Color * Input.Occlusion;
	vec3 Diffuse = 0.5 * max(dot(LightDir, Normal), 0.0) * DirectionalLightColor * Input.Color;

	float ShadowFactor = 0.0;
	if(Input.ClipSpacePosZ <= CascadesDistances[1])
	{
		ShadowFactor = ShadowCalc(0.0, Input.FragPosLightSpace[0], Normal, LightDir);
	}
	else if(Input.ClipSpacePosZ <= CascadesDistances[2])
	{
		ShadowFactor = ShadowCalc(1.0, Input.FragPosLightSpace[1], Normal, LightDir);
	}
	else
	{
		ShadowFactor = ShadowCalc(2.0, Input.FragPosLightSpace[2], Normal, LightDir);
	}
	vec3 FinalColor = Ambient + (1.0 - ShadowFactor)*Diffuse;
	FinalColor = Fog(FinalColor, length(Input.FragPosSim), RayDir, LightDir);
	FinalColor = sqrt(FinalColor);
	FragColor = vec4(FinalColor, 1.0);
}