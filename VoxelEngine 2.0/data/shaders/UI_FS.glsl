#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float Alpha = 1.0; 
uniform bool TexturedUI;
uniform sampler2D Texture;
uniform vec3 Color = vec3(1.0, 0.0, 1.0);

void main()
{
    if(TexturedUI)
    {
        vec3 SampledColor = texture(Texture, TexCoords).rgb;
        FragColor = vec4(SampledColor, Alpha);
    }
    else
    {
        FragColor = vec4(Color, Alpha);
    }
}