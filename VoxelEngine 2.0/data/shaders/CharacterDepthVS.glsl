#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 BoneTransformations[7];
uniform int BoneID;

uniform mat4 Model = mat4(1.0);
uniform mat4 ViewProjection = mat4(1.0);

void main()
{
	vec4 LocalP = BoneTransformations[BoneID] * vec4(aPos, 1.0);
	gl_Position = ViewProjection * Model * LocalP;
}