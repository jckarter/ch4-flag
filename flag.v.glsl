#version 110

uniform mat4 p_matrix;
uniform sampler2D texture;

attribute vec3 position, normal;
attribute vec2 texcoord;

varying vec3 frag_position, frag_normal;
varying vec2 frag_texcoord;

const vec3 eye = vec3(0.5, 0.0, -1.0);

void main()
{
    gl_Position = p_matrix * vec4(position - eye, 1.0);
    frag_position = position;
    frag_normal = normal;
    frag_texcoord = texcoord;
}
