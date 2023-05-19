#version 460 core

out vec4 FragColor;

in vec3 TexCoords;

layout (binding = 12) uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
}