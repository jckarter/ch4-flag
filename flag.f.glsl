#version 110

uniform mat4 p_matrix, mv_matrix;
uniform sampler2D texture;

varying vec3 frag_position, frag_normal;
varying vec2 frag_texcoord;
varying float frag_shininess;
varying vec4 frag_specular;

const vec3 light_direction = vec3(0.408248, -0.816497, 0.408248);
const vec4 light_diffuse = vec4(0.8, 0.8, 0.8, 0.0);
const vec4 light_ambient = vec4(0.2, 0.2, 0.2, 1.0);
const vec4 light_specular = vec4(1.0, 1.0, 1.0, 1.0);

void main()
{
    vec3 light_eye_direction = (mv_matrix * vec4(light_direction, 0.0)).xyz,
         normal = normalize(frag_normal),
         eye = normalize(frag_position),
         reflection = reflect(light_eye_direction, normal);

    vec4 color = texture2D(texture, frag_texcoord);
    float diffuse = max(-dot(normal, light_eye_direction), 0.0);
    float spec = max(pow(-dot(reflection, eye), frag_shininess), 0.0);
    
    gl_FragColor = spec * frag_specular * light_specular + color * (light_diffuse * diffuse + light_ambient);
}
