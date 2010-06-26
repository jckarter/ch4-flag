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
    glBufferData(
        GL_ARRAY_BUFFER,
        vertex_count * sizeof(struct flag_vertex),
        vertex_data,
        hint
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out_mesh->vertex_buffer);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        element_count * sizeof(GLushort),
        element_data,
        GL_STATIC_DRAW
    );
}

void init_flag_mesh(struct flag_mesh *out_mesh)
{
    static const GLsizei
        FLAG_X_RES = 100,
        FLAG_Y_RES = 75;
    static const GLfloat
        FLAG_LO_XY[2] = { 0.0f, -0.375f },
        FLAG_HI_XY[2] = { 1.0f,  0.375f };

    static const GLfloat
        x_step = (FLAG_HI_XY[0] - FLAG_LO_XY[0])/((GLfloat)(FLAG_X_RES - 1)),
        y_step = (FLAG_HI_XY[1] - FLAG_LO_XY[1])/((GLfloat)(FLAG_Y_RES - 1));
        s_step = 1.0f/((GLfloat)(FLAG_X_RES - 1)),
        t_step = 1.0f/((GLfloat)(FLAG_Y_RES - 1));

    GLsizei vertex_count = FLAG_X_RES * FLAG_Y_RES;
    struct flag_vertex *vertex_data
        = (flag_vertex*) malloc(vertex_count * sizeof(struct flag_vertex));
    GLsizei element_count = 6 * (FLAG_X_RES - 1) * (FLAG_Y_RES - 1);
    GLushort *element_data
        = (GLushort*) malloc(element_count * sizeof(GLushort));
    GLsizei y, x, i;
    GLushort index;

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

    init_mesh(
        out_mesh,
        vertex_count, vertex_data,
        element_count, element_data,
        GL_STREAM_DRAW
    );
}

void init_background_mesh(struct flag_mesh *out_mesh)
{
    static const GLsizei FLAGPOLE_RES = 16;
    static const GLfloat
        FLAGPOLE_TRUCK_TOP    =  0.5f,
        FLAGPOLE_TRUCK_CROWN  =  0.41f,
        FLAGPOLE_TRUCK_BOTTOM =  0.38f,
        FLAGPOLE_SHAFT_TOP    =  0.3775f,
        FLAGPOLE_SHAFT_BOTTOM = -2.00f,
        FLAGPOLE_TRUCK_CROWN_RADIUS  = 0.020f,
        FLAGPOLE_TRUCK_BOTTOM_RADIUS = 0.015f,
        FLAGPOLE_SHAFT_RADIUS        = 0.010f,
        FLAGPOLE_AXIS_XZ[2] = { -FLAGPOLE_SHAFT_RADIUS, 0.0f };

    static const GLfloat
        GROUND_LO[3] = { -2.0f, FLAGPOLE_SHAFT_BOTTOM, -3.0f },
        GROUND_HI[3] = {  4.0f, FLAGPOLE_SHAFT_BOTTOM,  3.0f },
        WALL_LO[3] = { GROUND_LO[0], FLAGPOLE_SHAFT_BOTTOM, GROUND_HI[2] },
        WALL_HI[3] = { GROUND_HI[0], 1.0f, GROUND_HI[2] },

    static const GLfloat
        TEX_FLAGPOLE_LO[2] = { 0.0f,    0.0f },
        TEX_FLAGPOLE_HI[2] = { 0.125f,  1.0f },
        TEX_GROUND_LO[2]   = { 0.125f,  0.03125f },
        TEX_GROUND_HI[2]   = { 0.5625f, 0.96875f },
        TEX_WALL_LO[2]   = { 0.5625f, 0.03125f },
        TEX_WALL_HI[2]   = { 1.0f,    0.96875f },

    static const GLfloat
        theta_step = 2.0f * (float)M_PI / (GLfloat)FLAGPOLE_RES,
        s_step = 1.0f / (GLfloat)FLAGPOLE_RES;

    GLsizei
        flagpole_vertex_count = 1 + FLAGPOLE_RES * 6,
        wall_vertex_count = 4,
        ground_vertex_count = 4,
        vertex_count = flagpole_vertex_count
            + wall_vertex_count
            + ground_vertex_count;

    struct flag_vertex *vertex_data
        = (flag_vertex*) malloc(vertex_count * sizeof(struct flag_vertex));


}

void upload_flag_mesh(
    struct flag_mesh const *mesh,
    struct flag_vertex const *vertex_data, GLsizei vertex_count
) {
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertex_count * sizeof(struct flag_vertex),
        vertex_data
    );
}
