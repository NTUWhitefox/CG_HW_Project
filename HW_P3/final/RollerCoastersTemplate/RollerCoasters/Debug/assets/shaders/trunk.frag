#version 450

// Inputs
uniform sampler2D albedoMap;   // Base color texture
uniform sampler2D normalMap;   // Normal map
uniform sampler2D heightMap;   // Height map (optional)
uniform sampler2D aoMap;       // Ambient Occlusion (optional)

// Lighting inputs
uniform vec3 lightPos;         // Light position in world space
uniform vec3 lightColor;       // Light color
uniform vec3 cameraPos;        // Camera position

// Outputs
layout(location = 0) out vec4 fragColor;

// Varying from vertex shader
in vec2 fragUV;
in vec3 fragNormal;
in vec3 fragPos;

// Configuration for optional maps
const bool useAOMap = false;

void main() {
    const float pi = 3.14159266;

    // Sample textures
    vec3 albedo = texture(albedoMap, fragUV).rgb;
    vec3 normal = texture(normalMap, fragUV).rgb * 2.0 - 1.0;
    float height = texture(heightMap, fragUV).r;

    // Fallback for optional AO map
    float ao = useAOMap ? texture(aoMap, fragUV).r : 1.0; // Default AO to full light

    // Adjust normal with bump mapping
    vec3 T = dFdx(fragPos);  // Tangent vector
    vec3 B = dFdy(fragPos);  // Bitangent vector
    vec3 N = normalize(fragNormal);  // Original normal
    mat3 TBN = mat3(normalize(T), normalize(B), N);
    vec3 bumpedNormal = normalize(TBN * normal);

    // Lighting
    vec3 L = normalize(lightPos - fragPos); // Compute light direction dynamically
    float NdotL = max(dot(bumpedNormal, L), 0.0);

    // Ambient occlusion
    vec3 ambient = albedo * ao;

    // Diffuse lighting
    vec3 diffuse = albedo * NdotL;

    // Combine lighting
    vec3 color = ambient + diffuse;
    fragColor = vec4(color * lightColor, 1.0);
}