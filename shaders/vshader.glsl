#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 nPos;
layout (location = 2) in vec2 tPos;
layout (location = 3) in vec3 tanPos;

layout (binding = 0) uniform sampler2D texSampler;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMat;
uniform mat3 NormalMat;

out vec2 texCoords;
out vec3 Normal;
out vec3 FragPos;
out vec3 FragPosWorldSpace;
out mat3 TBN;

void main()
{
    FragPosWorldSpace = vec3(model * vec4(aPos, 1.0));
    FragPos = vec3(view * vec4(FragPosWorldSpace, 1.0));
    gl_Position = projection * vec4(FragPos, 1.0); 
    texCoords = tPos;

    Normal = normalize(NormalMat * nPos);
    vec3 T = normalize(NormalMat * tanPos);
    // Re-orthogonalize T with respect to Normal.
    T = normalize(T - dot(T, Normal) * Normal);
    // Then retrieve perpendicular vector B with the cross product of T and Normal.
    vec3 B = cross(Normal, T);

    TBN = mat3(T, B, Normal);
}