#version 330 core

in vec2 TexCoords; // Texture coordinates from vertex shader

out vec4 FragColor; // Final fragment color

uniform sampler2D rainTexture;

void main() {
    // Sample the texture at the given coordinates
    vec4 texColor = texture(rainTexture, TexCoords);

    // Discard fragments with very low alpha for better blending
    if (texColor.a < 0.1)
        discard;

    FragColor = texColor;
}
