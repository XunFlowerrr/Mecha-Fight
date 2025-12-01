#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;
out vec4 FragPosLightSpace;
out vec3 FragPosWorld;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    TexCoords = aTexCoords;
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    FragPosWorld = vec3(worldPos);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
