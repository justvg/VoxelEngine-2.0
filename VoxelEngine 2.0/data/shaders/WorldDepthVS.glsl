#version 330 core
layout (location = 0) in vec4 aPos;

uniform mat4 Model = mat4(1.0);
uniform mat4 ViewProjection = mat4(1.0);

void main()
{
	gl_Position = ViewProjection * Model * vec4(aPos.xyz, 1.0);
}