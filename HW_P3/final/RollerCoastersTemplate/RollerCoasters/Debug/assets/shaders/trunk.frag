#version 450

// Inputs
uniform sampler2D albedoMap;   // Base color texture
uniform sampler2D normalMap;   // Normal map
uniform sampler2D heightMap;   // Height map
uniform sampler2D roughnessMap; // Roughness (optional)
uniform sampler2D aoMap;       // Ambient Occlusion (optional)

// Lighting inputs
uniform vec3 lightDir;         // Directional light
uniform vec3 lightColor;       // Light color
uniform vec3 cameraPos;        // Camera position

// Outputs
layout(location = 0) out vec4 fragColor;

// Varying from vertex shader
in vec2 fragUV;
in vec3 fragNormal;
in vec3 fragPos;

void main() {
    // Sample textures
    vec3 albedo = texture(albedoMap, fragUV).rgb;
    vec3 normal = texture(normalMap, fragUV).rgb * 2.0 - 1.0;
    float height = texture(heightMap, fragUV).r;
    float roughness = texture(roughnessMap, fragUV).r;  // Optional
    float ao = texture(aoMap, fragUV).r;                // Optional

    // Adjust normal with bump mapping
    vec3 T = dFdx(fragPos);  // Tangent vector
    vec3 B = dFdy(fragPos);  // Bitangent vector
    vec3 N = normalize(fragNormal);  // Original normal
    mat3 TBN = mat3(normalize(T), normalize(B), N);
    vec3 bumpedNormal = normalize(TBN * normal);

    // Parallax Mapping (optional, approximate)
    vec3 viewDir = normalize(cameraPos - fragPos);
    vec2 parallaxUV = fragUV + viewDir.xy * height * 0.05;

    // Lighting (Cook-Torrance BRDF)
    vec3 L = normalize(lightDir);
    vec3 V = normalize(cameraPos - fragPos);
    vec3 H = normalize(L + V);  // Halfway vector

    float NdotL = max(dot(bumpedNormal, L), 0.0);
    float NdotV = max(dot(bumpedNormal, V), 0.0);
    float NdotH = max(dot(bumpedNormal, H), 0.0);
    float LdotH = max(dot(L, H), 0.0);

    // Fresnel (Schlick's approximation)
    vec3 F0 = vec3(0.04); // Default reflectance for non-metals
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - LdotH, 5.0);

    // Geometry (Smith's method)
    float k = roughness * roughness / 2.0;
    float G = (NdotL * (1.0 - k) + k) * (NdotV * (1.0 - k) + k);

    // Distribution (GGX)
    float alpha = roughness * roughness;
    float D = alpha / (pi * pow(NdotH * (alpha - 1.0) + 1.0, 2.0));

    // Final lighting
    vec3 specular = (D * F * G) / (4.0 * NdotL * NdotV + 0.001);
    vec3 diffuse = albedo * NdotL;

    // Ambient occlusion
    vec3 ambient = albedo * ao;

    // Combine lighting
    vec3 color = ambient + diffuse + specular;
    fragColor = vec4(color * lightColor, 1.0);
}
