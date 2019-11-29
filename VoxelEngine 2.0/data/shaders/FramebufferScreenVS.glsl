#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 Projection = mat4(1.0);

void main()
{
    TexCoords = aTexCoords;
    gl_Position = Projection * vec4(0.45f*(aPos.x - 0.6) - 0.5, 0.45f*0.5*aPos.y + 0.6, -2.0, 1.0);
}