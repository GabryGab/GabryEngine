#version 460 core

layout (binding=0) uniform sampler2D samp;
layout (binding=1) uniform sampler2D bloomSamp;

uniform float offsetX;
uniform float offsetY;
uniform float gamma, exposure, bloomBias;
uniform bool useTonemapping, useGammacorrection, usePostProcessing, useBlackAndWhite, useBloom;

in vec2 tc;

out vec4 FragColor;

vec3 aces(vec3);
float luma(vec3);

void main()
{
    vec3 hdrColor = texture(samp, tc).rgb;
    

    if(usePostProcessing) {

        // Bloom before tonemapping.
        if(useBloom) {
            vec3 bloomColor = texture(bloomSamp, tc).rgb;
            hdrColor = mix(hdrColor, bloomColor, bloomBias);
        }

        // Tone mapping before gamma correction.
        if(useTonemapping) {
            hdrColor = aces(hdrColor * exposure);
        }

        // Black and white.
        if(useBlackAndWhite) {
            hdrColor = vec3(luma(hdrColor));
        }

        // Gamma correction must be done last.
        if(useGammacorrection) {
            hdrColor = pow(hdrColor, vec3(1.0/gamma));
        }
    }

    FragColor = vec4(hdrColor, 1.0);
}

// Tone mapping algorithm.
// Final 1 is at about 10.
vec3 aces(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Calculate luma of vec3.
// Luma is the overall brightness of the pixel.
// Basically gray scale.
float luma(vec3 color) {
  return dot(color, vec3(0.299, 0.587, 0.114));
}