#version 330 core

layout (location = 0) in vec3 aPos;       // Position of the vertex
layout (location = 1) in vec2 aTexCoords; // Texture coordinates
layout (location = 2) in vec3 aInstanceOffset; // Offset for instanced rendering

out vec2 TexCoords; // Pass texture coordinates to fragment shader

uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform float scaleFactor;

void main() {
    // Calculate the position of each particle in world space
    vec3 worldPosition = aPos * scaleFactor + aInstanceOffset;

    // Transform the position into clip space
    gl_Position = projMatrix * viewMatrix * vec4(worldPosition, 1.0);
    TexCoords = aTexCoords;
}
