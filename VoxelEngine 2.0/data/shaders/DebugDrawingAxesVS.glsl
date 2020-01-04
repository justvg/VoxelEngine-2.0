#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 Model = mat4(1.0);
uniform mat4 ViewProjection = mat4(1.0);

out vec3 Color;

void main()
{
    Color = aColor;
    gl_Position = ViewProjection * Model * vec4(aPos, 1.0);
}