#include <stdlib.h>
#include <GL/glew.h>
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include <stddef.h>

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
            GLint texture, p_matrix;
        } uniforms;

        struct {
            GLint position, normal, texture;
        } attributes;
    } flag_program;

    GLfloat p_matrix[16];
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

void calculate_flag_vertex(flag_vertex *v, GLfloat s, GLfloat t, GLfloat time)
{
    v->position[0] = s - 0.125f*(1.0f - s)*t*(t-1.0f)*sinf(1.5f*(GLfloat)M_PI*time);
    v->position[1] = 0.75f*t - 0.375f;
    v->position[2] = 0.125f*(s*sinf(1.5f*(GLfloat)M_PI*(time + s)) - t*(t-1.0f));
    v->position[3] = 1.0f;

    GLfloat
        sgrad[3] = {
            1.0f + 0.125f*t*(t-1.0f)*sinf(1.5f*(float)M_PI*time),
            0.0f,
            0.125f*(
                sinf(1.5f*(GLfloat)M_PI*(time + s))
                + s*cosf(1.5f*(GLfloat)M_PI*(time + s))*1.5f*(GLfloat)M_PI
            )
        },
        tgrad[3] = {
            -0.125f*(1.0f - s)*(2.0f*t - 1.0f)*sinf(1.5f*(GLfloat)M_PI*time),
            0.75f,
            0.25f*t - 0.125f
        };

    v->normal[0] = sgrad[1] * tgrad[2] - sgrad[2] * tgrad[1];
    v->normal[1] = sgrad[2] * tgrad[0] - sgrad[0] * tgrad[2];
    v->normal[2] = sgrad[0] * tgrad[1] - sgrad[1] * tgrad[0];
    v->normal[3] = 0.0f;
}

static const GLsizei
    FLAG_X_RES = 100,
    FLAG_Y_RES = 75,
    FLAG_S_STEP = 1.0f/((GLfloat)(FLAG_X_RES - 1)),
    FLAG_T_STEP = 1.0f/((GLfloat)(FLAG_Y_RES - 1)),
    FLAG_VERTEX_COUNT = FLAG_X_RES * FLAG_Y_RES;

struct flag_vertex *init_flag_mesh(struct flag_mesh *out_mesh)
{
    static const GLfloat
        s_step = 
        t_step =

    struct flag_vertex *vertex_data
        = (flag_vertex*) malloc(FLAG_VERTEX_COUNT * sizeof(struct flag_vertex));
    GLsizei element_count = 6 * (FLAG_X_RES - 1) * (FLAG_Y_RES - 1);
    GLushort *element_data
        = (GLushort*) malloc(element_count * sizeof(GLushort));
    GLsizei s, t, i;
    GLushort index;

    for (t = 0, i = 0; t < FLAG_Y_RES; ++t)
        for (s = 0; s < FLAG_X_RES; ++s, ++i) {
            GLfloat ss = FLAG_S_STEP * s, tt = FLAG_T_STEP * t;

            calculate_flag_vertex(&vertex_data[i], ss, tt, 0.0f);

            vertex_data[i].texcoord[0] = ss;
            vertex_data[i].texcoord[1] = tt;
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
        FLAG_VERTEX_COUNT, vertex_data,
        element_count, element_data,
        GL_STREAM_DRAW
    );

    free((void*)element_data);
    return vertex_data;
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

#define _FLAGPOLE_T(y) \
    (TEX_FLAGPOLE_LO[1] \
        + (TEX_FLAGPOLE_HI[1] - TEX_FLAGPOLE_LO[1]) \
        * ((y) - FLAGPOLE_TRUCK_TOP)/(FLAGPOLE_SHAFT_BOTTOM - FLAGPOLE_TRUCK_TOP) \
    )

    static const GLfloat
        theta_step = 2.0f * (float)M_PI / (GLfloat)FLAGPOLE_RES,
        s_step = (TEX_FLAGPOLE_HI[0] - TEX_FLAGPOLE_LO[0]) / (GLfloat)FLAGPOLE_RES,
        t_truck_top    = TEX_FLAGPOLE_LO[1],
        t_truck_crown  = _FLAGPOLE_T(FLAGPOLE_TRUCK_CROWN),
        t_truck_bottom = _FLAGPOLE_T(FLAGPOLE_TRUCK_BOTTOM),
        t_shaft_top    = _FLAGPOLE_T(FLAGPOLE_SHAFT_TOP),
        t_shaft_bottom = _FLAGPOLE_T(FLAGPOLE_SHAFT_BOTTOM);

#undef _FLAGPOLE_T

    GLsizei
        flagpole_vertex_count = 2 + FLAGPOLE_RES * 5,
        wall_vertex_count = 4,
        ground_vertex_count = 4,
        vertex_count = flagpole_vertex_count
            + wall_vertex_count
            + ground_vertex_count;

    struct flag_vertex *vertex_data
        = (flag_vertex*) malloc(vertex_count * sizeof(struct flag_vertex));
    GLsizei vertex_i = 0, elt_i, i;

    GLsizei
        flagpole_element_count = 3 * (8 * FLAGPOLE_RES),
        wall_element_count = 6,
        ground_element_count = 6,
        element_count = flagpole_element_count
            + wall_element_count
            + ground_element_count;

    struct flag_vertex *vertex_data
        = (flag_vertex*) malloc(vertex_count * sizeof(struct flag_vertex));

    struct GLushort *element_data
        = (GLushort*) malloc(vertex_count * sizeof(GLushort));

    vertex_data[0].position[0] = GROUND_LO[0];
    vertex_data[0].position[1] = GROUND_LO[1];
    vertex_data[0].position[2] = GROUND_LO[2];
    vertex_data[0].position[3] = 1.0f;
    vertex_data[0].normal[0]   = 0.0f;
    vertex_data[0].normal[1]   = 1.0f;
    vertex_data[0].normal[2]   = 0.0f;
    vertex_data[0].normal[3]   = 0.0f;
    vertex_data[0].texcoord[0] = TEX_GROUND_LO[0];
    vertex_data[0].texcoord[1] = TEX_GROUND_LO[1];
    vertex_data[0].texcoord[2] = 0.0f;
    vertex_data[0].texcoord[3] = 0.0f;

    vertex_data[1].position[0] = GROUND_HI[0];
    vertex_data[1].position[1] = GROUND_LO[1];
    vertex_data[1].position[2] = GROUND_LO[2];
    vertex_data[1].position[3] = 1.0f;
    vertex_data[1].normal[0]   = 0.0f;
    vertex_data[1].normal[1]   = 1.0f;
    vertex_data[1].normal[2]   = 0.0f;
    vertex_data[1].normal[3]   = 0.0f;
    vertex_data[1].texcoord[0] = TEX_GROUND_HI[0];
    vertex_data[1].texcoord[1] = TEX_GROUND_LO[1];
    vertex_data[1].texcoord[2] = 0.0f;
    vertex_data[1].texcoord[3] = 0.0f;

    vertex_data[2].position[0] = GROUND_HI[0];
    vertex_data[2].position[1] = GROUND_LO[1];
    vertex_data[2].position[2] = GROUND_HI[2];
    vertex_data[2].position[3] = 1.0f;
    vertex_data[2].normal[0]   = 0.0f;
    vertex_data[2].normal[1]   = 1.0f;
    vertex_data[2].normal[2]   = 0.0f;
    vertex_data[2].normal[3]   = 0.0f;
    vertex_data[2].texcoord[0] = TEX_GROUND_HI[0];
    vertex_data[2].texcoord[1] = TEX_GROUND_HI[1];
    vertex_data[2].texcoord[2] = 0.0f;
    vertex_data[2].texcoord[3] = 0.0f;

    vertex_data[3].position[0] = GROUND_LO[0];
    vertex_data[3].position[1] = GROUND_LO[1];
    vertex_data[3].position[2] = GROUND_HI[2];
    vertex_data[3].position[3] = 1.0f;
    vertex_data[3].normal[0]   = 0.0f;
    vertex_data[3].normal[1]   = 1.0f;
    vertex_data[3].normal[2]   = 0.0f;
    vertex_data[3].normal[3]   = 0.0f;
    vertex_data[3].texcoord[0] = TEX_GROUND_LO[0];
    vertex_data[3].texcoord[1] = TEX_GROUND_HI[1];
    vertex_data[3].texcoord[2] = 0.0f;
    vertex_data[3].texcoord[3] = 0.0f;

    vertex_data[4].position[0] = WALL_LO[0];
    vertex_data[4].position[1] = WALL_LO[1];
    vertex_data[4].position[2] = WALL_LO[2];
    vertex_data[4].position[3] = 1.0f;
    vertex_data[4].normal[0]   = 0.0f;
    vertex_data[4].normal[1]   = 0.0f;
    vertex_data[4].normal[2]   = -1.0f;
    vertex_data[4].normal[3]   = 0.0f;
    vertex_data[4].texcoord[0] = TEX_WALL_LO[0];
    vertex_data[4].texcoord[1] = TEX_WALL_LO[1];
    vertex_data[4].texcoord[2] = 0.0f;
    vertex_data[4].texcoord[3] = 0.0f;

    vertex_data[5].position[0] = WALL_HI[0];
    vertex_data[5].position[1] = WALL_LO[1];
    vertex_data[5].position[2] = WALL_LO[2];
    vertex_data[5].position[3] = 1.0f;
    vertex_data[5].normal[0]   = 0.0f;
    vertex_data[5].normal[1]   = 0.0f;
    vertex_data[5].normal[2]   = -1.0f;
    vertex_data[5].normal[3]   = 0.0f;
    vertex_data[5].texcoord[0] = TEX_WALL_HI[0];
    vertex_data[5].texcoord[1] = TEX_WALL_LO[1];
    vertex_data[5].texcoord[2] = 0.0f;
    vertex_data[5].texcoord[3] = 0.0f;

    vertex_data[6].position[0] = WALL_HI[0];
    vertex_data[6].position[1] = WALL_HI[1];
    vertex_data[6].position[2] = WALL_LO[2];
    vertex_data[6].position[3] = 1.0f;
    vertex_data[6].normal[0]   = 0.0f;
    vertex_data[6].normal[1]   = 0.0f;
    vertex_data[6].normal[2]   = -1.0f;
    vertex_data[6].normal[3]   = 0.0f;
    vertex_data[6].texcoord[0] = TEX_WALL_HI[0];
    vertex_data[6].texcoord[1] = TEX_WALL_HI[1];
    vertex_data[6].texcoord[2] = 0.0f;
    vertex_data[6].texcoord[3] = 0.0f;

    vertex_data[7].position[0] = WALL_LO[0];
    vertex_data[7].position[1] = WALL_HI[1];
    vertex_data[7].position[2] = WALL_LO[2];
    vertex_data[7].position[3] = 1.0f;
    vertex_data[7].normal[0]   = 0.0f;
    vertex_data[7].normal[1]   = 0.0f;
    vertex_data[7].normal[2]   = -1.0f;
    vertex_data[7].normal[3]   = 0.0f;
    vertex_data[7].texcoord[0] = TEX_WALL_LO[0];
    vertex_data[7].texcoord[1] = TEX_WALL_HI[1];
    vertex_data[7].texcoord[2] = 0.0f;
    vertex_data[7].texcoord[3] = 0.0f;

    vertex_data[8].position[0] = FLAGPOLE_AXIS_XZ[0];
    vertex_data[8].position[1] = FLAGPOLE_TRUCK_TOP;
    vertex_data[8].position[2] = FLAGPOLE_AXIS_XZ[1];
    vertex_data[8].position[3] = 1.0f;
    vertex_data[8].normal[0]   = 0.0f;
    vertex_data[8].normal[1]   = 1.0f;
    vertex_data[8].normal[2]   = 0.0f;
    vertex_data[8].normal[3]   = 0.0f;
    vertex_data[8].texcoord[0] = TEX_FLAGPOLE_LO[0];
    vertex_data[8].texcoord[1] = t_truck_top;
    vertex_data[8].texcoord[2] = 0.0f;
    vertex_data[8].texcoord[3] = 0.0f;

    for (i = 0, vertex_i = 9; i < FLAGPOLE_RES; ++i) {
        float sn = sinf(theta_step * (float)i), cs = cosf(theta_step * (float)i);
        float s = TEX_FLAGPOLE_LO[0] + s_step * (float)i;

        vertex_data[vertex_i].position[0]
            = FLAGPOLE_AXIS_XZ[0] + FLAGPOLE_TRUCK_CROWN_RADIUS*cs;
        vertex_data[vertex_i].position[1] = FLAGPOLE_TRUCK_CROWN;
        vertex_data[vertex_i].position[2]
            = FLAGPOLE_AXIS_XZ[1] + FLAGPOLE_TRUCK_CROWN_RADIUS*sn;
        vertex_data[vertex_i].position[3] = 1.0f;
        // XXX normal
        vertex_data[vertex_i].normal[0]   = cs;
        vertex_data[vertex_i].normal[1]   = 0.0f;
        vertex_data[vertex_i].normal[2]   = sn;
        vertex_data[vertex_i].normal[3]   = 0.0f;
        vertex_data[vertex_i].texcoord[0] = s;
        vertex_data[vertex_i].texcoord[1] = t_truck_crown;
        vertex_data[vertex_i].texcoord[2] = 0.0f;
        vertex_data[vertex_i].texcoord[3] = 0.0f;
        ++vertex_i;

        vertex_data[vertex_i].position[0]
            = FLAGPOLE_AXIS_XZ[0] + FLAGPOLE_TRUCK_BOTTOM_RADIUS*cs;
        vertex_data[vertex_i].position[1] = FLAGPOLE_TRUCK_BOTTOM;
        vertex_data[vertex_i].position[2]
            = FLAGPOLE_AXIS_XZ[1] + FLAGPOLE_TRUCK_BOTTOM_RADIUS*sn;
        vertex_data[vertex_i].position[3] = 1.0f;
        // XXX normal
        vertex_data[vertex_i].normal[0]   = cs;
        vertex_data[vertex_i].normal[1]   = 0.0f;
        vertex_data[vertex_i].normal[2]   = sn;
        vertex_data[vertex_i].normal[3]   = 0.0f;
        vertex_data[vertex_i].texcoord[0] = s;
        vertex_data[vertex_i].texcoord[1] = t_truck_bottom;
        vertex_data[vertex_i].texcoord[2] = 0.0f;
        vertex_data[vertex_i].texcoord[3] = 0.0f;
        ++vertex_i;

        vertex_data[vertex_i].position[0]
            = FLAGPOLE_AXIS_XZ[0] + FLAGPOLE_SHAFT_RADIUS*cs;
        vertex_data[vertex_i].position[1] = FLAGPOLE_SHAFT_TOP;
        vertex_data[vertex_i].position[2]
            = FLAGPOLE_AXIS_XZ[1] + FLAGPOLE_SHAFT_RADIUS*sn;
        vertex_data[vertex_i].position[3] = 1.0f;
        vertex_data[vertex_i].normal[0]   = cs;
        vertex_data[vertex_i].normal[1]   = 0.0f;
        vertex_data[vertex_i].normal[2]   = sn;
        vertex_data[vertex_i].normal[3]   = 0.0f;
        vertex_data[vertex_i].texcoord[0] = s;
        vertex_data[vertex_i].texcoord[1] = t_shaft_top;
        vertex_data[vertex_i].texcoord[2] = 0.0f;
        vertex_data[vertex_i].texcoord[3] = 0.0f;
        ++vertex_i;

        vertex_data[vertex_i].position[0]
            = FLAGPOLE_AXIS_XZ[0] + FLAGPOLE_SHAFT_RADIUS*cs;
        vertex_data[vertex_i].position[1] = FLAGPOLE_SHAFT_BOTTOM;
        vertex_data[vertex_i].position[2]
            = FLAGPOLE_AXIS_XZ[1] + FLAGPOLE_TRUCK_BOTTOM_RADIUS*sn;
        vertex_data[vertex_i].position[3] = 1.0f;
        vertex_data[vertex_i].normal[0]   = cs;
        vertex_data[vertex_i].normal[1]   = 0.0f;
        vertex_data[vertex_i].normal[2]   = sn;
        vertex_data[vertex_i].normal[3]   = 0.0f;
        vertex_data[vertex_i].texcoord[0] = s;
        vertex_data[vertex_i].texcoord[1] = t_shaft_bottom;
        vertex_data[vertex_i].texcoord[2] = 0.0f;
        vertex_data[vertex_i].texcoord[3] = 0.0f;
        ++vertex_i;

        vertex_data[vertex_i].position[0]
            = FLAGPOLE_AXIS_XZ[0] + FLAGPOLE_SHAFT_RADIUS*cs;
        vertex_data[vertex_i].position[1] = FLAGPOLE_SHAFT_BOTTOM;
        vertex_data[vertex_i].position[2]
            = FLAGPOLE_AXIS_XZ[1] + FLAGPOLE_TRUCK_BOTTOM_RADIUS*sn;
        vertex_data[vertex_i].position[3] =  1.0f;
        vertex_data[vertex_i].normal[0]   =  0.0f;
        vertex_data[vertex_i].normal[1]   = -1.0f;
        vertex_data[vertex_i].normal[2]   =  0.0f;
        vertex_data[vertex_i].normal[3]   =  0.0f;
        vertex_data[vertex_i].texcoord[0] =  s;
        vertex_data[vertex_i].texcoord[1] =  t_shaft_bottom;
        vertex_data[vertex_i].texcoord[2] =  0.0f;
        vertex_data[vertex_i].texcoord[3] =  0.0f;
        ++vertex_i;
    }
    vertex_data[vertex_i].position[0]
        = 0.0f;
    vertex_data[vertex_i].position[1] = FLAGPOLE_SHAFT_BOTTOM;
    vertex_data[vertex_i].position[2]
        = 0.0f;
    vertex_data[vertex_i].position[3] =  1.0f;
    vertex_data[vertex_i].normal[0]   =  0.0f;
    vertex_data[vertex_i].normal[1]   = -1.0f;
    vertex_data[vertex_i].normal[2]   =  0.0f;
    vertex_data[vertex_i].normal[3]   =  0.0f;
    vertex_data[vertex_i].texcoord[0] =  s;
    vertex_data[vertex_i].texcoord[1] =  t_shaft_bottom;
    vertex_data[vertex_i].texcoord[2] =  0.0f;
    vertex_data[vertex_i].texcoord[3] =  0.0f;

    element_i = 0;

    element_data[element_i++] = 0;
    element_data[element_i++] = 1;
    element_data[element_i++] = 2;

    element_data[element_i++] = 0;
    element_data[element_i++] = 2;
    element_data[element_i++] = 3;

    element_data[element_i++] = 4;
    element_data[element_i++] = 5;
    element_data[element_i++] = 6;

    element_data[element_i++] = 4;
    element_data[element_i++] = 6;
    element_data[element_i++] = 7;

    for (i = 0; i < FLAGPOLE_RES - 1; ++i) {
        element_data[element_i++] = 8;
        element_data[element_i++] = 9 + 5*(i+1);
        element_data[element_i++] = 9 + 5*i;

        element_data[element_i++] = 9 + 5*i;
        element_data[element_i++] = 9 + 5*(i+1);
        element_data[element_i++] = 9 + 5*i     + 1;
        element_data[element_i++] = 9 + 5*i     + 1;
        element_data[element_i++] = 9 + 5*(i+1);
        element_data[element_i++] = 9 + 5*(i+1) + 1;

        element_data[element_i++] = 9 + 5*i     + 1;
        element_data[element_i++] = 9 + 5*(i+1) + 1;
        element_data[element_i++] = 9 + 5*i     + 2;
        element_data[element_i++] = 9 + 5*i     + 2;
        element_data[element_i++] = 9 + 5*(i+1) + 1;
        element_data[element_i++] = 9 + 5*(i+1) + 2;

        element_data[element_i++] = 9 + 5*i     + 2;
        element_data[element_i++] = 9 + 5*(i+1) + 2;
        element_data[element_i++] = 9 + 5*i     + 3;
        element_data[element_i++] = 9 + 5*i     + 3;
        element_data[element_i++] = 9 + 5*(i+1) + 2;
        element_data[element_i++] = 9 + 5*(i+1) + 3;

        element_data[element_i++] = 9 + 5*i     + 4;
        element_data[element_i++] = 9 + 5*(i+1) + 4;
        element_data[element_i++] = vertex_i;
    }

    element_data[element_i++] = 8;
    element_data[element_i++] = 9 + 0;
    element_data[element_i++] = 9 + 5*(FLAGPOLE_RES-1);

    element_data[element_i++] = 9 + 5*(FLAGPOLE_RES-1);
    element_data[element_i++] = 9 + 0;
    element_data[element_i++] = 9 + 5*(FLAGPOLE_RES-1) + 1;
    element_data[element_i++] = 9 + 5*(FLAGPOLE_RES-1) + 1;
    element_data[element_i++] = 9 + 0;
    element_data[element_i++] = 9 + 1;

    element_data[element_i++] = 9 + 5*(FLAGPOLE_RES-1) + 1;
    element_data[element_i++] = 9 + 1;
    element_data[element_i++] = 9 + 5*(FLAGPOLE_RES-1) + 2;
    element_data[element_i++] = 9 + 5*(FLAGPOLE_RES-1) + 2;
    element_data[element_i++] = 9 + 1;
    element_data[element_i++] = 9 + 2;

    element_data[element_i++] = 9 + 5*(FLAGPOLE_RES-1) + 2;
    element_data[element_i++] = 9 + 2;
    element_data[element_i++] = 9 + 5*(FLAGPOLE_RES-1) + 3;
    element_data[element_i++] = 9 + 5*(FLAGPOLE_RES-1) + 3;
    element_data[element_i++] = 9 + 2;
    element_data[element_i++] = 9 + 3;

    element_data[element_i++] = 9 + 5*(FLAGPOLE_RES-1) + 4;
    element_data[element_i++] = 9 + 4;
    element_data[element_i++] = vertex_i;
    
    init_mesh(
        out_mesh,
        vertex_count, vertex_data,
        element_count, element_data,
        GL_STATIC_DRAW
    );

    free(element_data);
    free(vertex_data);
}

void update_flag_mesh(
    struct flag_mesh const *mesh,
    struct flag_vertex *vertex_data,
    GLfloat time
) {
    for (t = 0, i = 0; t < FLAG_Y_RES; ++t)
        for (s = 0; s < FLAG_X_RES; ++s, ++i) {
            GLfloat ss = FLAG_S_STEP * s, tt = FLAG_T_STEP * t;

            calculate_flag_vertex(&vertex_data[i], ss, tt, 0.0f);
        }

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        FLAG_VERTEX_COUNT * sizeof(struct flag_vertex),
        vertex_data
    );
}

void render_mesh(struct flag_mesh const *mesh)
{
    glBindTexture(GL_TEXTURE_2D, mesh->texture);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->element_buffer);
    glDrawElements(GL_TRIANGLES, mesh->element_count, GL_UNSIGNED_SHORT, (void*)0);
}

static GLuint make_texture(const char *filename)
{
    int width, height;
    void *pixels = read_tga(filename, &width, &height);
    GLuint texture;

    if (!pixels)
        return 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D, 0,           /* target, level */
        GL_RGB8,                    /* internal format */
        width, height, 0,           /* width, height, border */
        GL_BGR, GL_UNSIGNED_BYTE,   /* external format, type */
        pixels                      /* pixels */
    );
    free(pixels);
    return texture;
}

static void show_info_log(
    GLuint object,
    PFNGLGETSHADERIVPROC glGet__iv,
    PFNGLGETSHADERINFOLOGPROC glGet__InfoLog
)
{
    GLint log_length;
    char *log;

    glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
    log = malloc(log_length);
    glGet__InfoLog(object, log_length, NULL, log);
    fprintf(stderr, "%s", log);
    free(log);
}

static GLuint make_shader(GLenum type, const char *filename)
{
    GLint length;
    GLchar *source = file_contents(filename, &length);
    GLuint shader;
    GLint shader_ok;

    if (!source)
        return 0;

    shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar**)&source, &length);
    free(source);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
    if (!shader_ok) {
        fprintf(stderr, "Failed to compile %s:\n", filename);
        show_info_log(shader, glGetShaderiv, glGetShaderInfoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint make_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLint program_ok;

    GLuint program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
    if (!program_ok) {
        fprintf(stderr, "Failed to link shader program:\n");
        show_info_log(program, glGetProgramiv, glGetProgramInfoLog);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

static void init_gl_state(void)
{
    glEnable(GL_DEPTH_TEST);
}

static int make_resources(void)
{
    g_resources.flag_vertex_array = init_flag_mesh(&g_resources.flag);
    init_background_mesh(&g_resources.background);

    g_resources.flag.texture = make_texture("flag.tga");
    g_resources.background.texture = make_texture("background.tga");

    g_resources.flag_program.vertex_shader
        = make_shader(GL_VERTEX_SHADER, "flag.v.glsl");
    g_resources.flag_program.fragment_shader
        = make_shader(GL_FRAGMENT_SHADER, "flag.f.glsl");

    g_resources.flag_program.program = make_program(
        g_resources.flag_program.vertex_shader,
        g_resources.flag_program.fragment_shader
    );

    // XXX error checking
    init_p_matrix(&g_resources.p_matrix);

    g_resources.flag_program.uniforms.texture
        = glGetUniformLocation(program, "texture");
    g_resources.flag_program.uniforms.p_matrix
        = glGetUniformLocation(program, "p_matrix");

    g_resources.flag_program.attributes.position
        = glGetAttribLocation(program, "position");
    g_resources.flag_program.attributes.normal
        = glGetAttribLocation(program, "normal");
    g_resources.flag_program.attributes.texture
        = glGetAttribLocation(program, "texture");
}

static void update(void)
{
    int milliseconds = glutGet(GLUT_ELAPSED_TIME);
    GLfloat seconds = (GLfloat)milliseconds * (1.0f/1000.0f);

    update_flag_mesh(&g_resources.flag, g_resources.flag_vertex_array, seconds);
}

static void render(void)
{
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(g_resources.flag_program.uniforms.texture, 0);

    glUniformMatrix4fv(
        g_resources.flag_program.uniforms.p_matrix,
        1, GL_FALSE,
        g_resources.p_matrix
    );

    glEnableVertexAttribArray(g_resources.flag_program.attributes.position);
    glEnableVertexAttribArray(g_resources.flag_program.attributes.normal);
    glEnableVertexAttribArray(g_resources.flag_program.attributes.texture);

    glVertexAttribPointer(
        g_resources.flag_program.attributes.position,
        3, GL_FLOAT, GL_FALSE, sizeof(struct flag_vertex),
        (void*)offsetof(struct flag_vertex, position)
    );
    glVertexAttribPointer(
        g_resources.flag_program.attributes.normal,
        3, GL_FLOAT, GL_FALSE, sizeof(struct flag_vertex),
        (void*)offsetof(struct flag_vertex, normal)
    );
    glVertexAttribPointer(
        g_resources.flag_program.attributes.texcoord,
        2, GL_FLOAT, GL_FALSE, sizeof(struct flag_vertex),
        (void*)offsetof(struct flag_vertex, texcoord)
    );

    render_mesh(&g_resources.flag);
    render_mesh(&g_resources.background);

    glDisableVertexAttribArray(g_resources.flag_program.attributes.position);
    glDisableVertexAttribArray(g_resources.flag_program.attributes.normal);
    glDisableVertexAttribArray(g_resources.flag_program.attributes.texture);
    glutSwapBuffers();
}

int main()
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(640, 480);
    glutCreateWindow("Flag");
    glutIdleFunc(&update);
    glutDisplayFunc(&render);

    glewInit();
    if (!GLEW_VERSION_2_0) {
        fprintf(stderr, "OpenGL 2.0 not available\n");
        return 1;
    }

    init_gl_state();
    if (!make_resources()) {
        fprintf(stderr, "Failed to load resources\n");
        return 1;
    }

    glutMainLoop();
    return 0;
}
