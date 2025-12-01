#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;
in vec3 FragPosWorld;

uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform bool useAlbedoTexture;
uniform vec3 fallbackColor;
uniform vec3 lightIntensity;
uniform mat4 lightSpaceMatrix;
uniform bool useSSAO;
uniform sampler2D ssaoMap;
uniform float aoStrength;
uniform vec2 screenSize;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z <= 0.0 || projCoords.z >= 1.0 ||
        projCoords.x <= 0.0 || projCoords.x >= 1.0 ||
        projCoords.y <= 0.0 || projCoords.y >= 1.0)
    {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float ndotl = clamp(dot(normal, lightDir), 0.0, 1.0);
    float slope = sqrt(max(1.0 - ndotl * ndotl, 0.0));
    const float biasMin = 0.0004;
    const float biasMax = 0.015;
    const float biasSlopeFactor = 0.008;
    float bias = clamp(biasMin + slope * biasSlopeFactor, biasMin, biasMax);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main()
{
    vec3 baseColor = fallbackColor;
    if (useAlbedoTexture)
    {
        vec4 texColor = texture(texture_diffuse1, TexCoords);
        if (texColor.a < 0.05)
        {
            discard;
        }
        baseColor = texColor.rgb;
    }

    // Lighting
    vec3 lightColor = lightIntensity;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    // Ambient
    float ambientStrength = 0.45;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Apply normal offset to reduce self-shadowing
    float normalOffsetScale = 0.015;
    vec3 offsetPos = FragPosWorld + norm * normalOffsetScale;
    vec4 offsetLightSpace = lightSpaceMatrix * vec4(offsetPos, 1.0);

    // Calculate shadow
    float shadow = ShadowCalculation(offsetLightSpace, norm, lightDir);

    float aoFactor = 1.0;
    if (useSSAO && screenSize.x > 0.0 && screenSize.y > 0.0)
    {
        vec2 screenUV = gl_FragCoord.xy / screenSize;
        float aoSample = texture(ssaoMap, screenUV).r;
        aoFactor = mix(1.0, aoSample, aoStrength);
    }

    // Combine with shadow
    vec3 result = (ambient + (1.0 - shadow) * diffuse) * baseColor * aoFactor;
    FragColor = vec4(result, 1.0);
}
