#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec2 ScreenP = vec2(0.0, 0.0);
uniform vec2 Scale = vec2(1.0, 1.0);
uniform mat4 Projection = mat4(1.0);

void main()
{
    vec2 P = ScreenP + Scale*aPos;
    gl_Position = Projection * vec4(P, -1.0, 1.0);
}