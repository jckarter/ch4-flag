#version 110

uniform mat4 p_matrix;
uniform sampler2D texture;

attribute vec3 position, normal;
attribute vec2 texcoord;
attribute float specular;

varying vec3 frag_position, frag_normal;
varying vec2 frag_texcoord;
varying float frag_specular;

const vec3 eye = vec3(0.5, 0.0, -1.0);

void main()
{
    vec3 eye_position = position - eye;
    gl_Position = p_matrix * vec4(eye_position, 1.0);
    frag_position = eye_position;
    frag_normal   = normal;
    frag_texcoord = texcoord;
    frag_specular = specular;
}
