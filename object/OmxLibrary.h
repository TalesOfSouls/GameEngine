/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_OBJECT_OMX_LIBRARY_H
#define COMS_OBJECT_OMX_LIBRARY_H

#include "../stdlib/Stdlib.h"
#include "Omx.h"
#include "OmxAnimation.h"

// A model references materials, skeletons, animations, hitboxes and particle systems by id.
// These may be defined in the model file itself or in other omx files (shared libraries).
// The library is simply the list of all loaded omx files that should be searched.
// @performance Linear search, for many files this should be replaced by a hash map id -> element (ids are unique per game)

struct OmxLibrary {
    uint32 file_count;
    const OmxFile** files;
};

#define OMX_LIBRARY_FIND(func_name, type, array, count)                                       \
    inline                                                                                    \
    const type* func_name(const OmxLibrary* lib, uint32 id, const OmxFile** owner = NULL) NO_EXCEPT \
    {                                                                                         \
        for (uint32 f = 0; f < lib->file_count; ++f) {                                        \
            const OmxFile* file = lib->files[f];                                              \
            for (uint32 i = 0; i < file->count; ++i) {                                        \
                if (file->array[i].id == id) {                                                \
                    if (owner) {                                                              \
                        *owner = file;                                                        \
                    }                                                                         \
                                                                                              \
                    return &file->array[i];                                                   \
                }                                                                             \
            }                                                                                 \
        }                                                                                     \
                                                                                              \
        return NULL;                                                                          \
    }

OMX_LIBRARY_FIND(omx_find_material, OmxMaterial, materials, material_count)
OMX_LIBRARY_FIND(omx_find_skeleton, OmxSkeleton, skeletons, skeleton_count)
OMX_LIBRARY_FIND(omx_find_animation, OmxAnimation, animations, animation_count)
OMX_LIBRARY_FIND(omx_find_hitbox, OmxHitbox, hitboxes, hitbox_count)
OMX_LIBRARY_FIND(omx_find_particle_system, OmxParticleSystem, particle_systems, particle_system_count)

// Animation id of a gameplay slot (e.g. "run"), OMX_INVALID_ID if the model doesn't support it
inline
uint32 omx_find_slot(const OmxFile* model, uint32 slot_name) NO_EXCEPT
{
    for (uint32 i = 0; i < model->slot_count; ++i) {
        if (model->slots[i].name == slot_name) {
            return model->slots[i].animation;
        }
    }

    return OMX_INVALID_ID;
}

/////////////////////////////////////////////////////////////
// Instance memory
/////////////////////////////////////////////////////////////

// Every placed object (= instance) of a model needs its own memory for the animation state, bone matrices,
// particles and custom state. The layout is identical for all instances of a model
// -> calculate once, then reserve instance_count * size (e.g. in a ChunkMemory)
struct OmxInstanceLayout {
    uint32 bone_count;
    uint32 layer_count;

    uint32 layers_offset;    // OmxAnimationLayer[layer_count]
    uint32 pose_offset;      // OmxTransform[bone_count] (local pose)
    uint32 matrices_offset;  // f32[16 * bone_count] (skinning matrices -> upload to gpu)

    uint32 emitter_count;
    uint32 emitters_offset;  // OmxEmitterState[emitter_count]
    uint32 particles_offset; // OmxParticle[sum of max_particles]
    uint32 particle_count;

    uint32 state_offset;     // custom state fields (see OmxStateField::offset)

    uint32 size;             // total bytes per instance
};

struct OmxEmitterState {
    uint32 particle_offset;  // index into the instance particle array
    uint32 particle_max;
    uint32 alive;
    f32 time;
    f32 spawn_accumulator;
    uint32 active;
};

FORCE_INLINE
uint32 omx_align_instance(uint32 offset, uint32 alignment) NO_EXCEPT
{
    return (offset + alignment - 1) & ~(alignment - 1);
}

// Returns false if a referenced skeleton/particle system is not in the library (-> load the dependency first)
bool omx_instance_layout(const OmxFile* model, const OmxLibrary* lib, OmxInstanceLayout* layout) NO_EXCEPT
{
    memset(layout, 0, sizeof(OmxInstanceLayout));

    if (model->skeleton != OMX_INVALID_ID) {
        const OmxSkeleton* skeleton = omx_find_skeleton(lib, model->skeleton);
        if (!skeleton) {
            return false;
        }

        layout->bone_count = skeleton->bone_count;
        layout->layer_count = model->anim_layers;
    }

    for (uint32 i = 0; i < model->emitter_count; ++i) {
        const OmxParticleSystem* pfx = omx_find_particle_system(lib, model->emitters[i].particle_system);
        if (!pfx) {
            return false;
        }

        layout->particle_count += pfx->max_particles;
    }

    layout->emitter_count = model->emitter_count;

    uint32 offset = 0;

    layout->layers_offset = offset;
    offset += sizeof(OmxAnimationLayer) * layout->layer_count;

    offset = omx_align_instance(offset, 16);
    layout->pose_offset = offset;
    offset += sizeof(OmxTransform) * layout->bone_count;

    offset = omx_align_instance(offset, 64);
    layout->matrices_offset = offset;
    offset += sizeof(f32) * 16 * layout->bone_count;

    offset = omx_align_instance(offset, 16);
    layout->emitters_offset = offset;
    offset += sizeof(OmxEmitterState) * layout->emitter_count;

    offset = omx_align_instance(offset, 16);
    layout->particles_offset = offset;
    offset += sizeof(OmxParticle) * layout->particle_count;

    offset = omx_align_instance(offset, 16);
    layout->state_offset = offset;
    offset += model->state_size;

    // Instances are often stored in arrays -> keep every instance cache line aligned
    layout->size = omx_align_instance(offset, 64);

    return true;
}

// Initializes the memory of one instance (memory must be layout->size bytes)
void omx_instance_init(
    const OmxFile* model,
    const OmxLibrary* lib,
    const OmxInstanceLayout* layout,
    byte* memory
) NO_EXCEPT
{
    memset(memory, 0, layout->size);

    OmxAnimationLayer* layers = (OmxAnimationLayer *) (memory + layout->layers_offset);
    for (uint32 i = 0; i < layout->layer_count; ++i) {
        layers[i].animation = OMX_INVALID_ID;
        layers[i].speed = 1.0f;
    }

    if (layout->bone_count) {
        const OmxFile* skel_file;
        const OmxSkeleton* skeleton = omx_find_skeleton(lib, model->skeleton, &skel_file);
        omx_pose_from_bind(skel_file, skeleton, (OmxTransform *) (memory + layout->pose_offset));
    }

    OmxEmitterState* emitters = (OmxEmitterState *) (memory + layout->emitters_offset);
    uint32 particle_offset = 0;
    for (uint32 i = 0; i < layout->emitter_count; ++i) {
        const OmxParticleSystem* pfx = omx_find_particle_system(lib, model->emitters[i].particle_system);

        emitters[i].particle_offset = particle_offset;
        emitters[i].particle_max = pfx->max_particles;
        emitters[i].active = (model->emitters[i].flags & OMX_EMITTER_AUTO_START) ? 1 : 0;

        particle_offset += pfx->max_particles;
    }
}

#endif
