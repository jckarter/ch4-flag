#version 110

uniform mat4 p_matrix;
uniform sampler2D texture;

varying vec3 frag_position, frag_normal;
varying vec2 frag_texcoord;

const vec3 light_direction = vec3(0.0, -1.0, -0.1);
const vec4 light_diffuse = vec4(0.8, 0.8, 0.8, 0.8);
const vec4 light_ambient = vec4(0.2, 0.2, 0.2, 0.2);
// XXX specular

void main()
{
    vec4 color = texture2D(texture, frag_texcoord);
    float diffuse = max(dot(normalize(frag_normal), -normalize(light_direction)), 0.0);
    
    gl_FragColor = light_diffuse * diffuse * color + light_ambient;
}
