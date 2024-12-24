// Vertex Shader
#version 450

// Inputs
layout(location = 0) in vec3 inPosition;   // Vertex position
layout(location = 1) in vec3 inNormal;     // Vertex normal
layout(location = 2) in vec2 inUV;         // Vertex UV coordinates
layout(location = 3) in vec3 inTangent;    // Vertex tangent vector

// Outputs to the fragment shader
out vec2 fragUV;         // Pass UV coordinates
out vec3 fragNormal;     // Pass normal vector
out vec3 fragPos;        // Pass world position
out vec3 fragTangent;    // Pass tangent for TBN matrix

// Uniforms
uniform mat4 model;       // Model matrix
uniform mat4 view;        // View matrix
uniform mat4 projection;  // Projection matrix

void main() {
    // Calculate world position of the vertex
    vec4 worldPosition = model * vec4(inPosition, 1.0);
    fragPos = worldPosition.xyz;

    // Transform the normal to world space
    fragNormal = mat3(transpose(inverse(model))) * inNormal;

    // Transform the tangent to world space
    fragTangent = mat3(model) * inTangent;

    // Pass UV coordinates to the fragment shader
    fragUV = inUV;

    // Final position in clip space
    gl_Position = projection * view * worldPosition;
}
