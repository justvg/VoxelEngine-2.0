#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 SimP;

uniform mat4 ViewProjection = mat4(1.0);

void main()
{
    vec3 SimPos = SimP + aPos;
	gl_Position = ViewProjection * vec4(SimPos, 1.0);
}