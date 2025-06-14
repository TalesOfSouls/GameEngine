void make_player(
    float *data,
    float x, float y, float z, float rx, float ry)
{
    float ao[6][4] = {0};
    float light[6][4] = {
        {0.8, 0.8, 0.8, 0.8},
        {0.8, 0.8, 0.8, 0.8},
        {0.8, 0.8, 0.8, 0.8},
        {0.8, 0.8, 0.8, 0.8},
        {0.8, 0.8, 0.8, 0.8},
        {0.8, 0.8, 0.8, 0.8}
    };

    make_cube_faces(
        data, ao, light,
        1, 1, 1, 1, 1, 1,
        226, 224, 241, 209, 225, 227,
        0, 0, 0, 0.4
    );

    float ma[16];
    float mb[16];

    mat_identity(ma);
    mat_rotate(mb, 0, 1, 0, rx);
    mat_multiply(ma, mb, ma);
    mat_rotate(mb, cosf(rx), 0, sinf(rx), -ry);
    mat_multiply(ma, mb, ma);
    mat_apply(data, ma, 36, 3, 10);
    mat_translate(mb, x, y, z);
    mat_multiply(ma, mb, ma);
    mat_apply(data, ma, 36, 0, 10);
}