global_variable char CharacterVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"layout (location = 2) in vec3 aColor;\n"
"\n"
"const int CASCADES_COUNT = 3;\n"
"\n"
"uniform mat4 BoneTransformations[7];\n"
"uniform int BoneID;\n"
"\n"
"uniform mat4 Model = mat4(1.0);\n"
"layout (std140) uniform Matrices\n"
"{\n"
"	mat4 ViewProjection;\n"
"	mat4 LightSpaceMatrices[3];\n"
"\n"
"	// NOTE(georgy): This is for the situation when we use debug camera. Even if we use debug camera,\n"
"	//  			 we want Output.ClipSpacePosZ to be as it is from our default camera, not debug one\n"
"	mat4 ViewProjectionForClipSpacePosZ;\n"
"};\n"
"\n"
"out vs_out\n"
"{\n"
"	vec3 FragPosSim;\n"
"	vec3 Normal;\n"
"	vec3 Color;\n"
"	vec4 FragPosLightSpace[CASCADES_COUNT];\n"
"	float ClipSpacePosZ;\n"
"} Output;\n"
"\n"
"void main()\n"
"{\n"
"	vec4 LocalP = BoneTransformations[BoneID] * vec4(aPos, 1.0);\n"
"	vec3 LocalNormal = normalize(mat3(BoneTransformations[BoneID]) * aNormal);\n"
"\n"
"	vec4 SimPos = Model * LocalP;\n"
"	Output.FragPosSim = vec3(SimPos);\n"
"	Output.Normal = normalize(mat3(Model) * LocalNormal);\n"
"	Output.Color = aColor;\n"
"	for(int CascadeIndex = 0;\n"
"		CascadeIndex < CASCADES_COUNT;\n"
"		CascadeIndex++)\n"
"	{\n"
"		Output.FragPosLightSpace[CascadeIndex] = LightSpaceMatrices[CascadeIndex] * SimPos;\n"
"	}\n"
"\n"
"	gl_Position = ViewProjection * SimPos;\n"
"	// Output.ClipSpacePosZ = gl_Position.z;\n"
"	Output.ClipSpacePosZ = (ViewProjectionForClipSpacePosZ * SimPos).z;\n"
"}\n"
;
global_variable char CharacterFS[] = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"const int CASCADES_COUNT = 3;\n"
"const int MAX_POINT_LIGHTS = 64;\n"
"const vec3 FogColor = vec3(0.0, 0.175, 0.375);\n"
"\n"
"struct point_light\n"
"{\n"
"	vec3 P;\n"
"	vec3 Color;\n"
"};\n"
"\n"
"in vs_out\n"
"{	\n"
"	vec3 FragPosSim;\n"
"	vec3 Normal;\n"
"	vec3 Color;\n"
"	vec4 FragPosLightSpace[CASCADES_COUNT];\n"
"	float ClipSpacePosZ;\n"
"} Input;\n"
"\n"
"uniform sampler2DArray ShadowMaps;\n"
"uniform sampler2D ShadowNoiseTexture;\n"
"\n"
"layout (std140) uniform ShadowsInfo\n"
"{\n"
"													// base alignment	// aligned offset\n"
"	bool ShadowsEnabled;							// 4				// 0		\n"
"	float CascadesDistances[CASCADES_COUNT + 1];	// 16				// 16 (4->16) (CascadesDistances[0])\n"
"													// 16				// 32 (4->16) (CascadesDistances[1])\n"
"													// 16				// 48 (4->16) (CascadesDistances[2])\n"
"													// 16				// 64 (4->16) (CascadesDistances[3])\n"
"	vec2 SampleOffsets[16];							// 16				// 80\n"
"	float Bias;										// 4				// 336\n"
"	int Width, Height;								// 4				// 340\n"
"};\n"
"\n"
"layout (std140) uniform DirectionalLightInfo\n"
"{\n"
"	vec3 DirectionalLightDir;\n"
"	vec3 DirectionalLightColor;\n"
"};\n"
"\n"
"vec3 Fog(vec3 SourceColor, float Distance, vec3 RayDir, vec3 MoonDir)\n"
"{\n"
"	const float e = 2.71828182845904523536028747135266249;\n"
"\n"
"	float MoonAmount = max(dot(RayDir, MoonDir), 0.0);\n"
"	vec3 FogFinalColor = mix(FogColor, DirectionalLightColor, pow(MoonAmount, 15.0));\n"
"	float FogAmount = 1.0 - pow(e, -pow(Distance*0.0125, 2));\n"
"	vec3 Result = mix(SourceColor, FogFinalColor, FogAmount);\n"
"	return(Result);\n"
"}\n"
"\n"
"float ShadowCalc(float ShadowMapIndex, vec4 FragPosLightSpace, float TexelCount, vec3 Normal, vec3 LightDir)\n"
"{\n"
"	vec3 ProjectedCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;\n"
"	ProjectedCoords = ProjectedCoords * 0.5 + 0.5;\n"
"	float BiasScaled = 	mix(Bias, 2.0*Bias, (1.0 - max(dot(Normal, LightDir), 0.0)));\n"
"	float CurrentFragmentDepth = ProjectedCoords.z - BiasScaled;\n"
"\n"
"	float Result = 0.0;\n"
"	vec2 TexelSize = 1.0 / textureSize(ShadowMaps, 0).xy;\n"
"\n"
"	vec2 NoiseScale = vec2(Width, Height) / 8.0;\n"
"	vec2 RandomVec = normalize(texture(ShadowNoiseTexture, NoiseScale*(gl_FragCoord.xy/vec2(Width, Height))).rg);\n"
"	vec2 Perp = vec2(-RandomVec.y, RandomVec.x);\n"
"	mat2 ChangeOffsetMatrix = mat2(RandomVec, Perp);\n"
"\n"
"	for(int SampleOffsetIndex = 0;\n"
"		SampleOffsetIndex < 4;\n"
"		SampleOffsetIndex++)\n"
"	{\n"
"		vec2 SampleOffset = ChangeOffsetMatrix * SampleOffsets[SampleOffsetIndex];\n"
"		vec2 Offset = TexelCount*TexelSize*SampleOffset;\n"
"		float Depth = texture(ShadowMaps, vec3(ProjectedCoords.xy + Offset, ShadowMapIndex)).r;\n"
"		Result += CurrentFragmentDepth > Depth ? 1.0 : 0.0;\n"
"	}\n"
"\n"
"	if(Result == 4.0)\n"
"	{\n"
"		Result = 1.0;\n"
"	}\n"
"	else if(Result == 0.0)\n"
"	{\n"
"		Result = 0.0;\n"
"	}\n"
"	else\n"
"	{\n"
"		for(int SampleOffsetIndex = 4;\n"
"			SampleOffsetIndex < 16;\n"
"			SampleOffsetIndex++)\n"
"		{\n"
"			vec2 SampleOffset = ChangeOffsetMatrix * SampleOffsets[SampleOffsetIndex];\n"
"			vec2 Offset = TexelCount*TexelSize*SampleOffset;\n"
"			float Depth = texture(ShadowMaps, vec3(ProjectedCoords.xy + Offset, ShadowMapIndex)).r;\n"
"			Result += CurrentFragmentDepth > Depth ? 1.0 : 0.0;\n"
"		}\n"
"		Result /= 16.0;\n"
"	}\n"
"	\n"
"	return(Result);\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"	vec3 LightDir = normalize(-DirectionalLightDir);\n"
"	vec3 RayDir = normalize(Input.FragPosSim);\n"
"	vec3 Normal = normalize(Input.Normal);\n"
"\n"
"	vec3 Color = Input.Color.rgb;\n"
"	\n"
"	vec3 Ambient = 0.3 * DirectionalLightColor * Color;\n"
"	vec3 DiffuseDir = 0.5 * max(dot(LightDir, Normal), 0.0) * DirectionalLightColor * Color;\n"
"\n"
"	float ShadowFactor = 0.0;\n"
"	if(ShadowsEnabled)\n"
"	{\n"
"		if(Input.ClipSpacePosZ <= CascadesDistances[1])\n"
"		{\n"
"			if((CascadesDistances[1] - Input.ClipSpacePosZ) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((CascadesDistances[1] - Input.ClipSpacePosZ) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor1, ShadowFactor2, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);\n"
"			}\n"
"		}\n"
"		else if(Input.ClipSpacePosZ <= CascadesDistances[2])\n"
"		{\n"
"			if((Input.ClipSpacePosZ - CascadesDistances[1]) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((Input.ClipSpacePosZ - CascadesDistances[1]) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor2, ShadowFactor1, t);\n"
"			}\n"
"			else if((CascadesDistances[2] - Input.ClipSpacePosZ) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((CascadesDistances[2] - Input.ClipSpacePosZ) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor1, ShadowFactor2, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"			}\n"
"		}\n"
"		else\n"
"		{\n"
"			if((Input.ClipSpacePosZ - CascadesDistances[2]) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((Input.ClipSpacePosZ - CascadesDistances[2]) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor2, ShadowFactor1, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"			}\n"
"		}\n"
"	}\n"
"	vec3 FinalColor = Ambient + (1.0 - ShadowFactor)*DiffuseDir;\n"
"	FinalColor = Fog(FinalColor, length(Input.FragPosSim), RayDir, LightDir);\n"
"	FinalColor = sqrt(FinalColor);\n"
"	FragColor = vec4(FinalColor, 1.0);\n"
"}\n"
;
global_variable char WorldVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec4 aPos;\n"
"layout (location = 1) in vec4 aColor;\n"
"layout (location = 2) in vec3 aNormal;\n"
"\n"
"const int CASCADES_COUNT = 3;\n"
"\n"
"uniform mat4 Model = mat4(1.0);\n"
"layout (std140) uniform Matrices\n"
"{\n"
"	mat4 ViewProjection;\n"
"	mat4 LightSpaceMatrices[3];\n"
"			\n"
"	// NOTE(georgy): This is for the situation when we use debug camera. Even if we use debug camera,\n"
"	//  			 we want Output.ClipSpacePosZ to be as it is from our default camera, not debug one\n"
"	mat4 ViewProjectionForClipSpacePosZ;\n"
"};\n"
"\n"
"out vs_out\n"
"{	\n"
"	vec3 FragPosSim;\n"
"	vec3 Normal;\n"
"	vec4 Color;\n"
"	float Occlusion;\n"
"	vec4 FragPosLightSpace[CASCADES_COUNT];\n"
"	float ClipSpacePosZ;\n"
"} Output;\n"
"\n"
"void main()\n"
"{\n"
"	vec4 SimPos = Model * vec4(aPos.xyz, 1.0); \n"
"	Output.FragPosSim = vec3(SimPos);\n"
"	Output.Normal = mat3(Model) * aNormal;\n"
"	Output.Color = aColor;\n"
"	Output.Occlusion = aPos.w;\n"
"	for(int CascadeIndex = 0;\n"
"		CascadeIndex < CASCADES_COUNT;\n"
"		CascadeIndex++)\n"
"	{\n"
"		Output.FragPosLightSpace[CascadeIndex] = LightSpaceMatrices[CascadeIndex] * SimPos;\n"
"	}\n"
"\n"
"	gl_Position = ViewProjection * SimPos;\n"
"	// Output.ClipSpacePosZ  = gl_Position.z;\n"
"	Output.ClipSpacePosZ = (ViewProjectionForClipSpacePosZ * SimPos).z;\n"
"}\n"
;
global_variable char WorldFS[] = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"const int CASCADES_COUNT = 3;\n"
"const vec3 FogColor = vec3(0.0, 0.175, 0.375);\n"
"\n"
"in vs_out\n"
"{	\n"
"	vec3 FragPosSim;\n"
"	vec3 Normal;\n"
"	vec4 Color;\n"
"	float Occlusion;\n"
"	vec4 FragPosLightSpace[CASCADES_COUNT];\n"
"	float ClipSpacePosZ;\n"
"} Input;\n"
"\n"
"uniform sampler2DArray ShadowMaps;\n"
"uniform sampler2D ShadowNoiseTexture;\n"
"\n"
"layout (std140) uniform ShadowsInfo\n"
"{\n"
"													// base alignment	// aligned offset\n"
"	bool ShadowsEnabled;							// 4				// 0		\n"
"	float CascadesDistances[CASCADES_COUNT + 1];	// 16				// 16 (4->16) (CascadesDistances[0])\n"
"													// 16				// 32 (4->16) (CascadesDistances[1])\n"
"													// 16				// 48 (4->16) (CascadesDistances[2])\n"
"													// 16				// 64 (4->16) (CascadesDistances[3])\n"
"	vec2 SampleOffsets[16];							// 16				// 80\n"
"	float Bias;										// 4				// 336\n"
"	int Width, Height;								// 4				// 340\n"
"};\n"
"\n"
"layout (std140) uniform DirectionalLightInfo\n"
"{\n"
"	vec3 DirectionalLightDir;\n"
"	vec3 DirectionalLightColor;\n"
"};\n"
"\n"
"vec3 Fog(vec3 SourceColor, float Distance, vec3 RayDir, vec3 MoonDir)\n"
"{\n"
"	const float e = 2.71828182845904523536028747135266249;\n"
"\n"
"	float MoonAmount = max(dot(RayDir, MoonDir), 0.0);\n"
"	vec3 FogFinalColor = mix(FogColor, DirectionalLightColor, pow(MoonAmount, 15.0));\n"
"	float FogAmount = 1.0 - pow(e, -pow(Distance*0.0125, 2));\n"
"	vec3 Result = mix(SourceColor, FogFinalColor, FogAmount);\n"
"	return(Result);\n"
"}\n"
"\n"
"float ShadowCalc(float ShadowMapIndex, vec4 FragPosLightSpace, float TexelCount, vec3 Normal, vec3 LightDir)\n"
"{\n"
"	vec3 ProjectedCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;\n"
"	ProjectedCoords = ProjectedCoords * 0.5 + 0.5;\n"
"\n"
"	float CosAlpha = max(dot(Normal, LightDir), 0.0);\n"
"	float SinAlpha = sqrt(1.0 - CosAlpha*CosAlpha);\n"
"	float TanAlpha = SinAlpha / CosAlpha;\n"
"	float BiasScaled = Bias*TanAlpha;\n"
"	\n"
"	float CurrentFragmentDepth = ProjectedCoords.z - BiasScaled;\n"
"\n"
"	float Result = 0.0;\n"
"	vec2 TexelSize = 1.0 / textureSize(ShadowMaps, 0).xy;\n"
"\n"
"	vec2 NoiseScale = vec2(Width, Height) / 8.0;\n"
"	vec2 RandomVec = normalize(texture(ShadowNoiseTexture, NoiseScale*(gl_FragCoord.xy/vec2(Width, Height))).rg);\n"
"	vec2 Perp = vec2(-RandomVec.y, RandomVec.x);\n"
"	mat2 ChangeOffsetMatrix = mat2(RandomVec, Perp);\n"
"\n"
"	for(int SampleOffsetIndex = 0;\n"
"		SampleOffsetIndex < 4;\n"
"		SampleOffsetIndex++)\n"
"	{\n"
"		vec2 SampleOffset = ChangeOffsetMatrix * SampleOffsets[SampleOffsetIndex];\n"
"		vec2 Offset = TexelCount*TexelSize*SampleOffset;\n"
"		float Depth = texture(ShadowMaps, vec3(ProjectedCoords.xy + Offset, ShadowMapIndex)).r;\n"
"		Result += CurrentFragmentDepth > Depth ? 1.0 : 0.0;\n"
"	}\n"
"\n"
"	if(Result == 4.0)\n"
"	{\n"
"		Result = 1.0;\n"
"	}\n"
"	else if(Result == 0.0)\n"
"	{\n"
"		Result = 0.0;\n"
"	}\n"
"	else\n"
"	{\n"
"		for(int SampleOffsetIndex = 4;\n"
"			SampleOffsetIndex < 16;\n"
"			SampleOffsetIndex++)\n"
"		{\n"
"			vec2 SampleOffset = ChangeOffsetMatrix * SampleOffsets[SampleOffsetIndex];\n"
"			vec2 Offset = TexelCount*TexelSize*SampleOffset;\n"
"			float Depth = texture(ShadowMaps, vec3(ProjectedCoords.xy + Offset, ShadowMapIndex)).r;\n"
"			Result += CurrentFragmentDepth > Depth ? 1.0 : 0.0;\n"
"		}\n"
"		Result /= 16.0;\n"
"	}\n"
"\n"
"	return(Result);\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"	vec3 LightDir = normalize(-DirectionalLightDir);\n"
"	vec3 RayDir = normalize(Input.FragPosSim);\n"
"	vec3 Normal = normalize(Input.Normal);\n"
"	\n"
"	vec3 Color = Input.Color.rgb;\n"
"\n"
"	vec3 Ambient = 0.3 * DirectionalLightColor * Color * Input.Occlusion;\n"
"	vec3 DiffuseDir = 0.5 * max(dot(LightDir, Normal), 0.0) * DirectionalLightColor * Color;\n"
"\n"
"	float ShadowFactor = 0.0;\n"
"	if(ShadowsEnabled)\n"
"	{\n"
"		if(Input.ClipSpacePosZ <= CascadesDistances[1])\n"
"		{\n"
"			if((CascadesDistances[1] - Input.ClipSpacePosZ) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((CascadesDistances[1] - Input.ClipSpacePosZ) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(0.0, Input.FragPosLightSpace[0], 3.0, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor1, ShadowFactor2, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(0.0, Input.FragPosLightSpace[0], 3.0, Normal, LightDir);\n"
"			}\n"
"		}\n"
"		else if(Input.ClipSpacePosZ <= CascadesDistances[2])\n"
"		{\n"
"			if((Input.ClipSpacePosZ - CascadesDistances[1]) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((Input.ClipSpacePosZ - CascadesDistances[1]) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(0.0, Input.FragPosLightSpace[0], 3.0, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor2, ShadowFactor1, t);\n"
"			}\n"
"			else if((CascadesDistances[2] - Input.ClipSpacePosZ) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((CascadesDistances[2] - Input.ClipSpacePosZ) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor1, ShadowFactor2, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"			}\n"
"		}\n"
"		else\n"
"		{\n"
"			if((Input.ClipSpacePosZ - CascadesDistances[2]) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((Input.ClipSpacePosZ - CascadesDistances[2]) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor2, ShadowFactor1, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"			}\n"
"		}\n"
"	}\n"
"	vec3 FinalColor = Ambient + (1.0 - ShadowFactor)*DiffuseDir;\n"
"	FinalColor = Fog(FinalColor, length(Input.FragPosSim), RayDir, LightDir);\n"
"	FinalColor = sqrt(FinalColor);\n"
"	FragColor = vec4(FinalColor, Input.Color.a);\n"
"	// FragColor = vec4(FinalColor, 1.0);\n"
"}\n"
;
global_variable char WaterVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec4 aPos;\n"
"layout (location = 1) in vec4 aColor;\n"
"\n"
"const int CASCADES_COUNT = 3;\n"
"\n"
"uniform mat4 Model = mat4(1.0);\n"
"layout (std140) uniform Matrices\n"
"{\n"
"	mat4 ViewProjection;\n"
"	mat4 LightSpaceMatrices[3];\n"
"			\n"
"	// NOTE(georgy): This is for the situation when we use debug camera. Even if we use debug camera,\n"
"	//  			 we want Output.ClipSpacePosZ to be as it is from our default camera, not debug one\n"
"	mat4 ViewProjectionForClipSpacePosZ;\n"
"};\n"
"\n"
"uniform mat4 View = mat4(1.0);\n"
"\n"
"out vs_out\n"
"{	\n"
"	vec3 FragPosSim;\n"
"    float FragZView;\n"
"	vec3 Normal;\n"
"	vec4 Color;\n"
"	float Occlusion;\n"
"	vec4 FragPosLightSpace[CASCADES_COUNT];\n"
"	float ClipSpacePosZ;\n"
"} Output;\n"
"\n"
"void main()\n"
"{\n"
"	vec4 SimPos = Model * vec4(aPos.xyz, 1.0); \n"
"	Output.FragPosSim = vec3(SimPos);\n"
"    Output.FragZView = -(View * SimPos).z;\n"
"	Output.Normal = vec3(0.0, 1.0, 0.0);\n"
"	Output.Color = aColor;\n"
"	Output.Occlusion = aPos.w;\n"
"	for(int CascadeIndex = 0;\n"
"		CascadeIndex < CASCADES_COUNT;\n"
"		CascadeIndex++)\n"
"	{\n"
"		Output.FragPosLightSpace[CascadeIndex] = LightSpaceMatrices[CascadeIndex] * SimPos;\n"
"	}\n"
"\n"
"	gl_Position = ViewProjection * SimPos;\n"
"	// Output.ClipSpacePosZ  = gl_Position.z;\n"
"	Output.ClipSpacePosZ = (ViewProjectionForClipSpacePosZ * SimPos).z;\n"
"}\n"
;
global_variable char WaterFS[] = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"const int CASCADES_COUNT = 3;\n"
"const int MAX_POINT_LIGHTS = 64;\n"
"const vec3 FogColor = vec3(0.0, 0.175, 0.375);\n"
"\n"
"struct point_light\n"
"{\n"
"	vec3 P;\n"
"	vec3 Color;\n"
"};\n"
"\n"
"in vs_out\n"
"{	\n"
"	vec3 FragPosSim;\n"
"    float FragZView;\n"
"	vec3 Normal;\n"
"	vec4 Color;\n"
"	float Occlusion;\n"
"	vec4 FragPosLightSpace[CASCADES_COUNT];\n"
"	float ClipSpacePosZ;\n"
"} Input;\n"
"\n"
"uniform sampler2DArray ShadowMaps;\n"
"uniform sampler2D ShadowNoiseTexture;\n"
"\n"
"layout (std140) uniform ShadowsInfo\n"
"{\n"
"													// base alignment	// aligned offset\n"
"	bool ShadowsEnabled;							// 4				// 0		\n"
"	float CascadesDistances[CASCADES_COUNT + 1];	// 16				// 16 (4->16) (CascadesDistances[0])\n"
"													// 16				// 32 (4->16) (CascadesDistances[1])\n"
"													// 16				// 48 (4->16) (CascadesDistances[2])\n"
"													// 16				// 64 (4->16) (CascadesDistances[3])\n"
"	vec2 SampleOffsets[16];							// 16				// 80\n"
"	float Bias;										// 4				// 336\n"
"	int Width, Height;								// 4				// 340\n"
"};\n"
"\n"
"layout (std140) uniform DirectionalLightInfo\n"
"{\n"
"	vec3 DirectionalLightDir;\n"
"	vec3 DirectionalLightColor;\n"
"};\n"
"\n"
"vec3 Fog(vec3 SourceColor, float Distance, vec3 RayDir, vec3 MoonDir)\n"
"{\n"
"	const float e = 2.71828182845904523536028747135266249;\n"
"\n"
"	float MoonAmount = max(dot(RayDir, MoonDir), 0.0);\n"
"	vec3 FogFinalColor = mix(FogColor, DirectionalLightColor, pow(MoonAmount, 15.0));\n"
"	float FogAmount = 1.0 - pow(e, -pow(Distance*0.0125, 2));\n"
"	vec3 Result = mix(SourceColor, FogFinalColor, FogAmount);\n"
"	return(Result);\n"
"}\n"
"\n"
"float ShadowCalc(float ShadowMapIndex, vec4 FragPosLightSpace, float TexelCount, vec3 Normal, vec3 LightDir)\n"
"{\n"
"	vec3 ProjectedCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;\n"
"	ProjectedCoords = ProjectedCoords * 0.5 + 0.5;\n"
"	float BiasScaled = 	mix(Bias, 2.0*Bias, (1.0 - max(dot(Normal, LightDir), 0.0)));\n"
"	float CurrentFragmentDepth = ProjectedCoords.z - BiasScaled;\n"
"\n"
"	float Result = 0.0;\n"
"	vec2 TexelSize = 1.0 / textureSize(ShadowMaps, 0).xy;\n"
"\n"
"	vec2 NoiseScale = vec2(Width, Height) / 8.0;\n"
"	vec2 RandomVec = normalize(texture(ShadowNoiseTexture, NoiseScale*(gl_FragCoord.xy/vec2(Width, Height))).rg);\n"
"	vec2 Perp = vec2(-RandomVec.y, RandomVec.x);\n"
"	mat2 ChangeOffsetMatrix = mat2(RandomVec, Perp);\n"
"\n"
"	for(int SampleOffsetIndex = 0;\n"
"		SampleOffsetIndex < 4;\n"
"		SampleOffsetIndex++)\n"
"	{\n"
"		vec2 SampleOffset = ChangeOffsetMatrix * SampleOffsets[SampleOffsetIndex];\n"
"		vec2 Offset = TexelCount*TexelSize*SampleOffset;\n"
"		float Depth = texture(ShadowMaps, vec3(ProjectedCoords.xy + Offset, ShadowMapIndex)).r;\n"
"		Result += CurrentFragmentDepth > Depth ? 1.0 : 0.0;\n"
"	}\n"
"\n"
"	if(Result == 4.0)\n"
"	{\n"
"		Result = 1.0;\n"
"	}\n"
"	else if(Result == 0.0)\n"
"	{\n"
"		Result = 0.0;\n"
"	}\n"
"	else\n"
"	{\n"
"		for(int SampleOffsetIndex = 4;\n"
"			SampleOffsetIndex < 16;\n"
"			SampleOffsetIndex++)\n"
"		{\n"
"			vec2 SampleOffset = ChangeOffsetMatrix * SampleOffsets[SampleOffsetIndex];\n"
"			vec2 Offset = TexelCount*TexelSize*SampleOffset;\n"
"			float Depth = texture(ShadowMaps, vec3(ProjectedCoords.xy + Offset, ShadowMapIndex)).r;\n"
"			Result += CurrentFragmentDepth > Depth ? 1.0 : 0.0;\n"
"		}\n"
"		Result /= 16.0;\n"
"	}\n"
"\n"
"	return(Result);\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"	vec3 LightDir = normalize(-DirectionalLightDir);\n"
"	vec3 RayDir = normalize(Input.FragPosSim);\n"
"	vec3 Normal = normalize(Input.Normal);\n"
"	\n"
"	vec3 Color = Input.Color.rgb;\n"
"\n"
"	vec3 Ambient = 0.3 * DirectionalLightColor * Color * Input.Occlusion;\n"
"	vec3 DiffuseDir = 0.5 * max(dot(LightDir, Normal), 0.0) * DirectionalLightColor * Color;\n"
"\n"
"\n"
"	float ShadowFactor = 0.0;\n"
"	if(ShadowsEnabled)\n"
"	{\n"
"		if(Input.ClipSpacePosZ <= CascadesDistances[1])\n"
"		{\n"
"			if((CascadesDistances[1] - Input.ClipSpacePosZ) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((CascadesDistances[1] - Input.ClipSpacePosZ) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor1, ShadowFactor2, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);\n"
"			}\n"
"		}\n"
"		else if(Input.ClipSpacePosZ <= CascadesDistances[2])\n"
"		{\n"
"			if((Input.ClipSpacePosZ - CascadesDistances[1]) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((Input.ClipSpacePosZ - CascadesDistances[1]) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor2, ShadowFactor1, t);\n"
"			}\n"
"			else if((CascadesDistances[2] - Input.ClipSpacePosZ) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((CascadesDistances[2] - Input.ClipSpacePosZ) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor1, ShadowFactor2, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"			}\n"
"		}\n"
"		else\n"
"		{\n"
"			if((Input.ClipSpacePosZ - CascadesDistances[2]) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((Input.ClipSpacePosZ - CascadesDistances[2]) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor2, ShadowFactor1, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"			}\n"
"		}\n"
"	}\n"
"	vec3 FinalColor = Ambient + (1.0 - ShadowFactor)*DiffuseDir;\n"
"	FinalColor = Fog(FinalColor, length(Input.FragPosSim), RayDir, LightDir);\n"
"	FinalColor = sqrt(FinalColor);\n"
"	FragColor = vec4(FinalColor, mix(Input.Color.a, 1.0, Input.FragZView*0.03));\n"
"}\n"
;
global_variable char HitpointsVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"\n"
"uniform vec3 CameraUp = vec3(0.0, 1.0, 0.0);\n"
"uniform vec3 CameraRight;\n"
"uniform vec3 BillboardSimCenterP;;\n"
"uniform vec2 Scale;\n"
"\n"
"uniform mat4 ViewProjection = mat4(1.0);\n"
"\n"
"void main()\n"
"{\n"
"    vec3 Pos = aPos + vec3(0.5, 0.0, 0.0);\n"
"    Pos = Pos * vec3(Scale, 1.0);\n"
"    Pos = Pos - vec3(0.5, 0.0, 0.0);\n"
"    Pos = Pos.x * CameraRight + Pos.y * CameraUp;\n"
"    Pos = Pos + BillboardSimCenterP;\n"
"\n"
"    gl_Position = ViewProjection * vec4(Pos, 1.0);\n"
"}\n"
;
global_variable char HitpointsFS[] = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"uniform vec3 Color;\n"
"\n"
"void main()\n"
"{\n"
"    FragColor = vec4(Color, 1.0);\n"
"}\n"
;
global_variable char BlockParticleVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"layout (location = 2) in vec3 SimP;\n"
"layout (location = 3) in vec3 Color;\n"
"layout (location = 4) in float Scale;\n"
"\n"
"const int CASCADES_COUNT = 3;\n"
"\n"
"layout (std140) uniform Matrices\n"
"{\n"
"	mat4 ViewProjection;\n"
"	mat4 LightSpaceMatrices[3];\n"
"\n"
"	// NOTE(georgy): This is for the situation when we use debug camera. Even if we use debug camera,\n"
"	//  			 we want Output.ClipSpacePosZ to be as it is from our default camera, not debug one\n"
"	mat4 ViewProjectionForClipSpacePosZ;\n"
"};\n"
"\n"
"out vs_out\n"
"{	\n"
"	vec3 FragPosSim;\n"
"	vec3 Normal;\n"
"	vec3 Color;\n"
"	vec4 FragPosLightSpace[CASCADES_COUNT];\n"
"	float ClipSpacePosZ;\n"
"} Output;\n"
"\n"
"void main()\n"
"{\n"
"	vec4 SimPos = vec4(Scale*aPos + SimP, 1.0); \n"
"	Output.FragPosSim = vec3(SimPos);\n"
"	Output.Normal = normalize(aNormal);\n"
"	Output.Color = Color;\n"
"	for(int CascadeIndex = 0;\n"
"		CascadeIndex < CASCADES_COUNT;\n"
"		CascadeIndex++)\n"
"	{\n"
"		Output.FragPosLightSpace[CascadeIndex] = LightSpaceMatrices[CascadeIndex] * SimPos;\n"
"	}\n"
"\n"
"	gl_Position = ViewProjection * SimPos;\n"
"	// Output.ClipSpacePosZ = gl_Position.z;\n"
"	Output.ClipSpacePosZ = (ViewProjectionForClipSpacePosZ * SimPos).z;\n"
"}\n"
;
global_variable char BlockParticleFS[] = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"const int CASCADES_COUNT = 3;\n"
"const int MAX_POINT_LIGHTS = 64;\n"
"const vec3 FogColor = vec3(0.0, 0.175, 0.375);\n"
"\n"
"struct point_light\n"
"{\n"
"	vec3 P;\n"
"	vec3 Color;\n"
"};\n"
"\n"
"in vs_out\n"
"{	\n"
"	vec3 FragPosSim;\n"
"	vec3 Normal;\n"
"	vec3 Color;\n"
"	vec4 FragPosLightSpace[CASCADES_COUNT];\n"
"	float ClipSpacePosZ;\n"
"} Input;\n"
"\n"
"uniform sampler2DArray ShadowMaps;\n"
"uniform sampler2D ShadowNoiseTexture;\n"
"\n"
"layout (std140) uniform ShadowsInfo\n"
"{\n"
"													// base alignment	// aligned offset\n"
"	bool ShadowsEnabled;							// 4				// 0		\n"
"	float CascadesDistances[CASCADES_COUNT + 1];	// 16				// 16 (4->16) (CascadesDistances[0])\n"
"													// 16				// 32 (4->16) (CascadesDistances[1])\n"
"													// 16				// 48 (4->16) (CascadesDistances[2])\n"
"													// 16				// 64 (4->16) (CascadesDistances[3])\n"
"	vec2 SampleOffsets[16];							// 16				// 80\n"
"	float Bias;										// 4				// 336\n"
"	int Width, Height;								// 4				// 340\n"
"};\n"
"\n"
"layout (std140) uniform DirectionalLightInfo\n"
"{\n"
"	vec3 DirectionalLightDir;\n"
"	vec3 DirectionalLightColor;\n"
"};\n"
"\n"
"vec3 Fog(vec3 SourceColor, float Distance, vec3 RayDir, vec3 MoonDir)\n"
"{\n"
"	const float e = 2.71828182845904523536028747135266249;\n"
"\n"
"	float MoonAmount = max(dot(RayDir, MoonDir), 0.0);\n"
"	vec3 FogFinalColor = mix(FogColor, DirectionalLightColor, pow(MoonAmount, 15.0));\n"
"	float FogAmount = 1.0 - pow(e, -pow(Distance*0.0125, 2));\n"
"	vec3 Result = mix(SourceColor, FogFinalColor, FogAmount);\n"
"	return(Result);\n"
"}\n"
"\n"
"float ShadowCalc(float ShadowMapIndex, vec4 FragPosLightSpace, float TexelCount, vec3 Normal, vec3 LightDir)\n"
"{\n"
"	vec3 ProjectedCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;\n"
"	ProjectedCoords = ProjectedCoords * 0.5 + 0.5;\n"
"	float BiasScaled = 	mix(Bias, 2.0*Bias, (1.0 - max(dot(Normal, LightDir), 0.0)));\n"
"	float CurrentFragmentDepth = ProjectedCoords.z - BiasScaled;\n"
"\n"
"	float Result = 0.0;\n"
"	vec2 TexelSize = 1.0 / textureSize(ShadowMaps, 0).xy;\n"
"\n"
"	vec2 NoiseScale = vec2(Width, Height) / 8.0;\n"
"	vec2 RandomVec = normalize(texture(ShadowNoiseTexture, NoiseScale*(gl_FragCoord.xy/vec2(Width, Height))).rg);\n"
"	vec2 Perp = vec2(-RandomVec.y, RandomVec.x);\n"
"	mat2 ChangeOffsetMatrix = mat2(RandomVec, Perp);\n"
"\n"
"	for(int SampleOffsetIndex = 0;\n"
"		SampleOffsetIndex < 4;\n"
"		SampleOffsetIndex++)\n"
"	{\n"
"		vec2 SampleOffset = ChangeOffsetMatrix * SampleOffsets[SampleOffsetIndex];\n"
"		vec2 Offset = TexelCount*TexelSize*SampleOffset;\n"
"		float Depth = texture(ShadowMaps, vec3(ProjectedCoords.xy + Offset, ShadowMapIndex)).r;\n"
"		Result += CurrentFragmentDepth > Depth ? 1.0 : 0.0;\n"
"	}\n"
"\n"
"	if(Result == 4.0)\n"
"	{\n"
"		Result = 1.0;\n"
"	}\n"
"	else if(Result == 0.0)\n"
"	{\n"
"		Result = 0.0;\n"
"	}\n"
"	else\n"
"	{\n"
"		for(int SampleOffsetIndex = 4;\n"
"			SampleOffsetIndex < 16;\n"
"			SampleOffsetIndex++)\n"
"		{\n"
"			vec2 SampleOffset = ChangeOffsetMatrix * SampleOffsets[SampleOffsetIndex];\n"
"			vec2 Offset = TexelCount*TexelSize*SampleOffset;\n"
"			float Depth = texture(ShadowMaps, vec3(ProjectedCoords.xy + Offset, ShadowMapIndex)).r;\n"
"			Result += CurrentFragmentDepth > Depth ? 1.0 : 0.0;\n"
"		}\n"
"		Result /= 16.0;\n"
"	}\n"
"	\n"
"	return(Result);\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"	vec3 LightDir = normalize(-DirectionalLightDir);\n"
"    vec3 RayDir = normalize(Input.FragPosSim);\n"
"	vec3 Normal = normalize(Input.Normal);\n"
"\n"
"	vec3 Color = Input.Color.rgb;\n"
"\n"
"    vec3 Ambient = 0.3 * DirectionalLightColor * Color;\n"
"	vec3 DiffuseDir = 0.5 * max(dot(LightDir, Normal), 0.0) * DirectionalLightColor * Color;\n"
"\n"
"    float ShadowFactor = 0.0;\n"
"	if(ShadowsEnabled)\n"
"	{\n"
"		if(Input.ClipSpacePosZ <= CascadesDistances[1])\n"
"		{\n"
"			if((CascadesDistances[1] - Input.ClipSpacePosZ) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((CascadesDistances[1] - Input.ClipSpacePosZ) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor1, ShadowFactor2, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);\n"
"			}\n"
"		}\n"
"		else if(Input.ClipSpacePosZ <= CascadesDistances[2])\n"
"		{\n"
"			if((Input.ClipSpacePosZ - CascadesDistances[1]) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((Input.ClipSpacePosZ - CascadesDistances[1]) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(0.0, Input.FragPosLightSpace[0], 5.0, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor2, ShadowFactor1, t);\n"
"			}\n"
"			else if((CascadesDistances[2] - Input.ClipSpacePosZ) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((CascadesDistances[2] - Input.ClipSpacePosZ) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor1, ShadowFactor2, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"			}\n"
"		}\n"
"		else\n"
"		{\n"
"			if((Input.ClipSpacePosZ - CascadesDistances[2]) <= 1.0)\n"
"			{\n"
"				float t = (2.0 - ((Input.ClipSpacePosZ - CascadesDistances[2]) + 1.0)) / 2.0;\n"
"				float ShadowFactor1 = ShadowCalc(1.0, Input.FragPosLightSpace[1], 2.5, Normal, LightDir);\n"
"				float ShadowFactor2 = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"				ShadowFactor = mix(ShadowFactor2, ShadowFactor1, t);\n"
"			}\n"
"			else\n"
"			{\n"
"				ShadowFactor = ShadowCalc(2.0, Input.FragPosLightSpace[2], 1.5, Normal, LightDir);\n"
"			}\n"
"		}\n"
"	}\n"
"	vec3 FinalColor = Ambient + (1.0 - ShadowFactor)*DiffuseDir;\n"
"	FinalColor = Fog(FinalColor, length(Input.FragPosSim), RayDir, LightDir);\n"
"	FinalColor = sqrt(FinalColor);\n"
"	FragColor = vec4(FinalColor, 1.0);\n"
"}\n"
;
global_variable char CharacterDepthVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"\n"
"uniform mat4 BoneTransformations[7];\n"
"uniform int BoneID;\n"
"\n"
"uniform mat4 Model = mat4(1.0);\n"
"uniform mat4 ViewProjection = mat4(1.0);\n"
"\n"
"void main()\n"
"{\n"
"	vec4 LocalP = BoneTransformations[BoneID] * vec4(aPos, 1.0);\n"
"	gl_Position = ViewProjection * Model * LocalP;\n"
"}\n"
;
global_variable char EmptyFS[] = 
"#version 330 core\n"
"\n"
"void main()\n"
"{\n"
"    \n"
"}\n"
;
global_variable char WorldDepthVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec4 aPos;\n"
"\n"
"// uniform mat4 Model = mat4(1.0);\n"
"// uniform mat4 ViewProjection = mat4(1.0);\n"
"\n"
"uniform mat4 MVP = mat4(1.0);\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = MVP * vec4(aPos.xyz, 1.0);\n"
"}\n"
;
global_variable char BlockParticleDepthVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 2) in vec3 SimP;\n"
"\n"
"uniform mat4 ViewProjection = mat4(1.0);\n"
"\n"
"void main()\n"
"{\n"
"    vec3 SimPos = SimP + aPos;\n"
"	gl_Position = ViewProjection * vec4(SimPos, 1.0);\n"
"}\n"
;
global_variable char UI_VS[] = 
"#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"\n"
"uniform vec2 ScreenP = vec2(0.0, 0.0);\n"
"uniform vec2 Scale = vec2(1.0, 1.0);\n"
"\n"
"layout (std140) uniform Matrices\n"
"{\n"
"    mat4 Projection;\n"
"};\n"
"\n"
"out vec2 TexCoords;\n"
"\n"
"void main()\n"
"{\n"
"    TexCoords = aPos + vec2(0.5, 0.5);\n"
"\n"
"    vec2 P = ScreenP + Scale*aPos;\n"
"    gl_Position = Projection * vec4(P, -1.0, 1.0);\n"
"}\n"
;
global_variable char UI_FS[] = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"in vec2 TexCoords;\n"
"\n"
"uniform float Alpha = 1.0; \n"
"uniform bool TexturedUI;\n"
"uniform sampler2D Texture;\n"
"uniform vec3 Color = vec3(1.0, 0.0, 1.0);\n"
"\n"
"void main()\n"
"{\n"
"    if(TexturedUI)\n"
"    {\n"
"        vec3 SampledColor = texture(Texture, TexCoords).rgb;\n"
"        FragColor = vec4(SampledColor, Alpha);\n"
"    }\n"
"    else\n"
"    {\n"
"        FragColor = vec4(Color, Alpha);\n"
"    }\n"
"}\n"
;
global_variable char GlyphVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
" \n"
"uniform vec2 ScreenP = vec2(0.0, 0.0);\n"
"uniform vec3 WidthHeightScale = vec3(1.0, 1.0, 1.0);\n"
"uniform float AlignPercentageY = 0.0;\n"
"\n"
"layout (std140) uniform Matrices\n"
"{\n"
"    mat4 Projection;\n"
"};\n"
"\n"
"out vec2 TexCoords;\n"
"\n"
"void main()\n"
"{\n"
"    TexCoords = aPos;\n"
"\n"
"    float Scale = WidthHeightScale.z;\n"
"\n"
"    vec2 LeftBotCornerP = ScreenP - vec2(0.0, AlignPercentageY*Scale*WidthHeightScale.y);\n"
"    vec2 P = LeftBotCornerP + Scale*WidthHeightScale.xy*aPos;\n"
"    gl_Position = Projection * vec4(P, -1.0, 1.0);\n"
"}\n"
;
global_variable char GlyphFS[] = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"in vec2 TexCoords;\n"
"\n"
"uniform sampler2D Texture;\n"
"uniform vec3 Color = vec3(1.0, 1.0, 1.0);\n"
"\n"
"void main()\n"
"{\n"
"    FragColor = vec4(Color, texture(Texture, TexCoords).r);\n"
"}\n"
;
global_variable char FramebufferScreenVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in vec2 aTexCoords;\n"
"\n"
"out vec2 TexCoords;\n"
"\n"
"uniform mat4 Projection = mat4(1.0);\n"
"\n"
"void main()\n"
"{\n"
"    TexCoords = aTexCoords;\n"
"    gl_Position = Projection * vec4(0.45f*(aPos.x - 0.6) - 0.5, 0.45f*0.5*aPos.y + 0.6, -2.0, 1.0);\n"
"}\n"
;
global_variable char FramebufferScreenFS[] = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"in vec2 TexCoords;\n"
"\n"
"uniform sampler2D ScreenTexture;\n"
"\n"
"void main()\n"
"{\n"
"    float Color = texture(ScreenTexture, TexCoords).r;\n"
"    FragColor = vec4(vec3(Color), 1.0);\n"
"}\n"
;
global_variable char QuadDebug2DVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"\n"
"uniform vec2 ScreenP = vec2(0.0, 0.0);\n"
"uniform vec2 Scale = vec2(1.0, 1.0);\n"
"\n"
"layout (std140) uniform Matrices\n"
"{\n"
"    mat4 Projection;\n"
"};\n"
"\n"
"void main()\n"
"{\n"
"    vec2 P = ScreenP + Scale*aPos;\n"
"    gl_Position = Projection * vec4(P, -1.0, 1.0);\n"
"}\n"
;
global_variable char QuadDebug2DFS[] = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"uniform vec3 Color = vec3(1.0, 1.0, 1.0);\n"
"\n"
"void main()\n"
"{\n"
"    FragColor = vec4(Color, 1.0);\n"
"}\n"
;
global_variable char DebugDrawingVS[] = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"\n"
"uniform mat4 Model = mat4(1.0);\n"
"uniform mat4 ViewProjection = mat4(1.0);\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = ViewProjection * Model * vec4(aPos, 1.0);\n"
"}\n"
;
global_variable char DebugDrawingFS[] = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"uniform vec3 Color = vec3(1.0, 1.0, 1.0);\n"
"\n"
"void main()\n"
"{\n"
"    FragColor = vec4(Color, 1.0);\n"
"}\n"
;
