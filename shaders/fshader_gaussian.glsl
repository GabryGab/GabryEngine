#version 460 core

#define MAX 64

layout (binding=0) uniform sampler2D samp;

uniform bool horizontal;
uniform int radius;
uniform float[MAX] distribution;

in vec2 tc;

out vec4 FragColor;

void main()
{

    vec2 tex_offset = 1.0f / textureSize(samp, 0);
    vec3 temporaryColor = radius != 0 ? texture(samp, tc).rgb * distribution[0] : texture(samp, tc).rgb;

    if(horizontal) {
        for(int i = 1; i < radius; ++i) {
            temporaryColor+= texture(samp, tc + vec2(tex_offset.x * i, 0)).rgb * distribution[i];
            temporaryColor+= texture(samp, tc - vec2(tex_offset.x * i, 0)).rgb * distribution[i];
        }
    } else {
        for(int i = 1; i < radius; ++i) {
            temporaryColor+= texture(samp, tc + vec2(0, tex_offset.y * i)).rgb * distribution[i];
            temporaryColor+= texture(samp, tc - vec2(0, tex_offset.y * i)).rgb * distribution[i];
        }
    }

    FragColor = vec4(temporaryColor, 1.0f);
}