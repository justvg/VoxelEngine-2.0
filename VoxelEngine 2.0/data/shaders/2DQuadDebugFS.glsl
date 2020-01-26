#version 330 core
out vec4 FragColor;

uniform vec3 Color = vec3(1.0, 1.0, 1.0);

void main()
{
    FragColor = vec4(Color, 1.0);
}