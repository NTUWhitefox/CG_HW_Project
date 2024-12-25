#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 worldFragPos;

uniform sampler2D diffuseTexture;   // Base texture of the mesh
uniform float dissolveFactor;       // Controls how much of the object dissolves
//uniform vec4 edgeColor;             // Color for edges of the dissolve effect

// Simple hash function to generate pseudo-random noise
float hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p.x) * 43758.5453123 + p.y);
}

// Generate noise based on UV coordinates
float noise(vec2 uv) {
    vec2 i = floor(uv);
    vec2 f = fract(uv);

    // Smooth interpolation
    vec2 u = f * f * (3.0 - 2.0 * f);

    // Blend the noise contributions
    return mix(
        mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x),
        u.y
    );
}

void main()
{
    vec4 edgeColor = vec4(0.75f, 0.4f, 0.35f, 0.85f);
    // Sample the base texture color
    vec4 baseColor = texture(diffuseTexture, TexCoords);

    // Generate procedural noise
    float noiseValue = noise(TexCoords * 10.0); // Scale UV for finer detail

    // Check against the dissolve factor
    if (noiseValue > dissolveFactor) {
        discard; // Fully transparent
    }

    // Add edge effect
    float edgeThreshold = 0.1; // Adjust for edge size
    float edgeBlend = smoothstep(dissolveFactor - edgeThreshold, dissolveFactor, noiseValue);
    vec4 finalColor = mix(baseColor, edgeColor, edgeBlend);

    FragColor = finalColor;
}