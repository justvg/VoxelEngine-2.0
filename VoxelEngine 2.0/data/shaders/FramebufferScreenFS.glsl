#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D ScreenTexture;

void main()
{
    float Color = texture(ScreenTexture, TexCoords).r;
    FragColor = vec4(vec3(Color), 1.0);
}