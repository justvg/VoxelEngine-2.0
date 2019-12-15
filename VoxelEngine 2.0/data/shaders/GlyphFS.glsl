#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D Texture;

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, texture(Texture, TexCoords).r);
    
    // float Alpha = texture(Texture, TexCoords).r;
    // FragColor = vec4(Alpha, Alpha, Alpha, 1.0);
}