#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec2 noiseScale;
uniform float radius;
uniform float bias;
uniform float power;
uniform mat4 projection;

const int kernelSize = 64;
uniform vec3 samples[kernelSize];

void main()
{
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).xyz);
    if (length(normal) < 0.0001)
    {
        FragColor = 1.0;
        return;
    }

    float fragDepth = -fragPos.z;
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; ++i)
    {
        vec3 sampleVec = TBN * samples[i];
        vec3 samplePos = fragPos + sampleVec * radius;

        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0)
        {
            continue;
        }

        vec3 fetchedPos = texture(gPosition, offset.xy).xyz;
        float sampleDepth = -fetchedPos.z;
        if (sampleDepth <= 0.0)
        {
            continue;
        }

        float samplePosDepth = -samplePos.z;
        float rangeCheck = smoothstep(0.0, 1.0, radius / (abs(samplePosDepth - sampleDepth) + 1e-4));
        if (sampleDepth <= samplePosDepth - bias)
        {
            occlusion += rangeCheck;
        }
    }

    occlusion = 1.0 - (occlusion / kernelSize);
    FragColor = pow(clamp(occlusion, 0.0, 1.0), power);
}

