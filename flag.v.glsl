#version 110

uniform mat4 p_matrix;
uniform sampler2D texture;

attribute vec3 position, normal;
attribute vec2 texcoord;

varying vec3 frag_position, frag_normal;
varying vec2 frag_texcoord;

void main()
{
    gl_Position = p_matrix * vec4(position, 1.0);
    frag_position = position;
    frag_normal = normal;
    frag_texcoord = texcoord;
}
