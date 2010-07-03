#version 110

uniform mat4 p_matrix, mv_matrix;
uniform sampler2D texture;

attribute vec3 position, normal;
attribute vec2 texcoord;
attribute float specular;

varying vec3 frag_position, frag_normal;
varying vec2 frag_texcoord;
varying float frag_specular;

void main()
{
    vec4 eye_position = mv_matrix * vec4(position, 1.0);
    gl_Position = p_matrix * eye_position;
    frag_position = eye_position.xyz;
    frag_normal   = (mv_matrix * vec4(normal, 0.0)).xyz;
    frag_texcoord = texcoord;
    frag_specular = specular;
}
