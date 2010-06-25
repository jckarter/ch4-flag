#include <stdlib.h>
#include <GL/glew.h>
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

struct flag_mesh {
    GLuint vertex_buffer, element_buffer;
    GLsizei element_count;
    GLuint texture;
};

struct flag_vertex {
    float position[4];
    float normal[4];
    float texcoord[4];
};

static struct {
    struct flag_mesh flag, background;
    flag_vertex *flag_vertex_array;
    
    struct {
        GLuint vertex_shader, fragment_shader, program;

        struct {
            GLint texture, mvp_matrix;
        } uniforms;

        struct {
            GLint position, normal, texture;
        } attributes;
    } flag_program;

    GLfloat p_matrix;
} g_resources;

void init_mesh(
    struct flag_mesh *out_mesh,
    struct flag_vertex const *vertex_data, GLsizei vertex_count,
    GLushort const *element_data, GLsizei element_count,
    GLenum hint
) {
    glGenBuffers(1, &out_mesh->vertex_buffer);
    glGenBuffers(1, &out_mesh->element_buffer);

    glBindBuffer(GL_ARRAY_BUFFER, out_mesh->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(struct flag_vertex), vertex_data, hint);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out_mesh->vertex_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, element_count * sizeof(GLushort), element_data, GL_STATIC_DRAW);
}

const GLsizei FLAG_X_RES = 100, FLAG_Y_RES = 75;
const GLfloat FLAG_LO[2] = { 0.0f, -0.375f }, FLAG_HI[2] = { 1.0f, 0.375f };

void init_flag_mesh(struct flag_mesh *out_mesh)
{
    GLsizei vertex_count = FLAG_X_RES * FLAG_Y_RES;
    struct flag_vertex *vertex_data = (flag_vertex*) malloc(vertex_count * sizeof(struct flag_vertex));
    GLsizei element_count = 6 * (FLAG_X_RES - 1) * (FLAG_Y_RES - 1);
    GLushort *element_data = (GLushort*) malloc(element_count * sizeof(GLushort));
    GLsizei y, x, i;
    GLushort index;

    const GLfloat x_step = (FLAG_HI[0] - FLAG_LO[0])/((GLfloat)(FLAG_X_RES - 1)),
                  y_step = (FLAG_HI[1] - FLAG_LO[1])/((GLfloat)(FLAG_Y_RES - 1));
                  s_step = 1.0f/((GLfloat)(FLAG_X_RES - 1)),
                  t_step = 1.0f/((GLfloat)(FLAG_Y_RES - 1));

    for (y = 0, i = 0; y < FLAG_Y_RES; ++y)
        for (x = 0; x < FLAG_X_RES; ++x, ++i) {
            vertex_data[i].position[0] = x_step * x;
            vertex_data[i].position[1] = y_step * y;
            vertex_data[i].position[2] = 0.0f;
            vertex_data[i].position[3] = 1.0f;

            vertex_data[i].normal[0] =  0.0f;
            vertex_data[i].normal[1] =  0.0f;
            vertex_data[i].normal[2] = -1.0f;
            vertex_data[i].normal[3] =  0.0f;

            vertex_data[i].texcoord[0] = s_step;
            vertex_data[i].texcoord[1] = t_step;
            vertex_data[i].texcoord[2] = 0.0f;
            vertex_data[i].texcoord[3] = 0.0f;
        }

    for (y = 0, i = 0, index = 0; y < FLAG_Y_RES - 1; ++y, ++index)
        for (x = 0; x < FLAG_X_RES - 1; ++x, ++index) {
            element_data[i++] = index             ;
            element_data[i++] = index+FLAG_X_RES  ;
            element_data[i++] = index           +1;
            element_data[i++] = index           +1;
            element_data[i++] = index+FLAG_X_RES  ;
            element_data[i++] = index+FLAG_X_RES+1;
        }

    init_mesh(out_mesh, vertex_count, vertex_data, element_count, element_data, GL_STREAM_DRAW);
}

void update_flag_mesh(
    struct flag_mesh const *mesh,
    struct flag_vertex const *vertex_data, GLsizei vertex_count
) {
    
}
