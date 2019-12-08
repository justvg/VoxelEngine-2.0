#version 330 core
layout (location = 0) in vec3 aPos;

uniform vec3 CameraUp = vec3(0.0, 1.0, 0.0);
uniform vec3 CameraRight;
uniform vec3 BillboardSimCenterP;;
uniform vec2 Scale;

uniform mat4 ViewProjection = mat4(1.0);

void main()
{
    vec3 Pos = aPos + vec3(0.5, 0.0, 0.0);
    Pos = Pos * vec3(Scale, 1.0);
    Pos = Pos - vec3(0.5, 0.0, 0.0);
    Pos = Pos.x * CameraRight + Pos.y * CameraUp;
    Pos = Pos + BillboardSimCenterP;

    gl_Position = ViewProjection * vec4(Pos, 1.0);
}