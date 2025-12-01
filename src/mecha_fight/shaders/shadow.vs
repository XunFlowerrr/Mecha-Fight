#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aWeights;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;
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

void main()
{
    vec4 skinnedPosition = applySkinning(aPos);
    gl_Position = lightSpaceMatrix * model * skinnedPosition;
}
