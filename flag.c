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
#include "file-util.h"
#include "gl-util.h"
#include "vec-util.h"
#include "meshes.h"

static struct {
    struct flag_mesh flag, background;
    struct flag_vertex *flag_vertex_array;
    
    struct {
        GLuint vertex_shader, fragment_shader, program;

        struct {
            GLint texture, p_matrix, mv_matrix;
        } uniforms;

        struct {
            GLint position, normal, texcoord, shininess, specular;
        } attributes;
    } flag_program;

    GLfloat p_matrix[16], mv_matrix[16];
    GLfloat eye_offset[2];
    GLsizei window_size[2];
} g_resources;

static void init_gl_state(void)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

#define PROJECTION_FOV_RATIO 0.7f
#define PROJECTION_NEAR_PLANE 0.0625f
#define PROJECTION_FAR_PLANE 256.0f

static void update_p_matrix(GLfloat *matrix, int w, int h)
{
    GLfloat wf = (GLfloat)w, hf = (GLfloat)h;
    GLfloat
        r_xy_factor = fminf(wf, hf) * 1.0f/PROJECTION_FOV_RATIO,
        r_x = r_xy_factor/wf,
        r_y = r_xy_factor/hf,
        r_zw_factor = 1.0f/(PROJECTION_FAR_PLANE - PROJECTION_NEAR_PLANE),
        r_z = (PROJECTION_NEAR_PLANE + PROJECTION_FAR_PLANE)*r_zw_factor,
        r_w = -2.0f*PROJECTION_NEAR_PLANE*PROJECTION_FAR_PLANE*r_zw_factor;

    matrix[ 0] = r_x;  matrix[ 1] = 0.0f; matrix[ 2] = 0.0f; matrix[ 3] = 0.0f;
    matrix[ 4] = 0.0f; matrix[ 5] = r_y;  matrix[ 6] = 0.0f; matrix[ 7] = 0.0f;
    matrix[ 8] = 0.0f; matrix[ 9] = 0.0f; matrix[10] = r_z;  matrix[11] = 1.0f;
    matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = r_w;  matrix[15] = 0.0f;
}

static void update_mv_matrix(GLfloat *matrix, GLfloat *eye_offset)
{
    static const GLfloat BASE_EYE_POSITION[3]  = { 0.5f, -0.25f, -1.25f  };

    matrix[ 0] = 1.0f; matrix[ 1] = 0.0f; matrix[ 2] = 0.0f; matrix[ 3] = 0.0f;
    matrix[ 4] = 0.0f; matrix[ 5] = 1.0f; matrix[ 6] = 0.0f; matrix[ 7] = 0.0f;
    matrix[ 8] = 0.0f; matrix[ 9] = 0.0f; matrix[10] = 1.0f; matrix[11] = 0.0f;
    matrix[12] = -BASE_EYE_POSITION[0] - eye_offset[0];
    matrix[13] = -BASE_EYE_POSITION[1] - eye_offset[1];
    matrix[14] = -BASE_EYE_POSITION[2];
    matrix[15] = 1.0f;
}

static void render_mesh(struct flag_mesh const *mesh)
{
    glBindTexture(GL_TEXTURE_2D, mesh->texture);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
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
    glVertexAttribPointer(
        g_resources.flag_program.attributes.shininess,
        1, GL_FLOAT, GL_FALSE, sizeof(struct flag_vertex),
        (void*)offsetof(struct flag_vertex, shininess)
    );
    glVertexAttribPointer(
        g_resources.flag_program.attributes.specular,
        4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct flag_vertex),
        (void*)offsetof(struct flag_vertex, specular)
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->element_buffer);
    glDrawElements(
        GL_TRIANGLES,
        mesh->element_count,
        GL_UNSIGNED_SHORT,
        (void*)0
    );
}

#define INITIAL_WINDOW_WIDTH  640
#define INITIAL_WINDOW_HEIGHT 480

static void enact_flag_program(
    GLuint vertex_shader,
    GLuint fragment_shader,
    GLuint program
) {
    g_resources.flag_program.vertex_shader = vertex_shader;
    g_resources.flag_program.fragment_shader = fragment_shader;

    g_resources.flag_program.program = program;

    g_resources.flag_program.uniforms.texture
        = glGetUniformLocation(program, "texture");
    g_resources.flag_program.uniforms.p_matrix
        = glGetUniformLocation(program, "p_matrix");
    g_resources.flag_program.uniforms.mv_matrix
        = glGetUniformLocation(program, "mv_matrix");

    g_resources.flag_program.attributes.position
        = glGetAttribLocation(program, "position");
    g_resources.flag_program.attributes.normal
        = glGetAttribLocation(program, "normal");
    g_resources.flag_program.attributes.texcoord
        = glGetAttribLocation(program, "texcoord");
    g_resources.flag_program.attributes.shininess
        = glGetAttribLocation(program, "shininess");
    g_resources.flag_program.attributes.specular
        = glGetAttribLocation(program, "specular");
}

static int make_flag_program(
    GLuint *vertex_shader,
    GLuint *fragment_shader,
    GLuint *program
) {
    *vertex_shader = make_shader(GL_VERTEX_SHADER, "flag.v.glsl");
    if (*vertex_shader == 0)
        return 0;
    *fragment_shader = make_shader(GL_FRAGMENT_SHADER, "flag.f.glsl");
    if (*fragment_shader == 0)
        return 0;

    *program = make_program(*vertex_shader, *fragment_shader);
    if (*program == 0)
        return 0;

    return 1;
}

static void delete_flag_program(void)
{
    glDetachShader(
        g_resources.flag_program.program,
        g_resources.flag_program.vertex_shader
    );
    glDetachShader(
        g_resources.flag_program.program,
        g_resources.flag_program.fragment_shader
    );
    glDeleteProgram(g_resources.flag_program.program);
    glDeleteShader(g_resources.flag_program.vertex_shader);
    glDeleteShader(g_resources.flag_program.fragment_shader);
}

static void update_flag_program(void)
{
    printf("reloading program\n");
    GLuint vertex_shader, fragment_shader, program;

    if (make_flag_program(&vertex_shader, &fragment_shader, &program)) {
        delete_flag_program();
        enact_flag_program(vertex_shader, fragment_shader, program);
    }
}

static int make_resources(void)
{
    GLuint vertex_shader, fragment_shader, program;

    g_resources.flag_vertex_array = init_flag_mesh(&g_resources.flag);
    init_background_mesh(&g_resources.background);

    g_resources.flag.texture = make_texture("flag.tga");
    g_resources.background.texture = make_texture("background.tga");

    if (g_resources.flag.texture == 0 || g_resources.background.texture == 0)
        return 0;

    if (!make_flag_program(&vertex_shader, &fragment_shader, &program))
        return 0;

    enact_flag_program(vertex_shader, fragment_shader, program);

    g_resources.eye_offset[0] = 0.0f;
    g_resources.eye_offset[1] = 0.0f;
    g_resources.window_size[0] = INITIAL_WINDOW_WIDTH;
    g_resources.window_size[1] = INITIAL_WINDOW_HEIGHT;

    update_p_matrix(
        g_resources.p_matrix,
        INITIAL_WINDOW_WIDTH,
        INITIAL_WINDOW_HEIGHT
    );
    update_mv_matrix(g_resources.mv_matrix, g_resources.eye_offset);

    return 1;
}

static void update(void)
{
    int milliseconds = glutGet(GLUT_ELAPSED_TIME);
    GLfloat seconds = (GLfloat)milliseconds * (1.0f/1000.0f);

    update_flag_mesh(&g_resources.flag, g_resources.flag_vertex_array, seconds);
    glutPostRedisplay();
}

static void drag(int x, int y)
{
    float w = (float)g_resources.window_size[0];
    float h = (float)g_resources.window_size[1];
    g_resources.eye_offset[0] = (float)x/w - 0.5f;
    g_resources.eye_offset[1] = -(float)y/h + 0.5f;
    update_mv_matrix(g_resources.mv_matrix, g_resources.eye_offset);
}

static void mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        g_resources.eye_offset[0] = 0.0f;
        g_resources.eye_offset[1] = 0.0f;
        update_mv_matrix(g_resources.mv_matrix, g_resources.eye_offset);
    }
}

static void keyboard(unsigned char key, int x, int y)
{
    if (key == 'r' || key == 'R') {
        update_flag_program();
    }
}

static void reshape(int w, int h)
{
    g_resources.window_size[0] = w;
    g_resources.window_size[1] = h;
    update_p_matrix(g_resources.p_matrix, w, h);
    glViewport(0, 0, w, h);
}

static void render(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(g_resources.flag_program.program);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(g_resources.flag_program.uniforms.texture, 0);

    glUniformMatrix4fv(
        g_resources.flag_program.uniforms.p_matrix,
        1, GL_FALSE,
        g_resources.p_matrix
    );

    glUniformMatrix4fv(
        g_resources.flag_program.uniforms.mv_matrix,
        1, GL_FALSE,
        g_resources.mv_matrix
    );

    glEnableVertexAttribArray(g_resources.flag_program.attributes.position);
    glEnableVertexAttribArray(g_resources.flag_program.attributes.normal);
    glEnableVertexAttribArray(g_resources.flag_program.attributes.texcoord);
    glEnableVertexAttribArray(g_resources.flag_program.attributes.shininess);
    glEnableVertexAttribArray(g_resources.flag_program.attributes.specular);

    render_mesh(&g_resources.flag);
    render_mesh(&g_resources.background);

    glDisableVertexAttribArray(g_resources.flag_program.attributes.position);
    glDisableVertexAttribArray(g_resources.flag_program.attributes.normal);
    glDisableVertexAttribArray(g_resources.flag_program.attributes.texcoord);
    glDisableVertexAttribArray(g_resources.flag_program.attributes.shininess);
    glDisableVertexAttribArray(g_resources.flag_program.attributes.specular);
    glutSwapBuffers();
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT);
    glutCreateWindow("Flag");
    glutIdleFunc(&update);
    glutDisplayFunc(&render);
    glutReshapeFunc(&reshape);
    glutMotionFunc(&drag);
    glutMouseFunc(&mouse);
    glutKeyboardFunc(&keyboard);

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
