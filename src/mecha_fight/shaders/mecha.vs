#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aWeights;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;
out vec4 FragPosLightSpace;
out vec3 FragPosWorld;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform bool useSkinning;
uniform int bonesCount;
const int MAX_BONES = 100;
uniform mat4 bones[MAX_BONES];

vec4 applySkinning(vec3 position)
{
    if (!useSkinning)
    {
        return vec4(position, 1.0);
    }

    vec4 skinnedPosition = vec4(0.0);
    float totalWeight = 0.0;
    for (int i = 0; i < 4; ++i)
    {
        int boneID = aBoneIDs[i];
        float weight = aWeights[i];
        if (boneID < 0 || boneID >= bonesCount || weight <= 0.0)
        {
            continue;
        }
        skinnedPosition += (bones[boneID] * vec4(position, 1.0)) * weight;
        totalWeight += weight;
    }

    if (totalWeight <= 0.0)
    {
        return vec4(position, 1.0);
    }

    return skinnedPosition;
}

vec3 applySkinningToNormal(vec3 normal)
{
    if (!useSkinning)
    {
        return normal;
    }

    vec3 skinnedNormal = vec3(0.0);
    float totalWeight = 0.0;
    for (int i = 0; i < 4; ++i)
    {
        int boneID = aBoneIDs[i];
        float weight = aWeights[i];
        if (boneID < 0 || boneID >= bonesCount || weight <= 0.0)
        {
            continue;
        }
        mat3 boneMat = mat3(bones[boneID]);
        skinnedNormal += boneMat * normal * weight;
        totalWeight += weight;
    }

    if (totalWeight <= 0.0)
    {
        return normal;
    }

    return skinnedNormal;
}

void main()
{
    TexCoords = aTexCoords;
    vec4 skinnedPosition = applySkinning(aPos);
    vec3 skinnedNormal = applySkinningToNormal(aNormal);

    vec4 worldPos = model * skinnedPosition;
    FragPos = vec3(worldPos);
    FragPosWorld = vec3(worldPos);
    Normal = normalize(mat3(transpose(inverse(model))) * skinnedNormal);
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    gl_Position = projection * view * worldPos;
}
