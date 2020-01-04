#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D Texture;
uniform vec3 Color = vec3(1.0, 1.0, 1.0);

void main()
{
    FragColor = vec4(Color, texture(Texture, TexCoords).r);
}