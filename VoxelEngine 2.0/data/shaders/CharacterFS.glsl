#version 330 core
out vec4 FragColor;

const int CASCADES_COUNT = 3;

in vs_out
{	
	vec3 FragPosSim;
	vec3 Normal;
	vec3 Color;
	vec4 FragPosLightSpace[CASCADES_COUNT];
	float ClipSpacePosZ;
} Input;

uniform bool ShadowsEnabled;
uniform sampler2DArray ShadowMaps;
uniform float CascadesDistances[CASCADES_COUNT + 1];
uniform vec2 SampleOffsets[64];
uniform sampler2D ShadowNoiseTexture;
uniform float Bias;

uniform int Width, Height;

uniform vec3 DirectionalLightDir = vec3(0.0);
const vec3 DirectionalLightColor = vec3(0.666666, 0.788235, 0.79215);

const vec3 FogColor = vec3(0.0, 0.175, 0.375);

vec3 Fog(vec3 SourceColor, float Distance, vec3 RayDir, vec3 MoonDir)
{
	const float e = 2.71828182845904523536028747135266249;

	float MoonAmount = max(dot(RayDir, MoonDir), 0.0);
	vec3 FogFinalColor = mix(FogColor, DirectionalLightColor, pow(MoonAmount, 15.0));
	float FogAmount = 1.0 - pow(e, -pow(Distance*0.0125, 2));
	vec3 Result = mix(SourceColor, FogFinalColor, FogAmount);
	return(Result);
}

float ShadowCalc(float ShadowMapIndex, vec4 FragPosLightSpace, float TexelCount, vec3 Normal, vec3 LightDir)
{
	vec3 ProjectedCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
	ProjectedCoords = ProjectedCoords * 0.5 + 0.5;
	float BiasScaled = 	mix(Bias, 2.0*Bias, (1.0 - max(dot(Normal, LightDir), 0.0)));
	float CurrentFragmentDepth = ProjectedCoords.z - BiasScaled;

	float Result = 0.0;
	vec2 TexelSize = 1.0 / textureSize(ShadowMaps, 0).xy;

	vec2 NoiseScale = vec2(Width, Height) / 8.0;
	vec2 RandomVec = normalize(texture(ShadowNoiseTexture, NoiseScale*(gl_FragCoord.xy/vec2(Width, Height))).rg);
	vec2 Perp = vec2(-RandomVec.y, RandomVec.x);
	mat2 ChangeOffsetMatrix = mat2(RandomVec, Perp);

	for(int SampleOffsetIndex = 0;
		SampleOffsetIndex < 16;
		SampleOffsetIndex++)
	{
		vec2 SampleOffset = ChangeOffsetMatrix * SampleOffsets[SampleOffsetIndex];
		vec2 Offset = TexelCount*TexelSize*SampleOffset;
		float Depth = texture(ShadowMaps, vec3(ProjectedCoords.xy + Offset, ShadowMapIndex)).r;
		Result += CurrentFragmentDepth > Depth ? 1.0 : 0.0;
	}

	Result /= 16.0;

	// float Depth = texture(ShadowMaps, vec3(ProjectedCoords.xy, ShadowMapIndex)).r;
	// Result += CurrentFragmentDepth > Depth ? 1.0 : 0.0;

	return(Result);
}

void main()
{
	vec3 LightDir = normalize(-DirectionalLightDir);
	vec3 RayDir = normalize(Input.FragPosSim);
	vec3 Normal = normalize(Input.Normal);
	
	vec3 Ambient = 0.3 * Input.Color;
	vec3 Diffuse = 0.5 * max(dot(LightDir, Normal), 0.0) * DirectionalLightColor * Input.Color;

	float ShadowFactor = 0.0;
	if(ShadowsEnabled)
	{
		if(Input.ClipSpacePosZ <= CascadesDistances[1])
		{
			if((CascadesDistances[1] - Input.ClipSpacePosZ) <= 1.0)
			{
				float t = (2.0 - ((CascadesDistances[1] - Input.ClipSpacePosZ) + 1.0)) / 2.0;
				float ShadowFactor1 = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);
				float ShadowFactor2 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);
				ShadowFactor = mix(ShadowFactor1, ShadowFactor2, t);
			}
			else
			{
				ShadowFactor = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);
			}
		}
		else if(Input.ClipSpacePosZ <= CascadesDistances[2])
		{
			if((Input.ClipSpacePosZ - CascadesDistances[1]) <= 1.0)
			{
				float t = (2.0 - ((Input.ClipSpacePosZ - CascadesDistances[1]) + 1.0)) / 2.0;
				float ShadowFactor1 = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);
				float ShadowFactor2 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);
				ShadowFactor = mix(ShadowFactor2, ShadowFactor1, t);
			}
			else if((CascadesDistances[2] - Input.ClipSpacePosZ) <= 1.0)
			{
				float t = (2.0 - ((CascadesDistances[2] - Input.ClipSpacePosZ) + 1.0)) / 2.0;
				float ShadowFactor1 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);
				float ShadowFactor2 = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);
				ShadowFactor = mix(ShadowFactor1, ShadowFactor2, t);
			}
			else
			{
				ShadowFactor = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);
			}
		}
		else
		{
			if((Input.ClipSpacePosZ - CascadesDistances[2]) <= 1.0)
			{
				float t = (2.0 - ((Input.ClipSpacePosZ - CascadesDistances[2]) + 1.0)) / 2.0;
				float ShadowFactor1 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);
				float ShadowFactor2 = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);
				ShadowFactor = mix(ShadowFactor2, ShadowFactor1, t);
			}
			else
			{
				ShadowFactor = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);
			}
		}
	}
	vec3 FinalColor = Ambient + (1.0 - ShadowFactor)*Diffuse;
	FinalColor = Fog(FinalColor, length(Input.FragPosSim), RayDir, LightDir);
	FinalColor = sqrt(FinalColor);
	FragColor = vec4(FinalColor, 1.0);
}