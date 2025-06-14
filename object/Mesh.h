/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_OBJECT_MESH_H
#define COMS_OBJECT_MESH_H

#include "Vertex.h"
#include "../stdlib/Types.h"
#include "../system/FileUtils.cpp"
#include "../memory/RingMemory.h"
#include "../utils/EndianUtils.h"
#include "../utils/StringUtils.h"
#include "../stdlib/Simd.h"
#include "../compiler/CompilerUtils.h"

#define MESH_VERSION 1

// @todo how to handle different objects and groups?
//      maybe make a mesh hold other meshes?
// @todo handle vertices arrays where for example no texture coordinates are defined/used
struct Mesh {
    byte* data; // memory owner that subdivides into the pointers below

    uint32 object;

    uint32 group_count;
    uint32* groups;

    // the following section allows us to create 2 types of data
    //      Interleaved: [position normal tex_coord] [color] (elements depend on vertex_type)
    //      Separate: [position] [normal] [tex_coord] [color] (separate array for elements)
    uint32 vertex_type;
    uint32 vertex_count; // can mean only position or combination of position, normal, tex, ...
    f32* vertices;

    // @todo this only works if you have sub meshes e.g. one for body, one for hat, one for weapon etc.
    uint32 vertex_ref;
    uint32 vao;
    uint32 vbo;
    uint32 ebo;

    uint32 material_count;
    uint32* materials;

    uint32 texture;

    uint32 animation_count;
    uint32* animations;

    uint32 hitbox_count;
    uint32* hitboxes;

    uint32 audio_count;
    uint32* audios;

    uint32 mesh_count;
    Mesh* meshes;
};

// @todo also handle textures etc.
// WARNING: mesh needs to have memory already reserved and assigned to data
void mesh_from_file_txt(
    Mesh* mesh,
    const char* path,
    RingMemory* ring
) {
    FileBody file = {};
    file_read(path, &file, ring);
    ASSERT_SIMPLE(file.size);

    const char* pos = (char *) file.content;

    // move past the "version" string
    pos += 8;

    // @todo us version for different handling
    [[maybe_unused]] int32 version = (int32) str_to_int(pos, &pos); ++pos;

    int32 object_index = 0;
    int32 group_index = 0;

    mesh->vertices = (f32 *) mesh->data;

    // @todo The temp memory reservation is bad, once the file format is really finalized we need to change this.
    //  We can't just assume these sizes
    int32 vertex_count = 0;
    f32* vertices = (f32 *) ring_get_memory(ring, 500000 * sizeof(f32));

    int32 normal_count = 0;
    f32* normals = (f32 *) ring_get_memory(ring, 500000 * sizeof(f32));

    int32 tex_coord_count = 0;
    f32* tex_coords = (f32 *) ring_get_memory(ring, 500000 * sizeof(f32));

    //int32 color_count = 0;
    // f32* colors = (f32 *) ring_get_memory(ring, 500000 * sizeof(f32));

    int32 face_type = VERTEX_TYPE_POSITION;
    int32 face_count = 0;
    int32* faces = (int32 *) ring_get_memory(ring, 500000 * sizeof(int32));

    uint32 temp_color_count = 0;

    while (*pos != '\0') {
        str_skip_empty(&pos);

        if (*pos == '\0') {
            break;
        }

        // Parse type
        // WARNING: The code below could fail if [1] is outside of range
        //          However that should never happen for well formed files
        // @todo create an enum to make it easier to read
        int32 state = 0;
        if (*pos == 'v' && pos[1] == ' ') {
            state = 1;
        } else if (*pos == 'v' && pos[1] == 'n') {
            state = 2;
        } else if (*pos == 'v' && pos[1] == 't') {
            state = 3;
        } else if (*pos == 'v' && pos[1] == 'p') {
            state = 4;
        } else if (*pos == 'o' && pos[1] == ' ') {
            state = 5;
        } else if (*pos == 's' && pos[1] == ' ') {
            state = 6;
        } else if (*pos == 'f' && pos[1] == ' ') {
            state = 7;
        } else if (*pos == 'g' && pos[1] == ' ') {
            state = 8;
        } else if (*pos == 'l' && pos[1] == ' ') {
            state = 9;
        } else if (*pos == 'm' && pos[1] == 't') {
            state = 10;
        } else if (*pos == 'h' && pos[1] == 'i') {
            state = 11;
        } else if (*pos == 'a' && pos[1] == 'n') {
            state = 12;
        } else if (*pos == 'u' && pos[3] == 'm') {
            state = 13;
        } else if (*pos == 'u' && pos[3] == 'h') {
            state = 14;
        } else if (*pos == 'c' && pos[3] == 'o') {
            state = 15;
        } else {
            // not supported or comment
            str_move_to(&pos, '\n');
        }

        // move past keyword
        str_skip_non_empty(&pos);

        // move past whitespaces and newline
        bool is_next_line = false;
        while (*pos == ' ' || is_eol(pos)) {
            int32 eol_length = is_eol(pos);
            is_next_line |= ((bool) eol_length);
            pos += eol_length ? eol_length : 1;
        }

        if (*pos == '\0' || is_next_line) {
            continue;
        }

        // NOTE: we always load a file in the format: POSITION + NORMAL + TEXTURE + COLOR
        //      EVEN if some of the data is missing. This is necessary to keep the memory kinda in line.
        //      The actual binary file later will have the minimized layout.

        // handle data types
        switch (state) {
            case 0: break;
            case 1: {
                    // 'v'
                    if (vertex_count == 0) {
                        mesh->vertex_type |= VERTEX_TYPE_POSITION;
                    }

                    vertices[vertex_count * 3] = str_to_float(pos, &pos); ++pos;
                    vertices[vertex_count * 3 + 1] = str_to_float(pos, &pos); ++pos;
                    vertices[vertex_count * 3 + 2] = str_to_float(pos, &pos); ++pos;

                    // has color information
                    // @todo Move to own case statement // 'co'
                    if (*pos != '\n' && pos[1] != ' ' && pos[1] != '\n') {
                        if (vertex_count == 0) {
                            mesh->vertex_type |= VERTEX_TYPE_COLOR;
                        }

                        vertices[vertex_count * 12 + 8] = str_to_float(pos, &pos); ++pos;
                        vertices[vertex_count * 12 + 9] = str_to_float(pos, &pos); ++pos;
                        vertices[vertex_count * 12 + 10] = str_to_float(pos, &pos); ++pos;

                        // handle optional alpha [a]
                        if (*pos != '\n' && pos[1] != ' ' && pos[1] != '\n') {
                            vertices[vertex_count * 12 + 11] = str_to_float(pos, &pos); ++pos;
                        } else {
                            vertices[vertex_count * 12 + 11] = 1.0f;
                        }

                        ++temp_color_count;
                    }

                    ++vertex_count;
                } break;
            case 2: {
                    // 'vn'
                    normals[normal_count * 3] = str_to_float(pos, &pos); ++pos;
                    normals[normal_count * 3 + 1] = str_to_float(pos, &pos); ++pos;
                    normals[normal_count * 3 + 2] = str_to_float(pos, &pos); ++pos;

                    ++normal_count;
                } break;
            case 3: {
                    // 'vt'
                    tex_coords[tex_coord_count * 2] = str_to_float(pos, &pos); ++pos;
                    tex_coords[tex_coord_count * 2 + 1] = str_to_float(pos, &pos); ++pos;

                    ++tex_coord_count;
                } break;
            case 4: {
                    // 'vp'
                    str_to_float(pos, &pos); ++pos;

                    // handle optional [v]
                    if (*pos != '\n' && pos[1] != ' ' && pos[1] != '\n') {
                        str_to_float(pos, &pos); ++pos;

                        // handle optional [w]
                        if (*pos != '\n' && pos[1] != ' ' && pos[1] != '\n') {
                            str_to_float(pos, &pos); ++pos;
                        }
                    }
                } break;
            case 5: {
                    // 'o'
                    char text[100];
                    int32 i = 0;
                    while (*pos != '\0' && *pos != ' ') {
                        text[i++] = *pos++;
                    }
                    text[i] = '\0';

                    ++object_index;
                } break;
            case 6: {
                    // 's'
                    str_to_int(pos, &pos); ++pos;
                } break;
            case 7: {
                    // 'f'
                    int32 ftype = 0;
                    const char* tmp = pos;
                    while (*tmp != ' ') {
                        if (*tmp++ == '/') {
                            ++ftype;

                            if (*tmp++ == '/') {
                                ftype = 3;
                                break;
                            }
                        }
                    }

                    const int32 max_blocks = 3; // @todo this could actually be N. Might have to change in the future
                    int32 block = 0;

                    while (*pos != '\0' && !is_eol(pos)) {
                        if (ftype == 0) {
                            // v1 v2 v3 ...
                            if (face_count == 0) {
                                face_type = VERTEX_TYPE_POSITION;
                            }

                            faces[(face_count * max_blocks * 1) + block] = (int32) str_to_int(pos, &pos) - 1; ++pos;
                        } else if (ftype == 1) {
                            // v1/vt1 v2/vt2 v3/vt3 ...
                            if (face_count == 0) {
                                face_type = VERTEX_TYPE_POSITION | VERTEX_TYPE_TEXTURE_COORD;
                            }

                            faces[(face_count * max_blocks * 2) + block * 2] = (int32) str_to_int(pos, &pos) - 1; ++pos;
                            faces[(face_count * max_blocks * 2) + block * 2 + 1] = (int32) str_to_int(pos, &pos) - 1; ++pos;
                        } else if (ftype == 2) {
                            // v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ...
                            if (face_count == 0) {
                                face_type = VERTEX_TYPE_POSITION | VERTEX_TYPE_TEXTURE_COORD | VERTEX_TYPE_NORMAL;
                            }

                            faces[(face_count * max_blocks * 3) + block * 3] = (int32) str_to_int(pos, &pos) - 1; ++pos;
                            faces[(face_count * max_blocks * 3) + block * 3 + 1] = (int32) str_to_int(pos, &pos) - 1; ++pos;
                            faces[(face_count * max_blocks * 3) + block * 3 + 2] = (int32) str_to_int(pos, &pos) - 1; ++pos;
                        } else if (ftype == 3) {
                            // v1//vn1 v2//vn2 v3//vn3 ...
                            if (face_count == 0) {
                                face_type = VERTEX_TYPE_POSITION | VERTEX_TYPE_NORMAL;
                            }

                            faces[(face_count * max_blocks * 2) + block * 2] = (int32) str_to_int(pos, &pos) - 1; ++pos;
                            ++pos;
                            faces[(face_count * max_blocks * 2) + block * 2 + 1] = (int32) str_to_int(pos, &pos) - 1; ++pos;
                        }

                        ++block;
                    }

                    ++face_count;
                } break;
            case 8: {
                    // 'g'
                    char text[100];
                    int32 i = 0;
                    while (*pos != '\0' && *pos != ' ') {
                        text[i++] = *pos++;
                    }
                    text[i] = '\0';

                    ++group_index;
                } break;
            case 9: {
                    //l
                    while (*pos != '\0' && !is_eol(pos)) {
                        str_to_int(pos, &pos); ++pos;
                    }
                } break;
            case 10: {
                    // 'mtllib'
                    char text[100];
                    int32 i = 0;
                    while (*pos != '\0' && *pos != ' ') {
                        text[i++] = *pos++;
                    }
                    text[i] = '\0';
                } break;
            case 11: {
                    // 'hitlib'
                    char text[100];
                    int32 i = 0;
                    while (*pos != '\0' && *pos != ' ') {
                        text[i++] = *pos++;
                    }
                    text[i] = '\0';
                } break;
            case 12: {
                    // 'anilib'
                    char text[100];
                    int32 i = 0;
                    while (*pos != '\0' && *pos != ' ') {
                        text[i++] = *pos++;
                    }
                    text[i] = '\0';
                } break;
            case 13: {
                    // 'usemtl'
                    char text[100];
                    int32 i = 0;
                    while (*pos != '\0' && *pos != ' ') {
                        text[i++] = *pos++;
                    }
                    text[i] = '\0';
                } break;
            case 14: {
                    // 'usehit'
                    char text[100];
                    int32 i = 0;
                    while (*pos != '\0' && *pos != ' ') {
                        text[i++] = *pos++;
                    }
                    text[i] = '\0';
                } break;
            default: {
                UNREACHABLE();
            }
        }
    }

    mesh->vertex_type = face_type;

    // Populate the vertex data based on the face data
    if (face_count > 0) {
        int32 vertex_size = 3;
        int32 face_size = 1;

        // We need this since in the text file textures are defined before the normals (v/vt/vn)
        int32 normal_offset = ((mesh->vertex_type & VERTEX_TYPE_TEXTURE_COORD) ? 1 : 0);

        int32 texture_offset = ((mesh->vertex_type & VERTEX_TYPE_NORMAL) ? 1 : 0);

        if (mesh->vertex_type & VERTEX_TYPE_NORMAL) {
            vertex_size += 3;
            ++face_size;
        }

        if (mesh->vertex_type & VERTEX_TYPE_TEXTURE_COORD) {
            vertex_size += 2;
            ++face_size;
        }

        if (mesh->vertex_type & VERTEX_TYPE_COLOR) {
            vertex_size += 4;
            ++face_size;
        }

        mesh->vertex_count = face_count * 3;

        // Every face consists of 3 vertices -> *3
        for (int32 i = 0; i < face_count * 3; ++i) {
            const f32* vertex_pos = &vertices[faces[i * face_size] * 3];
            memcpy(
                mesh->data + i * vertex_size * sizeof(f32),
                vertex_pos,
                3 * sizeof(f32)
            );

            // Normals come before texture coordinates since we most likely need them more than texture (e.g. pre-computing of shadows on cpu etc.)
            if (mesh->vertex_type & VERTEX_TYPE_NORMAL) {
                const f32* normal_pos = &normals[faces[i * face_size + 1 + normal_offset] * 3];

                memcpy(
                    mesh->data + i * vertex_size * sizeof(f32)
                        + 3 * sizeof(f32), // Offset by position
                    normal_pos,
                    3 * sizeof(f32)
                );
            }

            if (mesh->vertex_type & VERTEX_TYPE_TEXTURE_COORD) {
                const f32* texture_pos = &tex_coords[faces[i * face_size + 1] * 2];

                memcpy(
                    mesh->data + i * vertex_size * sizeof(f32)
                        + 3 * sizeof(f32) // Offset by position
                        + texture_offset * 3 * sizeof(f32), // Offset by normal
                    texture_pos,
                    2 * sizeof(f32)
                );
            }
        }
    } else {
        // No face data -> just output the vertices
        mesh->vertex_count = vertex_count;
        memcpy(mesh->vertices, vertices, 3 * sizeof(f32) * mesh->vertex_count);
    }
}

enum MeshLoadingRestriction {
    MESH_LOADING_RESTRICTION_POSITION = 1,
    MESH_LOADING_RESTRICTION_NORMAL = 2,
    MESH_LOADING_RESTRICTION_TEXTURE = 4,
    MESH_LOADING_RESTRICTION_COLOR = 8,
    MESH_LOADING_RESTRICTION_FACES = 16,
    MESH_LOADING_RESTRICTION_EVERYTHING = 31
};

// @todo sometimes we don't care about some data, we should have an option which defines which data should be loaded
//      this can improve performance for algorithms on this. e.g.:
//      on the server side we only care about the vertex positions for collision (no normals, no color, ...)
int32 mesh_from_data(
    const byte* data,
    Mesh* mesh,
    //int32 load_format = MESH_LOADING_RESTRICTION_EVERYTHING,
    [[maybe_unused]] int32 steps = 8
)
{
    LOG_3("Load mesh");
    const byte* pos = data;

    // Read version, use to handle different versions differently
    int32 version = *((int32 *) pos);
    pos += sizeof(version);

    // Read base data
    mesh->vertex_type = *((int32 *) pos);
    pos += sizeof(mesh->vertex_type);

    mesh->vertex_count = *((int32 *) pos);
    pos += sizeof(mesh->vertex_count);

    #if !_WIN32 && !__LITTLE_ENDIAN__
        mesh->version = endian_swap(mesh->version);
        mesh->vertex_type = endian_swap(mesh->vertex_type);
        mesh->vertex_count = endian_swap(mesh->vertex_count);
    #endif

    int32 vertex_size = 0;
    if (mesh->vertex_type & VERTEX_TYPE_POSITION) {
        vertex_size += 3;
    }

    if (mesh->vertex_type & VERTEX_TYPE_NORMAL) {
        vertex_size += 3;
    }

    if (mesh->vertex_type & VERTEX_TYPE_TEXTURE_COORD) {
        vertex_size += 2;
    }

    if (mesh->vertex_type & VERTEX_TYPE_COLOR) {
        vertex_size += 4;
    }

    int32 offset = 0;
    if (mesh->vertex_count > 0) {
        memcpy(mesh->data, pos, sizeof(f32) * vertex_size * mesh->vertex_count);
        mesh->vertices = (f32 *) mesh->data;

        pos += sizeof(f32) * vertex_size * mesh->vertex_count;
        offset += sizeof(f32) * vertex_size * mesh->vertex_count;
    }

    /*
    #if OPENGL
        if (mesh->vertex_type & VERTEX_TYPE_NORMAL) {
            for (int i = 0; i < mesh->vertex_count; ++i) {
                mesh->vertices[i * vertex_size + 3] *= -1;
                mesh->vertices[i * vertex_size + 3 + 1] *= -1;
                mesh->vertices[i * vertex_size + 3 + 2] *= -1;
            }
        }
    #endif
    */

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) mesh->data,
        (int32 *) mesh->data,
        offset / 4, // everything is 4 bytes -> easy to swap
        steps
    );

    LOG_3("Loaded mesh");

    return offset;
}

// @bug this is wrong, since it is the max size
// We would have to check the vertex format to calculate the actual size
int32 mesh_data_size(const Mesh* mesh)
{
    return sizeof(int32)
        + sizeof(mesh->vertex_type)
        + sizeof(mesh->vertex_count)
        + 12 * sizeof(f32) * mesh->vertex_count; // 12 is the maximum value
}

int32 mesh_to_data(
    const Mesh* mesh,
    byte* data,
    uint32 vertex_save_format = VERTEX_TYPE_ALL,
    [[maybe_unused]] int32 steps = 8
)
{
    byte* pos = data;

    // version
    *((int32 *) pos) = MESH_VERSION;
    pos += sizeof(int32);

    // vertices
    if (vertex_save_format == VERTEX_TYPE_ALL) {
        vertex_save_format = mesh->vertex_type;
    }

    memcpy(pos, &vertex_save_format, sizeof(vertex_save_format));
    pos += sizeof(vertex_save_format);

    memcpy(pos, &mesh->vertex_count, sizeof(mesh->vertex_count));
    pos += sizeof(mesh->vertex_count);

    // vertices
    int32 vertex_size = 0;
    if (mesh->vertex_type & VERTEX_TYPE_POSITION) {
        vertex_size += 3;
    }

    if (mesh->vertex_type & VERTEX_TYPE_NORMAL) {
        vertex_size += 3;
    }

    if (mesh->vertex_type & VERTEX_TYPE_TEXTURE_COORD) {
        vertex_size += 2;
    }

    if (mesh->vertex_type & VERTEX_TYPE_COLOR) {
        vertex_size += 4;
    }

    int32 out_vertex_size = 0;
    if (vertex_save_format & VERTEX_TYPE_POSITION) {
        out_vertex_size += 3;
    }

    if (vertex_save_format & VERTEX_TYPE_NORMAL) {
        out_vertex_size += 3;
    }

    if (vertex_save_format & VERTEX_TYPE_TEXTURE_COORD) {
        out_vertex_size += 2;
    }

    if (vertex_save_format & VERTEX_TYPE_COLOR) {
        out_vertex_size += 4;
    }

    if ((mesh->vertex_type == VERTEX_TYPE_ALL && vertex_save_format == VERTEX_TYPE_ALL)
        || (mesh->vertex_type == vertex_save_format)
    ) {
        // data is the same as in the array
        memcpy(pos, mesh->vertices, vertex_size * sizeof(f32) * mesh->vertex_count);
        pos += vertex_size * sizeof(f32) * mesh->vertex_count;
    }

    int32 size = (int32) (pos - data);

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) data,
        (int32 *) data,
        size / 4, // everything in here is 4 bytes -> easy to swap
        steps
    );

    return size;
}

#endif