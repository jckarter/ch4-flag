
struct flag_mesh {
    GLuint vertex_buffer, element_buffer;
    GLsizei element_count;
    GLuint texture;
};

struct flag_vertex {
    GLfloat position[4];
    GLfloat normal[4];
    GLfloat texcoord[2];
    GLfloat shininess;
    GLubyte specular[4];
};

void init_mesh(
    struct flag_mesh *out_mesh,
    struct flag_vertex const *vertex_data, GLsizei vertex_count,
    GLushort const *element_data, GLsizei element_count,
    GLenum hint
);
struct flag_vertex *init_flag_mesh(struct flag_mesh *out_mesh);
void init_background_mesh(struct flag_mesh *out_mesh);
void update_flag_mesh(
    struct flag_mesh const *mesh,
    struct flag_vertex *vertex_data,
    GLfloat time
);
