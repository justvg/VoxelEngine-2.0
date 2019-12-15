#version 330 core
layout (location = 0) in vec2 aPos;
 
out vec2 TexCoords;

uniform vec2 ScreenP = vec2(0.0, 0.0);
uniform vec3 WidthHeightScale = vec3(1.0, 1.0, 1.0);
uniform float AlignPercentageY = 0.0;
uniform mat4 Projection = mat4(1.0);

void main()
{
    TexCoords = aPos;

    float Scale = WidthHeightScale.z;

    vec2 LeftBotCornerP = ScreenP - vec2(0.0, AlignPercentageY*Scale*WidthHeightScale.y);
    vec2 P = LeftBotCornerP + Scale*WidthHeightScale.xy*aPos;
    gl_Position = Projection * vec4(P, -1.0, 1.0);
}