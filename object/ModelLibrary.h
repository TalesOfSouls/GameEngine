/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_OBJECT_MODEL_LIBRARY_H
#define COMS_OBJECT_MODEL_LIBRARY_H

#include "../stdlib/Stdlib.h"
#include "Model.h"
#include "ModelAnimation.h"

// A model references materials, skeletons, animations, hitboxes and particle systems by id.
// These may be defined in the model file itself or in other model files (shared libraries).
// The library is simply the list of all loaded model files that should be searched.
// @performance Linear search, for many files this should be replaced by a hash map id -> element (ids are unique per game)

struct ModelLibrary {
    uint32 file_count;
    const ModelFile** files;
};

#define MODEL_LIBRARY_FIND(func_name, type, array, count)                                       \
    inline                                                                                    \
    const type* func_name(const ModelLibrary* lib, uint32 id, const ModelFile** owner = NULL) NO_EXCEPT \
    {                                                                                         \
        for (uint32 f = 0; f < lib->file_count; ++f) {                                        \
            const ModelFile* file = lib->files[f];                                              \
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

MODEL_LIBRARY_FIND(model_find_material, ModelMaterial, materials, material_count)
MODEL_LIBRARY_FIND(model_find_skeleton, ModelSkeleton, skeletons, skeleton_count)
MODEL_LIBRARY_FIND(model_find_animation, ModelAnimation, animations, animation_count)
MODEL_LIBRARY_FIND(model_find_hitbox, ModelHitbox, hitboxes, hitbox_count)
MODEL_LIBRARY_FIND(model_find_particle_system, ModelParticleSystem, particle_systems, particle_system_count)

// Animation id of a gameplay slot (e.g. "run"), MODEL_INVALID_ID if the model doesn't support it
inline
uint32 model_find_slot(const ModelFile* model, uint32 slot_name) NO_EXCEPT
{
    for (uint32 i = 0; i < model->slot_count; ++i) {
        if (model->slots[i].name == slot_name) {
            return model->slots[i].animation;
        }
    }

    return MODEL_INVALID_ID;
}

/////////////////////////////////////////////////////////////
// Instance memory
/////////////////////////////////////////////////////////////

// Every placed object (= instance) of a model needs its own memory for the animation state, bone matrices,
// particles and custom state. The layout is identical for all instances of a model
// -> calculate once, then reserve instance_count * size (e.g. in a ChunkMemory)
struct ModelInstanceLayout {
    uint32 bone_count;
    uint32 layer_count;

    uint32 layers_offset;    // ModelAnimationLayer[layer_count]
    uint32 pose_offset;      // ModelTransform[bone_count] (local pose)
    uint32 matrices_offset;  // f32[16 * bone_count] (skinning matrices -> upload to gpu)

    uint32 emitter_count;
    uint32 emitters_offset;  // ModelEmitterState[emitter_count]
    uint32 particles_offset; // ModelParticle[sum of max_particles]
    uint32 particle_count;

    uint32 state_offset;     // custom state fields (see ModelStateField::offset)

    uint32 size;             // total bytes per instance
};

struct ModelEmitterState {
    uint32 particle_offset;  // index into the instance particle array
    uint32 particle_max;
    uint32 alive;
    f32 time;
    f32 spawn_accumulator;
    uint32 active;
};

FORCE_INLINE
uint32 model_align_instance(uint32 offset, uint32 alignment) NO_EXCEPT
{
    return (offset + alignment - 1) & ~(alignment - 1);
}

// Returns false if a referenced skeleton/particle system is not in the library (-> load the dependency first)
bool model_instance_layout(const ModelFile* model, const ModelLibrary* lib, ModelInstanceLayout* layout) NO_EXCEPT
{
    memset(layout, 0, sizeof(ModelInstanceLayout));

    if (model->skeleton != MODEL_INVALID_ID) {
        const ModelSkeleton* skeleton = model_find_skeleton(lib, model->skeleton);
        if (!skeleton) {
            return false;
        }

        layout->bone_count = skeleton->bone_count;
        layout->layer_count = model->anim_layers;
    }

    for (uint32 i = 0; i < model->emitter_count; ++i) {
        const ModelParticleSystem* pfx = model_find_particle_system(lib, model->emitters[i].particle_system);
        if (!pfx) {
            return false;
        }

        layout->particle_count += pfx->max_particles;
    }

    layout->emitter_count = model->emitter_count;

    uint32 offset = 0;

    layout->layers_offset = offset;
    offset += sizeof(ModelAnimationLayer) * layout->layer_count;

    offset = model_align_instance(offset, 16);
    layout->pose_offset = offset;
    offset += sizeof(ModelTransform) * layout->bone_count;

    offset = model_align_instance(offset, 64);
    layout->matrices_offset = offset;
    offset += sizeof(f32) * 16 * layout->bone_count;

    offset = model_align_instance(offset, 16);
    layout->emitters_offset = offset;
    offset += sizeof(ModelEmitterState) * layout->emitter_count;

    offset = model_align_instance(offset, 16);
    layout->particles_offset = offset;
    offset += sizeof(ModelParticle) * layout->particle_count;

    offset = model_align_instance(offset, 16);
    layout->state_offset = offset;
    offset += model->state_size;

    // Instances are often stored in arrays -> keep every instance cache line aligned
    layout->size = model_align_instance(offset, 64);

    return true;
}

// Initializes the memory of one instance (memory must be layout->size bytes)
void model_instance_init(
    const ModelFile* model,
    const ModelLibrary* lib,
    const ModelInstanceLayout* layout,
    byte* memory
) NO_EXCEPT
{
    memset(memory, 0, layout->size);

    ModelAnimationLayer* layers = (ModelAnimationLayer *) (memory + layout->layers_offset);
    for (uint32 i = 0; i < layout->layer_count; ++i) {
        layers[i].animation = MODEL_INVALID_ID;
        layers[i].speed = 1.0f;
    }

    if (layout->bone_count) {
        const ModelFile* skel_file;
        const ModelSkeleton* skeleton = model_find_skeleton(lib, model->skeleton, &skel_file);
        model_pose_from_bind(skel_file, skeleton, (ModelTransform *) (memory + layout->pose_offset));
    }

    ModelEmitterState* emitters = (ModelEmitterState *) (memory + layout->emitters_offset);
    uint32 particle_offset = 0;
    for (uint32 i = 0; i < layout->emitter_count; ++i) {
        const ModelParticleSystem* pfx = model_find_particle_system(lib, model->emitters[i].particle_system);

        emitters[i].particle_offset = particle_offset;
        emitters[i].particle_max = pfx->max_particles;
        emitters[i].active = (model->emitters[i].flags & MODEL_EMITTER_AUTO_START) ? 1 : 0;

        particle_offset += pfx->max_particles;
    }
}

#endif
