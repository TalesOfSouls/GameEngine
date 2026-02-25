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

#include "../stdlib/Stdlib.h"

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
    // @bug What if it is a transforming mesh? don't we need the max_vertex_count as well?
    // However, that might be more something for the ECS?
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

#endif