#version 330 core

// Inputs from vertex shader
in vec3 FragPos;
in vec2 TexCoords;
in vec3 Normal;
in vec3 Tangent;
in vec3 Bitangent;

// Outputs
out vec4 FragColor;

// Uniforms
uniform sampler2D diffuseMap;    // Diffuse map
uniform sampler2D specularMap;   // Specular map
uniform sampler2D normalMap;     // Normal map
uniform vec3 lightPos;           // Light position in world space
uniform vec3 viewPos;            // Camera position in world space
uniform vec3 lightColor;         // Light color
uniform float lightIntensity;

const float Ka = 0.2;
const float Kd = 0.7;
const float Ks = 0.5;
const float shininess = 160.0;

// Function to calculate diffuse and specular contributions
vec2 blinnPhongDir(vec3 lightDir, float lightInt, float Ka, float Kd, float Ks, float shininess, vec3 normal) {
    vec3 s = normalize(lightDir);
    vec3 v = normalize(viewPos - FragPos);
    vec3 n = normalize(normal);
    vec3 h = normalize(v + s);
        
    float diffuse = Ka + Kd * lightInt * max(0.0, dot(n, s));
    float spec = Ks * pow(max(0.0, dot(n, h)), shininess);
    return vec2(diffuse, spec);
}

void main()
{
    // Retrieve normal from normal map
    vec3 normal = texture(normalMap, TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0); // Transform from [0,1] to [-1,1]

    // Calculate tangent space light direction
    vec3 T = normalize(Tangent);
    vec3 B = normalize(Bitangent);
    vec3 N = normalize(Normal);
    mat3 TBN = mat3(T, B, N);
    vec3 lightDir = TBN * (lightPos - FragPos);

    // Compute lighting
    vec2 lighting = blinnPhongDir(lightDir, lightIntensity, Ka, Kd, Ks, shininess, normal);

    // Retrieve texture colors
    vec3 diffuseColor = texture(diffuseMap, TexCoords).rgb;
    vec3 specularColor = texture(specularMap, TexCoords).rgb;

    // Final color calculation with lightColor
    vec3 color = lightColor * (lighting.x * diffuseColor + lighting.y * specularColor);

    FragColor = vec4(color, 1.0);
}