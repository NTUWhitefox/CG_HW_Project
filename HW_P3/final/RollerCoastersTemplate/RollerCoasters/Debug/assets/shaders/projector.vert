#version 330 core

layout(location = 0) in vec3 position;  // Vertex position
layout(location = 1) in vec3 normal;    // Vertex normal

uniform mat4 model;        // Model matrix (object transformation)
uniform mat4 view;         // Camera view matrix
uniform mat4 projection;   // Camera projection matrix
uniform mat4 texMatrix;    // Projection texture matrix

out vec4 texCoords;        // Texture coordinates passed to fragment shader

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    texCoords = texMatrix * model * vec4(position, 1.0);
}
