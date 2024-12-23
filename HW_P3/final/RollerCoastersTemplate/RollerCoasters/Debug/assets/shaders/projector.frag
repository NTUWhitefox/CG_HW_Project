#version 330 core

in vec4 texCoords;              // Interpolated texture coordinates
uniform sampler2D projectorTexture; // Projector's texture

out vec4 fragColor;             // Final color of the fragment

void main() {
    // Perspective divide to get normalized texture coordinates
    vec2 projectedCoords = texCoords.xy / texCoords.w;

    // Discard fragments outside the projector's view frustum
    if (projectedCoords.x < 0.0 || projectedCoords.x > 1.0 || 
        projectedCoords.y < 0.0 || projectedCoords.y > 1.0) {
        discard;
    }

    // Fetch the texture color
    fragColor = texture(projectorTexture, projectedCoords);
}
