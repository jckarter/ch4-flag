static void vec_cross(GLfloat *out_result, GLfloat *u, GLfloat *v)
{
    out_result[0] = u[1]*v[2] - u[2]*v[1];
    out_result[1] = u[2]*v[0] - u[0]*v[2];
    out_result[2] = u[0]*v[1] - u[1]*v[0];
}

static GLfloat vec_length(GLfloat *v)
{
    return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static void vec_normalize(GLfloat *inout_v)
{
    GLfloat rlen = 1.0f/vec_length(inout_v);
    inout_v[0] *= rlen;
    inout_v[1] *= rlen;
    inout_v[2] *= rlen;
}

#ifndef M_PI
#define M_PI 3.141592653589793
#endif
