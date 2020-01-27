#version 330 core
out vec4 FragColor; 

in vec2 TexCoords;
in vec3 FragSimP;

uniform sampler2D Texture;

const vec3 FogColor = vec3(0.0, 0.175, 0.375);
uniform vec3 DirectionalLightDir = vec3(0.0);
const vec3 DirectionalLightColor = vec3(0.666666, 0.788235, 0.79215);

vec3 Fog(vec3 SourceColor, float Distance, vec3 RayDir, vec3 MoonDir)
{
	const float e = 2.71828182845904523536028747135266249;

	float MoonAmount = max(dot(RayDir, MoonDir), 0.0);
	vec3 FogFinalColor = mix(FogColor, DirectionalLightColor, pow(MoonAmount, 15.0));
	float FogAmount = 1.0 - pow(e, -pow(Distance*0.0125, 2));
	vec3 Result = mix(SourceColor, FogFinalColor, FogAmount);
	return(Result);
}

void main()
{
    vec4 SampledColor = texture(Texture, TexCoords);
    vec3 FinalColor = (SampledColor*SampledColor).rgb;

    vec3 RayDir = normalize(FragSimP);
    vec3 LightDir = normalize(-DirectionalLightDir);
    FinalColor = Fog(FinalColor, length(FragSimP), RayDir, LightDir);
	FragColor = vec4(sqrt(FinalColor), SampledColor.a);
}