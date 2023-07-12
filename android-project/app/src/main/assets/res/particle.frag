#version 320 es
precision mediump float;
in vec2 TexCoords;
in vec4 ParticleColor;
out vec4 color;

uniform sampler2D image;

void main()
{
    color = (texture(image, TexCoords) * ParticleColor);
}