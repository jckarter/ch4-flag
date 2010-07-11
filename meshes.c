#include <stdlib.h>
#include <GL/glew.h>
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include "meshes.h"
#include "vec-util.h"

void init_mesh(
    struct flag_mesh *out_mesh,
    struct flag_vertex const *vertex_data, GLsizei vertex_count,
    GLushort const *element_data, GLsizei element_count,
    GLenum hint
) {
    glGenBuffers(1, &out_mesh->vertex_buffer);
    glGenBuffers(1, &out_mesh->element_buffer);
    out_mesh->element_count = element_count;

    glBindBuffer(GL_ARRAY_BUFFER, out_mesh->vertex_buffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertex_count * sizeof(struct flag_vertex),
        vertex_data,
        hint
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out_mesh->element_buffer);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        element_count * sizeof(GLushort),
        element_data,
        GL_STATIC_DRAW
    );
}

static void calculate_flag_vertex(
    struct flag_vertex *v,
    GLfloat s, GLfloat t, GLfloat time
) {
    GLfloat
        sgrad[3] = {
            1.0f + 0.5f*(0.0625f+0.03125f*sinf((GLfloat)M_PI*time))*t*(t - 1.0f),
            0.0f,
            0.125f*(
                sinf(1.5f*(GLfloat)M_PI*(time + s)) 
                + s*cosf(1.5f*(GLfloat)M_PI*(time + s))*(1.5f*(GLfloat)M_PI)
            )
        },
        tgrad[3] = {
            -(0.0625f+0.03125f*sinf((GLfloat)M_PI*time))*(1.0f - s)*(2.0f*t - 1.0f),
            0.75f,
            0.0f
        };

    v->position[0] = s - (0.0625f+0.03125f*sinf((GLfloat)M_PI*time))*(1.0f - 0.5f*s)*t*(t-1.0f);
    v->position[1] = 0.75f*t - 0.375f;
    v->position[2] = 0.125f*(s*sinf(1.5f*(GLfloat)M_PI*(time + s)));
    v->position[3] = 0.0f;

    vec_cross(v->normal, tgrad, sgrad);
    vec_normalize(v->normal);
    v->normal[3] = 0.0f;
}

#define FLAG_X_RES 100
#define FLAG_Y_RES 75
#define FLAG_S_STEP (1.0f/((GLfloat)(FLAG_X_RES - 1)))
#define FLAG_T_STEP (1.0f/((GLfloat)(FLAG_Y_RES - 1)))
#define FLAG_VERTEX_COUNT (FLAG_X_RES * FLAG_Y_RES)

struct flag_vertex *init_flag_mesh(struct flag_mesh *out_mesh)
{
    struct flag_vertex *vertex_data
        = (struct flag_vertex*) malloc(FLAG_VERTEX_COUNT * sizeof(struct flag_vertex));
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
            vertex_data[i].shininess   = 0.0f;
            vertex_data[i].specular[0] = 0;
            vertex_data[i].specular[1] = 0;
            vertex_data[i].specular[2] = 0;
            vertex_data[i].specular[3] = 0;
        }

    for (t = 0, i = 0, index = 0; t < FLAG_Y_RES - 1; ++t, ++index)
        for (s = 0; s < FLAG_X_RES - 1; ++s, ++index) {
            element_data[i++] = index             ;
            element_data[i++] = index           +1;
            element_data[i++] = index+FLAG_X_RES  ;
            element_data[i++] = index           +1;
            element_data[i++] = index+FLAG_X_RES+1;
            element_data[i++] = index+FLAG_X_RES  ;
        }

    init_mesh(
        out_mesh,
        vertex_data, FLAG_VERTEX_COUNT,
        element_data, element_count,
        GL_STREAM_DRAW
    );

    free((void*)element_data);
    return vertex_data;
}

#define FLAGPOLE_TRUCK_TOP            0.5f
#define FLAGPOLE_TRUCK_CROWN          0.41f
#define FLAGPOLE_TRUCK_BOTTOM         0.38f
#define FLAGPOLE_SHAFT_TOP            0.3775f
#define FLAGPOLE_SHAFT_BOTTOM        -1.0f
#define FLAGPOLE_TRUCK_TOP_RADIUS     0.005f
#define FLAGPOLE_TRUCK_CROWN_RADIUS   0.020f
#define FLAGPOLE_TRUCK_BOTTOM_RADIUS  0.015f
#define FLAGPOLE_SHAFT_RADIUS         0.010f
#define FLAGPOLE_SHININESS            4.0f

void init_background_mesh(struct flag_mesh *out_mesh)
{
    static const GLsizei FLAGPOLE_RES = 16, FLAGPOLE_SLICE = 6;
    GLfloat FLAGPOLE_AXIS_XZ[2] = { -FLAGPOLE_SHAFT_RADIUS, 0.0f };
    static const GLubyte FLAGPOLE_SPECULAR[4] = { 255, 255, 192, 0 };

    GLfloat
        GROUND_LO[3] = { -0.875f, FLAGPOLE_SHAFT_BOTTOM, -2.45f },
        GROUND_HI[3] = {  1.875f, FLAGPOLE_SHAFT_BOTTOM,  0.20f },
        WALL_LO[3] = { GROUND_LO[0], FLAGPOLE_SHAFT_BOTTOM, GROUND_HI[2] },
        WALL_HI[3] = { GROUND_HI[0], FLAGPOLE_SHAFT_BOTTOM + 3.0f, GROUND_HI[2] };

    static GLfloat
        TEX_FLAGPOLE_LO[2] = { 0.0f,    0.0f },
        TEX_FLAGPOLE_HI[2] = { 0.03125f,  1.0f },
        TEX_GROUND_LO[2]   = { 0.03125f,  0.0078125f },
        TEX_GROUND_HI[2]   = { 0.515625f, 0.9921875f },
        TEX_WALL_LO[2]     = { 0.515625f, 0.0078125f },
        TEX_WALL_HI[2]     = { 1.0f,      0.9921875f };

#define _FLAGPOLE_T(y) \
    (TEX_FLAGPOLE_LO[1] \
        + (TEX_FLAGPOLE_HI[1] - TEX_FLAGPOLE_LO[1]) \
        * ((y) - FLAGPOLE_TRUCK_TOP)/(FLAGPOLE_SHAFT_BOTTOM - FLAGPOLE_TRUCK_TOP) \
    )

    GLfloat
        theta_step = 2.0f * (GLfloat)M_PI / (GLfloat)FLAGPOLE_RES,
        s_step = (TEX_FLAGPOLE_HI[0] - TEX_FLAGPOLE_LO[0]) / (GLfloat)FLAGPOLE_RES,
        t_truck_top    = TEX_FLAGPOLE_LO[1],
        t_truck_crown  = _FLAGPOLE_T(FLAGPOLE_TRUCK_CROWN),
        t_truck_bottom = _FLAGPOLE_T(FLAGPOLE_TRUCK_BOTTOM),
        t_shaft_top    = _FLAGPOLE_T(FLAGPOLE_SHAFT_TOP),
        t_shaft_bottom = _FLAGPOLE_T(FLAGPOLE_SHAFT_BOTTOM);

#undef _FLAGPOLE_T

    GLsizei
        flagpole_vertex_count = 2 + FLAGPOLE_RES * FLAGPOLE_SLICE,
        wall_vertex_count = 4,
        ground_vertex_count = 4,
        vertex_count = flagpole_vertex_count
            + wall_vertex_count
            + ground_vertex_count;

    GLsizei vertex_i = 0, element_i, i;

    GLsizei
        flagpole_element_count = 3 * ((FLAGPOLE_SLICE - 1) * 2 * FLAGPOLE_RES),
        wall_element_count = 6,
        ground_element_count = 6,
        element_count = flagpole_element_count
            + wall_element_count
            + ground_element_count;

    struct flag_vertex *vertex_data
        = (struct flag_vertex*) malloc(vertex_count * sizeof(struct flag_vertex));

    GLushort *element_data
        = (GLushort*) malloc(element_count * sizeof(GLushort));

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
    vertex_data[0].shininess   = 0.0f;
    vertex_data[0].specular[0] = 0;
    vertex_data[0].specular[1] = 0;
    vertex_data[0].specular[2] = 0;
    vertex_data[0].specular[3] = 0;

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
    vertex_data[1].shininess   = 0.0f;
    vertex_data[1].specular[0] = 0;
    vertex_data[1].specular[1] = 0;
    vertex_data[1].specular[2] = 0;
    vertex_data[1].specular[3] = 0;

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
    vertex_data[2].shininess   = 0.0f;
    vertex_data[2].specular[0] = 0;
    vertex_data[2].specular[1] = 0;
    vertex_data[2].specular[2] = 0;
    vertex_data[2].specular[3] = 0;

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
    vertex_data[3].shininess   = 0.0f;
    vertex_data[3].specular[0] = 0;
    vertex_data[3].specular[1] = 0;
    vertex_data[3].specular[2] = 0;
    vertex_data[3].specular[3] = 0;

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
    vertex_data[4].shininess   = 0.0f;
    vertex_data[4].specular[0] = 0;
    vertex_data[4].specular[1] = 0;
    vertex_data[4].specular[2] = 0;
    vertex_data[4].specular[3] = 0;

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
    vertex_data[5].shininess   = 0.0f;
    vertex_data[5].specular[0] = 0;
    vertex_data[5].specular[1] = 0;
    vertex_data[5].specular[2] = 0;
    vertex_data[5].specular[3] = 0;

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
    vertex_data[6].shininess   = 0.0f;
    vertex_data[6].specular[0] = 0;
    vertex_data[6].specular[1] = 0;
    vertex_data[6].specular[2] = 0;
    vertex_data[6].specular[3] = 0;

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
    vertex_data[7].shininess   = 0.0f;
    vertex_data[7].specular[0] = 0;
    vertex_data[7].specular[1] = 0;
    vertex_data[7].specular[2] = 0;
    vertex_data[7].specular[3] = 0;

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
    vertex_data[8].shininess   = FLAGPOLE_SHININESS;
    vertex_data[8].specular[0] = 0;
    vertex_data[8].specular[1] = 0;
    vertex_data[8].specular[2] = 0;
    vertex_data[8].specular[3] = 0;

    for (i = 0, vertex_i = 9; i < FLAGPOLE_RES; ++i) {
        float sn = sinf(theta_step * (float)i), cs = cosf(theta_step * (float)i);
        float s = TEX_FLAGPOLE_LO[0] + s_step * (float)i;

        vertex_data[vertex_i].position[0]
            = FLAGPOLE_AXIS_XZ[0] + FLAGPOLE_TRUCK_TOP_RADIUS*cs;
        vertex_data[vertex_i].position[1] = FLAGPOLE_TRUCK_TOP;
        vertex_data[vertex_i].position[2]
            = FLAGPOLE_AXIS_XZ[1] + FLAGPOLE_TRUCK_TOP_RADIUS*sn;
        vertex_data[vertex_i].position[3] = 1.0f;
        vertex_data[vertex_i].normal[0]   = cs*0.5f;
        vertex_data[vertex_i].normal[1]   = sqrtf(3.0f/4.0f);
        vertex_data[vertex_i].normal[2]   = sn*0.5f;
        vertex_data[vertex_i].normal[3]   = 0.0f;
        vertex_data[vertex_i].texcoord[0] = s;
        vertex_data[vertex_i].texcoord[1] = t_truck_top;
        vertex_data[vertex_i].shininess   = FLAGPOLE_SHININESS;
        vertex_data[vertex_i].specular[0] = FLAGPOLE_SPECULAR[0];
        vertex_data[vertex_i].specular[1] = FLAGPOLE_SPECULAR[1];
        vertex_data[vertex_i].specular[2] = FLAGPOLE_SPECULAR[2];
        vertex_data[vertex_i].specular[3] = FLAGPOLE_SPECULAR[3];
        ++vertex_i;

        vertex_data[vertex_i].position[0]
            = FLAGPOLE_AXIS_XZ[0] + FLAGPOLE_TRUCK_CROWN_RADIUS*cs;
        vertex_data[vertex_i].position[1] = FLAGPOLE_TRUCK_CROWN;
        vertex_data[vertex_i].position[2]
            = FLAGPOLE_AXIS_XZ[1] + FLAGPOLE_TRUCK_CROWN_RADIUS*sn;
        vertex_data[vertex_i].position[3] = 1.0f;
        vertex_data[vertex_i].normal[0]   = cs;
        vertex_data[vertex_i].normal[1]   = 0.0f;
        vertex_data[vertex_i].normal[2]   = sn;
        vertex_data[vertex_i].normal[3]   = 0.0f;
        vertex_data[vertex_i].texcoord[0] = s;
        vertex_data[vertex_i].texcoord[1] = t_truck_crown;
        vertex_data[vertex_i].shininess   = FLAGPOLE_SHININESS;
        vertex_data[vertex_i].specular[0] = FLAGPOLE_SPECULAR[0];
        vertex_data[vertex_i].specular[1] = FLAGPOLE_SPECULAR[1];
        vertex_data[vertex_i].specular[2] = FLAGPOLE_SPECULAR[2];
        vertex_data[vertex_i].specular[3] = FLAGPOLE_SPECULAR[3];
        ++vertex_i;

        vertex_data[vertex_i].position[0]
            = FLAGPOLE_AXIS_XZ[0] + FLAGPOLE_TRUCK_BOTTOM_RADIUS*cs;
        vertex_data[vertex_i].position[1] = FLAGPOLE_TRUCK_BOTTOM;
        vertex_data[vertex_i].position[2]
            = FLAGPOLE_AXIS_XZ[1] + FLAGPOLE_TRUCK_BOTTOM_RADIUS*sn;
        vertex_data[vertex_i].position[3] = 1.0f;
        vertex_data[vertex_i].normal[0]   = cs*sqrtf(15.0f/16.0f);
        vertex_data[vertex_i].normal[1]   = -0.25f;
        vertex_data[vertex_i].normal[2]   = sn*sqrtf(15.0f/16.0f);
        vertex_data[vertex_i].normal[3]   = 0.0f;
        vertex_data[vertex_i].texcoord[0] = s;
        vertex_data[vertex_i].texcoord[1] = t_truck_bottom;
        vertex_data[vertex_i].shininess   = FLAGPOLE_SHININESS;
        vertex_data[vertex_i].specular[0] = FLAGPOLE_SPECULAR[0];
        vertex_data[vertex_i].specular[1] = FLAGPOLE_SPECULAR[1];
        vertex_data[vertex_i].specular[2] = FLAGPOLE_SPECULAR[2];
        vertex_data[vertex_i].specular[3] = FLAGPOLE_SPECULAR[3];
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
        vertex_data[vertex_i].shininess   = FLAGPOLE_SHININESS;
        vertex_data[vertex_i].specular[0] = FLAGPOLE_SPECULAR[0];
        vertex_data[vertex_i].specular[1] = FLAGPOLE_SPECULAR[1];
        vertex_data[vertex_i].specular[2] = FLAGPOLE_SPECULAR[2];
        vertex_data[vertex_i].specular[3] = FLAGPOLE_SPECULAR[3];
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
        vertex_data[vertex_i].shininess   = FLAGPOLE_SHININESS;
        vertex_data[vertex_i].specular[0] = FLAGPOLE_SPECULAR[0];
        vertex_data[vertex_i].specular[1] = FLAGPOLE_SPECULAR[1];
        vertex_data[vertex_i].specular[2] = FLAGPOLE_SPECULAR[2];
        vertex_data[vertex_i].specular[3] = FLAGPOLE_SPECULAR[3];
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
        vertex_data[vertex_i].shininess   =  FLAGPOLE_SHININESS;
        vertex_data[vertex_i].specular[0] = FLAGPOLE_SPECULAR[0];
        vertex_data[vertex_i].specular[1] = FLAGPOLE_SPECULAR[1];
        vertex_data[vertex_i].specular[2] = FLAGPOLE_SPECULAR[2];
        vertex_data[vertex_i].specular[3] = FLAGPOLE_SPECULAR[3];
        ++vertex_i;
    }
    vertex_data[vertex_i].position[0] =  0.0f;
    vertex_data[vertex_i].position[1] =  FLAGPOLE_SHAFT_BOTTOM;
    vertex_data[vertex_i].position[2] =  0.0f;
    vertex_data[vertex_i].position[3] =  1.0f;
    vertex_data[vertex_i].normal[0]   =  0.0f;
    vertex_data[vertex_i].normal[1]   = -1.0f;
    vertex_data[vertex_i].normal[2]   =  0.0f;
    vertex_data[vertex_i].normal[3]   =  0.0f;
    vertex_data[vertex_i].texcoord[0] =  0.5f;
    vertex_data[vertex_i].texcoord[1] =  t_shaft_bottom;
    vertex_data[vertex_i].shininess   =  FLAGPOLE_SHININESS;
    vertex_data[vertex_i].specular[0] = FLAGPOLE_SPECULAR[0];
    vertex_data[vertex_i].specular[1] = FLAGPOLE_SPECULAR[1];
    vertex_data[vertex_i].specular[2] = FLAGPOLE_SPECULAR[2];
    vertex_data[vertex_i].specular[3] = FLAGPOLE_SPECULAR[3];

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
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1);

        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 1;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1);
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 1;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1) + 1;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1);

        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 1;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 2;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1) + 1;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 2;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1) + 2;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1) + 1;

        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 2;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 3;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1) + 2;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 3;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1) + 3;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1) + 2;

        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 3;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 4;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1) + 3;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 4;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1) + 4;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1) + 3;

        element_data[element_i++] = 9 + FLAGPOLE_SLICE*i     + 5;
        element_data[element_i++] = vertex_i;
        element_data[element_i++] = 9 + FLAGPOLE_SLICE*(i+1) + 5;
    }

    element_data[element_i++] = 8;
    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1);
    element_data[element_i++] = 9;

    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1);
    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 1;
    element_data[element_i++] = 9;
    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 1;
    element_data[element_i++] = 9 + 1;
    element_data[element_i++] = 9;

    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 1;
    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 2;
    element_data[element_i++] = 9 + 1;
    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 2;
    element_data[element_i++] = 9 + 2;
    element_data[element_i++] = 9 + 1;

    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 2;
    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 3;
    element_data[element_i++] = 9 + 2;
    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 3;
    element_data[element_i++] = 9 + 3;
    element_data[element_i++] = 9 + 2;

    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 3;
    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 4;
    element_data[element_i++] = 9 + 3;
    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 4;
    element_data[element_i++] = 9 + 4;
    element_data[element_i++] = 9 + 3;

    element_data[element_i++] = 9 + FLAGPOLE_SLICE*(FLAGPOLE_RES-1) + 5;
    element_data[element_i++] = vertex_i;
    element_data[element_i++] = 9 + 5;

    init_mesh(
        out_mesh,
        vertex_data, vertex_count,
        element_data, element_count,
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
    GLsizei s, t, i;
    for (t = 0, i = 0; t < FLAG_Y_RES; ++t)
        for (s = 0; s < FLAG_X_RES; ++s, ++i) {
            GLfloat ss = FLAG_S_STEP * s, tt = FLAG_T_STEP * t;

            calculate_flag_vertex(&vertex_data[i], ss, tt, time);
        }

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        FLAG_VERTEX_COUNT * sizeof(struct flag_vertex),
        vertex_data,
        GL_STREAM_DRAW
    );
}

