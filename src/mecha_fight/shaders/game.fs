#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform vec3 viewPos;

void main()
{    
    // Debug: Show texture coordinates
    //FragColor = vec4(TexCoords.x, TexCoords.y, 0.0, 1.0);
    //return;
    
    // Debug: Show normals
    //FragColor = vec4(normalize(Normal) * 0.5 + 0.5, 1.0);
    //return;

    // Sample texture
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    
    // Check if this is terrain (no texture) or a model (has texture)
    bool isTerrain = (texColor.r < 0.01 && texColor.g < 0.01 && texColor.b < 0.01);
    
    // Check if this is a shadow (flat on ground with Y normal pointing up)
    bool isShadow = isTerrain && abs(Normal.y) > 0.99;
    
    // For shadow, render as dark semi-transparent
    if (isShadow) {
        // Create circular shadow with soft edges
        vec2 center = vec2(0.5, 0.5);
        float dist = distance(TexCoords, center);
        float shadow = 1.0 - smoothstep(0.3, 0.5, dist);
        
        texColor = vec4(0.0, 0.0, 0.0, shadow * 0.4); // Dark with alpha
    }
    // For terrain, use procedural grass and dirt coloring
    else if (isTerrain) {
        // Grass color (green)
        vec3 grassColor = vec3(0.2, 0.6, 0.2);
        // Dirt color (brown)
        vec3 dirtColor = vec3(0.4, 0.3, 0.2);
        
        // Mix based on height and texture coordinates for variation
        float heightFactor = (FragPos.y + 3.0) / 5.0; // Normalize height
        float noiseFactor = fract(sin(dot(TexCoords, vec2(12.9898, 78.233))) * 43758.5453);
        float mixFactor = clamp(heightFactor + noiseFactor * 0.3, 0.0, 1.0);
        
        texColor = vec4(mix(dirtColor, grassColor, mixFactor), 1.0);
    } else {
        // Discard transparent fragments for car model
        if (texColor.a < 0.5) discard;
    }
    
    // Basic lighting
    vec3 lightPos = vec3(10.0, 10.0, 10.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    
    // Ambient
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular (reduced for terrain)
    float specularStrength = isTerrain ? 0.0 : 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combine lighting with texture/procedural color
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}