#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform vec3 tint;
uniform float intensity;

void main()
{
    vec3 color = texture(skybox, TexCoords).rgb * tint * intensity;
    FragColor = vec4(color, 1.0);
}

