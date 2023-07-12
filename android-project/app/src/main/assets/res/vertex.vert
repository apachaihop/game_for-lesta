#version 320 es
precision mediump float;

layout (location = 0) in mediump vec4 vertex;// <vec2 position, vec2 texCoords>

out mediump vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
    TexCoords = vertex.zw;
    gl_Position = projection*view * model * vec4(vertex.xy, 0.0, 1.0);
}