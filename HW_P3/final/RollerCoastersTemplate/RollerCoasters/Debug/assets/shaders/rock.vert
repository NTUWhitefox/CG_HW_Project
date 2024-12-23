#version 330 core

// Input from vertex buffer
layout (location = 0) in vec3 aPos;        // Vertex position
layout (location = 1) in vec3 aNormal;     // Vertex normal
layout (location = 2) in vec2 aTexCoords;  // Texture coordinates
layout (location = 3) in vec3 aTangent;    // Tangent vector
layout (location = 4) in vec3 aBitangent;  // Bitangent vector

// Outputs to the fragment shader
out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;
out vec3 Tangent;
out vec3 Bitangent;

// Uniforms
uniform mat4 model;       // Model matrix
uniform mat4 view;        // View matrix
uniform mat4 projection;  // Projection matrix

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));      // World-space position
    Normal = mat3(transpose(inverse(model))) * aNormal;  // Transform normal to world-space
    Tangent = mat3(model) * aTangent;            // Transform tangent to world-space
    Bitangent = mat3(model) * aBitangent;        // Transform bitangent to world-space
    TexCoords = aTexCoords;                      // Pass texture coordinates

    gl_Position = projection * view * vec4(FragPos, 1.0); // Clip-space position
}