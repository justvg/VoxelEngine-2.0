#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 SimP;
layout (location = 2) in vec2 OffsetInAtlas;
layout (location = 3) in float Scale;

out vec2 TexCoords;

uniform vec3 CameraRight;
uniform vec3 CameraUp;
uniform float RowsInAtlas;

uniform mat4 ViewProjection = mat4(1.0);

void main()
{
    TexCoords = aPos.xy + vec2(0.5, 0.5);
    TexCoords = TexCoords / RowsInAtlas;
    TexCoords.x = TexCoords.x + OffsetInAtlas.x;
    TexCoords.y = TexCoords.y + (1.0 - (OffsetInAtlas.y + 1.0/RowsInAtlas));
    // TexCoords.y = TexCoords.y + OffsetInAtlas.y;

    vec3 P = SimP + Scale*CameraRight*aPos.x + Scale*CameraUp*aPos.y;
    gl_Position = ViewProjection * vec4(P, 1.0);
}