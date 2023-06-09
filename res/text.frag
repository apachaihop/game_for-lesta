#version 320 es
precision mediump float;

in mediump vec2 TexCoords;
out mediump vec4 color;

uniform sampler2D image;
uniform mediump vec3 spriteColor;

void main()
{
    color = vec4(spriteColor, 1.0) * texture(image, TexCoords);
}