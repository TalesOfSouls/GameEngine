/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_OBJECT_MODEL_H
#define COMS_OBJECT_MODEL_H

#include "../stdlib/Stdlib.h"
#include "../animation/AnimationEaseType.h"

// Model file: models + material, animation, hitbox and particle libraries
// Text format: *.modeltxt (@see ModelParser.h, Design-Documentation/File Formats/Model Format.md)
// Binary format: *.modelbin / asset archive payload (@see model_to_data, model_from_data below)
// The same grammar is used for models, material libraries, animation libraries, hitbox libraries and particle libraries.
// A file is parsed in 2 passes:
//      1. model_measure() counts all elements -> model_memory_size() tells you how much memory to reserve
//      2. model_parse() fills the reserved memory
// All arrays live in ONE memory block (ModelFile::data) which also makes the binary format trivial (@see model_to_data)

#define MODEL_VERSION 1
#define MODEL_INVALID_ID 0xFFFFFFFF
#define MODEL_INVALID_BONE 0xFFFF
#define MODEL_MAX_BONE_INFLUENCES 4
#define MODEL_MAX_CURVE_KEYS 4
#define MODEL_MEMORY_ALIGNMENT 16

/////////////////////////////////////////////////////////////
// Vertex
/////////////////////////////////////////////////////////////

// The memory order of the attributes is always the order below (= canonical order)
// The text layout line MUST use the same order
enum ModelVertexAttribute : uint32 {
    MODEL_VERTEX_POSITION = 1 << 0, // 3 x f32
    MODEL_VERTEX_NORMAL   = 1 << 1, // 3 x f32
    MODEL_VERTEX_TANGENT  = 1 << 2, // 4 x f32 (w = bitangent sign)
    MODEL_VERTEX_UV0      = 1 << 3, // 2 x f32
    MODEL_VERTEX_UV1      = 1 << 4, // 2 x f32 (e.g. lightmap, detail map)
    MODEL_VERTEX_COLOR    = 1 << 5, // 1 x uint32 (rgba8, r = lowest byte)
    MODEL_VERTEX_SKIN     = 1 << 6, // 4 x uint16 bone index + 4 x f32 weight
};

#define MODEL_VERTEX_ATTRIBUTE_COUNT 7

// Byte size of every attribute in canonical order
static const uint32 MODEL_VERTEX_ATTRIBUTE_SIZE[MODEL_VERTEX_ATTRIBUTE_COUNT] = {
    12, 12, 16, 8, 8, 4, 24
};

CONSTEXPR FORCE_INLINE
uint32 model_vertex_stride(uint32 layout) NO_EXCEPT
{
    uint32 stride = 0;
    for (int32 i = 0; i < MODEL_VERTEX_ATTRIBUTE_COUNT; ++i) {
        if (layout & (1 << i)) {
            stride += MODEL_VERTEX_ATTRIBUTE_SIZE[i];
        }
    }

    return stride;
}

// Byte offset of an attribute inside of a vertex, -1 if the layout doesn't contain the attribute
CONSTEXPR FORCE_INLINE
int32 model_vertex_offset(uint32 layout, ModelVertexAttribute attribute) NO_EXCEPT
{
    if (!(layout & attribute)) {
        return -1;
    }

    int32 offset = 0;
    for (int32 i = 0; i < MODEL_VERTEX_ATTRIBUTE_COUNT && (uint32) (1 << i) != (uint32) attribute; ++i) {
        if (layout & (1 << i)) {
            offset += MODEL_VERTEX_ATTRIBUTE_SIZE[i];
        }
    }

    return offset;
}

struct ModelVertexSkin {
    uint16 bones[MODEL_MAX_BONE_INFLUENCES];
    f32 weights[MODEL_MAX_BONE_INFLUENCES];
};

/////////////////////////////////////////////////////////////
// Assets
/////////////////////////////////////////////////////////////

enum ModelAssetType : byte {
    MODEL_ASSET_MODEL,    // another model file (material lib, animation lib, hitbox lib, ...)
    MODEL_ASSET_TEXTURE,  // texture or texture atlas
    MODEL_ASSET_AUDIO,
    MODEL_ASSET_SHADER,
    MODEL_ASSET_FONT,
    MODEL_ASSET_DATA,     // anything else
};

// Dependency declaration. At runtime only the id matters, the path is for the build tools / hot reloading
struct ModelAsset {
    uint32 id;
    uint32 path_offset; // offset into ModelFile::strings
    ModelAssetType type;
};

/////////////////////////////////////////////////////////////
// Material (PBR metallic/roughness + common extensions)
/////////////////////////////////////////////////////////////

enum ModelAlphaMode : byte {
    MODEL_ALPHA_OPAQUE,
    MODEL_ALPHA_MASK,  // alpha test with alpha_cutoff
    MODEL_ALPHA_BLEND,
    MODEL_ALPHA_ADD,   // additive (fire, magic, ...)
};

enum ModelMaterialFlag : uint16 {
    MODEL_MATERIAL_DOUBLE_SIDED   = 1 << 0,
    MODEL_MATERIAL_UNLIT          = 1 << 1,
    MODEL_MATERIAL_CAST_SHADOW    = 1 << 2,
    MODEL_MATERIAL_RECEIVE_SHADOW = 1 << 3,
    MODEL_MATERIAL_VERTEX_COLOR   = 1 << 4, // multiply base color with vertex color
    MODEL_MATERIAL_THIN_WALLED    = 1 << 5, // transmission without refraction (leaves, paper, ...)
};

enum ModelTextureSlot : byte {
    MODEL_TEXTURE_BASE,             // rgba
    MODEL_TEXTURE_NORMAL,           // tangent space, scale = normal strength
    MODEL_TEXTURE_ORM,              // r = occlusion, g = roughness, b = metallic, scale = occlusion strength
    MODEL_TEXTURE_EMISSIVE,
    MODEL_TEXTURE_TRANSMISSION,
    MODEL_TEXTURE_SUBSURFACE,
    MODEL_TEXTURE_CLEARCOAT,
    MODEL_TEXTURE_CLEARCOAT_NORMAL,
    MODEL_TEXTURE_SHEEN,
    MODEL_TEXTURE_ANISOTROPY,
    MODEL_TEXTURE_SPECULAR,
    MODEL_TEXTURE_HEIGHT,           // parallax, scale = height scale
    MODEL_TEXTURE_SLOT_SIZE,
};

struct ModelTextureRef {
    uint32 asset;      // texture/atlas asset id, MODEL_INVALID_ID = unused
    uint32 region;     // texture atlas element, MODEL_INVALID_ID = whole texture
    f32 scale;         // meaning depends on slot (see ModelTextureSlot)
    v2_f32 uv_offset;
    v2_f32 uv_scale;   // tiling
    byte uv_set;       // 0 = uv0, 1 = uv1
};

struct ModelMaterial {
    uint32 id;
    uint32 name;

    v4_f32 base_color;
    v3_f32 emissive;
    f32 emissive_strength;

    f32 metallic;
    f32 roughness;
    f32 specular;      // dielectric F0 control, 0.5 = 4%
    f32 ior;

    // Translucency
    f32 transmission;
    f32 thickness;
    v3_f32 attenuation_color;
    f32 attenuation_distance; // 0 = infinite

    // Subsurface scattering (skin, wax, marble)
    f32 subsurface;
    v3_f32 subsurface_color;
    f32 subsurface_radius;

    f32 clearcoat;
    f32 clearcoat_roughness;

    v3_f32 sheen_color; // cloth
    f32 sheen_roughness;

    f32 anisotropy;     // brushed metal, hair
    f32 anisotropy_rotation;

    f32 iridescence;    // soap bubbles, oil
    f32 iridescence_ior;

    f32 alpha_cutoff;

    uint32 shader;      // optional custom shader asset id, MODEL_INVALID_ID = default pbr

    ModelAlphaMode alpha_mode;
    uint16 flags;       // ModelMaterialFlag

    ModelTextureRef textures[MODEL_TEXTURE_SLOT_SIZE];
};

/////////////////////////////////////////////////////////////
// Skeleton
/////////////////////////////////////////////////////////////

// Bind pose in local space (relative to the parent)
struct ModelBone {
    uint32 name;      // hash, animations target bones by name -> animations are shareable
    uint16 parent;    // index inside of the skeleton, MODEL_INVALID_BONE = root
    uint16 flags;
    v3_f32 position;
    v3_f32 scale;
    quaternion rotation;
};

struct ModelSkeleton {
    uint32 id;
    uint32 name;
    uint32 bone_offset; // into ModelFile::bones
    uint32 bone_count;
    f32 height;         // reference height (e.g. hip height) used to scale root motion between different models
};

/////////////////////////////////////////////////////////////
// Animation
/////////////////////////////////////////////////////////////

enum ModelChannelPath : byte {
    MODEL_CHANNEL_POSITION,
    MODEL_CHANNEL_ROTATION,
    MODEL_CHANNEL_SCALE,
};

enum ModelAnimationFlag : uint32 {
    MODEL_ANIMATION_LOOP             = 1 << 0,
    MODEL_ANIMATION_ADDITIVE         = 1 << 1, // applied on top of another animation (e.g. breathing, aiming)
    MODEL_ANIMATION_ROOT_MOTION      = 1 << 2, // root translation moves the entity
    MODEL_ANIMATION_FULL_TRANSLATION = 1 << 3, // also apply non-root translations (only correct on the authoring skeleton)
};

// Value from this key to the next key is interpolated with ease
struct ModelKeyframe {
    f32 time;
    f32 value[4]; // position/scale = xyz, rotation = quaternion xyzw
    AnimationEaseType ease;
};

struct ModelChannel {
    uint32 bone;        // bone name hash
    uint32 key_offset;  // into ModelFile::keys
    uint32 key_count;
    ModelChannelPath path;
};

enum ModelEventType : byte {
    MODEL_EVENT_SOUND,    // ref = audio asset id
    MODEL_EVENT_PARTICLE, // ref = emitter name hash (defined on the model -> the animation doesn't need to know the effect)
    MODEL_EVENT_TAG,      // ref = tag name hash (e.g. footstep_l, hit_start, hit_end, combo_window)
};

struct ModelEvent {
    f32 time;
    uint32 ref;
    uint32 bone;        // bone name hash, 0 = model origin
    ModelEventType type;
};

struct ModelAnimation {
    uint32 id;
    uint32 name;
    uint32 rig;         // skeleton id the animation was authored for (optional, MODEL_INVALID_ID)
    uint32 flags;       // ModelAnimationFlag
    f32 duration;
    f32 speed;          // default playback speed

    uint32 channel_offset; // into ModelFile::channels
    uint32 channel_count;

    uint32 event_offset;   // into ModelFile::events (sorted by time)
    uint32 event_count;
};

/////////////////////////////////////////////////////////////
// Hitbox
/////////////////////////////////////////////////////////////

enum ModelShapeType : byte {
    MODEL_SHAPE_SPHERE,  // a = center, radius
    MODEL_SHAPE_CAPSULE, // a = start, b = end, radius
    MODEL_SHAPE_BOX,     // a = center, b = half extents, rotation
};

enum ModelHitLayer : byte {
    MODEL_HIT_LAYER_WORLD,  // world collision
    MODEL_HIT_LAYER_BODY,   // receives damage
    MODEL_HIT_LAYER_HEAD,   // receives damage (usually with multiplier)
    MODEL_HIT_LAYER_WEAPON, // deals damage (usually only active during hit_start - hit_end tag events)
    MODEL_HIT_LAYER_TRIGGER,
};

// Shapes are defined in bone space and follow the bone
struct ModelShape {
    uint32 bone;        // bone name hash, 0 = model origin
    v3_f32 a;
    v3_f32 b;
    f32 radius;
    f32 multiplier;     // e.g. damage multiplier
    quaternion rotation;
    ModelShapeType type;
    ModelHitLayer layer;
};

struct ModelHitbox {
    uint32 id;
    uint32 name;
    uint32 shape_offset; // into ModelFile::shapes
    uint32 shape_count;
};

/////////////////////////////////////////////////////////////
// Particles
/////////////////////////////////////////////////////////////

enum ModelEmitterShape : byte {
    MODEL_EMITTER_POINT,
    MODEL_EMITTER_SPHERE, // shape_param.x = radius
    MODEL_EMITTER_CONE,   // shape_param.x = angle (deg), shape_param.y = base radius
    MODEL_EMITTER_BOX,    // shape_param = half extents
};

enum ModelParticleFlag : uint16 {
    MODEL_PARTICLE_LOOP        = 1 << 0,
    MODEL_PARTICLE_WORLD_SPACE = 1 << 1, // particles stay in the world when the emitter moves (smoke trails)
    MODEL_PARTICLE_STRETCH     = 1 << 2, // velocity aligned billboards (sparks, rain)
    MODEL_PARTICLE_COLLIDE     = 1 << 3,
};

struct ModelColorKey {
    f32 time; // 0-1 normalized life time
    v4_f32 color;
};

struct ModelFloatKey {
    f32 time;
    f32 value;
};

struct ModelParticleSystem {
    uint32 id;
    uint32 name;
    uint32 material;      // material id (usually additive/blend with a flipbook texture atlas)
    uint32 max_particles; // memory reservation per emitter instance

    f32 rate;             // particles per second
    uint32 burst;         // particles spawned at start
    f32 duration;         // 0 = infinite

    v2_f32 life;          // min, max (seconds)
    v2_f32 speed;         // min, max
    v2_f32 spin;          // min, max (deg/s)
    v3_f32 gravity;       // acceleration
    f32 drag;
    v3_f32 shape_param;

    // Flipbook inside of the material texture (atlas)
    uint16 frames_x;
    uint16 frames_y;
    f32 fps;              // 0 = frame based on normalized life time

    byte color_key_count;
    byte size_key_count;
    ModelColorKey color[MODEL_MAX_CURVE_KEYS];
    ModelFloatKey size[MODEL_MAX_CURVE_KEYS];

    ModelEmitterShape shape;
    uint16 flags;         // ModelParticleFlag
};

// Runtime particle (used to calculate the per instance memory)
struct ModelParticle {
    v3_f32 position;
    f32 age;
    v3_f32 velocity;
    f32 life;
    f32 rotation;
    f32 spin;
};

/////////////////////////////////////////////////////////////
// Model
/////////////////////////////////////////////////////////////

// A draw range with one material
struct ModelMesh {
    uint32 name;
    uint32 material;     // material id, MODEL_INVALID_ID = default material
    uint32 index_offset; // into ModelFile::indices
    uint32 index_count;
};

// Maps a gameplay animation slot (e.g. "run") to an animation id
// -> different models can use different animations for the same slot
struct ModelAnimationSlot {
    uint32 name;
    uint32 animation;
};

// Attachment point (weapons, hats, ...)
struct ModelSocket {
    uint32 name;
    uint32 bone;
    v3_f32 position;
    quaternion rotation;
};

enum ModelEmitterFlag : uint16 {
    MODEL_EMITTER_AUTO_START = 1 << 0, // e.g. torch fire, otherwise started by animation events
};

// Particle system instance attached to the model
struct ModelEmitter {
    uint32 name;
    uint32 particle_system;
    uint32 bone;
    v3_f32 offset;
    uint16 flags;
};

// Point light attached to the model (e.g. lamp, torch, magic crystal)
// The runtime may switch/scale it per instance (e.g. on/off state of the entity)
struct ModelLight {
    uint32 name;
    uint32 bone;        // bone name hash, 0 = model origin
    v3_f32 offset;      // relative to the bone/origin
    v3_f32 color;       // linear rgb
    f32 intensity;
    f32 range;          // influence radius, the light is 0 at this distance
};

enum ModelStateType : byte {
    MODEL_STATE_BYTE,
    MODEL_STATE_INT32,
    MODEL_STATE_UINT32,
    MODEL_STATE_F32,
    MODEL_STATE_V2,
    MODEL_STATE_V3,
    MODEL_STATE_V4,
    MODEL_STATE_MAT4,
    MODEL_STATE_TYPE_SIZE,
};

static const uint32 MODEL_STATE_TYPE_BYTES[MODEL_STATE_TYPE_SIZE] = { 1, 4, 4, 4, 8, 12, 16, 64 };
static const uint32 MODEL_STATE_TYPE_ALIGNMENT[MODEL_STATE_TYPE_SIZE] = { 1, 4, 4, 4, 4, 4, 16, 16 };

// Custom per instance memory the model needs (e.g. door open state, health, cooldowns)
struct ModelStateField {
    uint32 name;
    uint32 offset;  // inside of the state block
    uint32 count;
    ModelStateType type;
};

/////////////////////////////////////////////////////////////
// File
/////////////////////////////////////////////////////////////

struct ModelFile {
    byte* data; // memory owner that subdivides into the pointers below
    uint64 data_size;

    uint32 version;
    uint32 name; // model name hash, 0 = file is only a library

    // Libraries (everything in here can be referenced by other files through its id)
    uint32 asset_count;
    ModelAsset* assets;

    uint32 string_size;
    char* strings;

    uint32 material_count;
    ModelMaterial* materials;

    uint32 skeleton_count;
    ModelSkeleton* skeletons;

    uint32 bone_count;
    ModelBone* bones;

    uint32 animation_count;
    ModelAnimation* animations;

    uint32 channel_count;
    ModelChannel* channels;

    uint32 key_count;
    ModelKeyframe* keys;

    uint32 event_count;
    ModelEvent* events;

    uint32 hitbox_count;
    ModelHitbox* hitboxes;

    uint32 shape_count;
    ModelShape* shapes;

    uint32 particle_system_count;
    ModelParticleSystem* particle_systems;

    // Model
    uint32 vertex_layout; // ModelVertexAttribute
    uint32 vertex_stride;
    uint32 vertex_count;
    byte* vertices;

    uint32 index_count;
    uint32* indices;

    uint32 mesh_count;
    ModelMesh* meshes;

    uint32 slot_count;
    ModelAnimationSlot* slots;

    uint32 socket_count;
    ModelSocket* sockets;

    uint32 emitter_count;
    ModelEmitter* emitters;

    uint32 light_count;
    ModelLight* lights;

    uint32 state_field_count;
    ModelStateField* state_fields;

    // Bytes of the custom per instance state: all state fields, at least the memory requirement of the file
    // (the runtime/entity class interprets the bytes, the file doesn't need to describe them)
    uint32 state_size;
    uint32 anim_layers;    // max. simultaneously blended animations per instance
    uint32 skeleton;       // skeleton id, MODEL_INVALID_ID = static model
    uint32 hitbox;         // hitbox id, MODEL_INVALID_ID = none

    v3_f32 bounds_min;
    v3_f32 bounds_max;
};

/////////////////////////////////////////////////////////////
// Hash
/////////////////////////////////////////////////////////////

// FNV-1a 32 bit. 0 is reserved for "no name"
CONSTEXPR FORCE_INLINE
uint32 model_hash(const char* str, int32 length) NO_EXCEPT
{
    uint32 hash = 2166136261u;
    for (int32 i = 0; i < length; ++i) {
        hash ^= (byte) str[i];
        hash *= 16777619u;
    }

    return hash ? hash : 1;
}

CONSTEXPR FORCE_INLINE
uint32 model_hash(const char* str) NO_EXCEPT
{
    int32 length = 0;
    while (str[length] != '\0') {
        ++length;
    }

    return model_hash(str, length);
}

/////////////////////////////////////////////////////////////
// Lookup helpers
/////////////////////////////////////////////////////////////

CONSTEXPR FORCE_INLINE
const ModelBone* model_skeleton_bones(const ModelFile* __restrict file, const ModelSkeleton* __restrict skeleton) NO_EXCEPT
{
    return file->bones + skeleton->bone_offset;
}

CONSTEXPR FORCE_INLINE
const ModelChannel* model_animation_channels(const ModelFile* __restrict file, const ModelAnimation* __restrict anim) NO_EXCEPT
{
    return file->channels + anim->channel_offset;
}

CONSTEXPR FORCE_INLINE
const ModelEvent* model_animation_events(const ModelFile* __restrict file, const ModelAnimation* __restrict anim) NO_EXCEPT
{
    return file->events + anim->event_offset;
}

CONSTEXPR FORCE_INLINE
const ModelShape* model_hitbox_shapes(const ModelFile* __restrict file, const ModelHitbox* __restrict hitbox) NO_EXCEPT
{
    return file->shapes + hitbox->shape_offset;
}

CONSTEXPR FORCE_INLINE
const char* model_asset_path(const ModelFile* __restrict file, const ModelAsset* __restrict asset) NO_EXCEPT
{
    return file->strings + asset->path_offset;
}

// Bone index inside of the skeleton, MODEL_INVALID_BONE if not found
CONSTEXPR inline
uint16 model_skeleton_find_bone(const ModelFile* __restrict file, const ModelSkeleton* __restrict skeleton, uint32 name) NO_EXCEPT
{
    const ModelBone* bones = model_skeleton_bones(file, skeleton);
    for (uint32 i = 0; i < skeleton->bone_count; ++i) {
        if (bones[i].name == name) {
            return (uint16) i;
        }
    }

    return MODEL_INVALID_BONE;
}

CONSTEXPR inline
const ModelStateField* model_find_state_field(const ModelFile* file, uint32 name) NO_EXCEPT
{
    for (uint32 i = 0; i < file->state_field_count; ++i) {
        if (file->state_fields[i].name == name) {
            return &file->state_fields[i];
        }
    }

    return NULL;
}

/////////////////////////////////////////////////////////////
// Binary format
/////////////////////////////////////////////////////////////
// [ModelBinaryHeader][ModelFile with the pointers replaced by offsets][padding][data block]
// All data is already in one block (ModelFile::data) -> the binary data is just a copy of it.
// Loading = copy (or use in place) + fix up the pointers, no parsing
// @todo Big endian platforms (all values are little endian)

#define MODEL_BINARY_MAGIC 0x314C444D // "MDL1"

#define MODEL_POINTER_LIST(X) \
    X(assets)               \
    X(strings)              \
    X(materials)            \
    X(skeletons)            \
    X(bones)                \
    X(animations)           \
    X(channels)             \
    X(keys)                 \
    X(events)               \
    X(hitboxes)             \
    X(shapes)               \
    X(particle_systems)     \
    X(vertices)             \
    X(indices)              \
    X(meshes)               \
    X(slots)                \
    X(sockets)              \
    X(emitters)             \
    X(lights)               \
    X(state_fields)

struct ModelBinaryHeader {
    uint32 magic;
    uint32 header_size; // = sizeof(ModelBinaryHeader) + sizeof(ModelFile), used as compatibility check
    uint64 data_size;
};

// The data block starts aligned behind the header -> the binary data can be used in place
#define MODEL_BINARY_DATA_OFFSET ((sizeof(ModelBinaryHeader) + sizeof(ModelFile) + (MODEL_MEMORY_ALIGNMENT - 1)) & ~((uint64) MODEL_MEMORY_ALIGNMENT - 1))

/**
 * Size of the binary representation
 *
 * @param const ModelFile* file Model
 *
 * @return uint64
 */
CONSTEXPR FORCE_INLINE
uint64 model_data_size(const ModelFile* file) NO_EXCEPT
{
    return MODEL_BINARY_DATA_OFFSET + file->data_size;
}

/**
 * Writes the binary representation (out must be at least model_data_size() bytes)
 *
 * @param const ModelFile*  file    Model
 * @param byte*             out     Output
 *
 * @return uint64 Written bytes
 */
inline
uint64 model_to_data(const ModelFile* file, byte* out) NO_EXCEPT
{
    const ModelBinaryHeader header = { MODEL_BINARY_MAGIC, (uint32) (sizeof(ModelBinaryHeader) + sizeof(ModelFile)), file->data_size };
    memcpy(out, &header, sizeof(header));

    // Pointers -> offsets (+1 so that NULL stays NULL)
    ModelFile copy = *file;
    copy.data = NULL;

    #define MODEL_TO_OFFSET(name) \
        *((uintptr_t *) &copy.name) = file->name ? (uintptr_t) ((const byte *) file->name - file->data) + 1 : 0;

    MODEL_POINTER_LIST(MODEL_TO_OFFSET)
    #undef MODEL_TO_OFFSET

    memcpy(out + sizeof(header), &copy, sizeof(copy));
    memset(out + sizeof(header) + sizeof(copy), 0, MODEL_BINARY_DATA_OFFSET - sizeof(header) - sizeof(copy));
    memcpy(out + MODEL_BINARY_DATA_OFFSET, file->data, file->data_size);

    return model_data_size(file);
}

/**
 * Reads the header + the ModelFile of binary data and validates them (internal)
 */
static inline
bool model_binary_header_read(const byte* data, uint64 size, ModelFile* file, uint64* data_size) NO_EXCEPT
{
    if (size < MODEL_BINARY_DATA_OFFSET) {
        return false;
    }

    ModelBinaryHeader header;
    memcpy(&header, data, sizeof(header));

    if (header.magic != MODEL_BINARY_MAGIC
        || header.header_size != sizeof(ModelBinaryHeader) + sizeof(ModelFile)
        || MODEL_BINARY_DATA_OFFSET + header.data_size > size
    ) {
        return false;
    }

    memcpy(file, data + sizeof(header), sizeof(ModelFile));
    *data_size = header.data_size;

    return true;
}

/**
 * Offsets -> pointers into file->data (internal)
 */
static inline
bool model_binary_pointers_fix(ModelFile* file) NO_EXCEPT
{
    #define MODEL_TO_POINTER(name)                                       \
        {                                                                \
            const uintptr_t offset = *((uintptr_t *) &file->name);       \
            if (offset > file->data_size + 1) {                          \
                return false;                                            \
            }                                                            \
                                                                         \
            *((byte **) &file->name) = offset ? file->data + offset - 1 : NULL; \
        }

    MODEL_POINTER_LIST(MODEL_TO_POINTER)
    #undef MODEL_TO_POINTER

    return true;
}

/**
 * Size of the data block of binary data (= memory model_from_data() needs for file->data)
 *
 * @param const byte*   data    Binary data
 * @param uint64        size    Size of the binary data
 *
 * @return uint64 0 if the data is invalid
 */
inline
uint64 model_data_block_size(const byte* data, uint64 size) NO_EXCEPT
{
    ModelFile file;
    uint64 data_size;

    return model_binary_header_read(data, size, &file, &data_size) ? data_size : 0;
}

/**
 * Loads binary data into memory owned by the caller
 *
 * file->data MUST point to at least model_data_block_size() bytes (MODEL_MEMORY_ALIGNMENT aligned)
 *
 * @param const byte*   data    Binary data
 * @param uint64        size    Size of the binary data
 * @param ModelFile*    file    Model (output)
 *
 * @return bool false if the data is invalid
 */
inline
bool model_from_data(const byte* data, uint64 size, ModelFile* file) NO_EXCEPT
{
    byte* const memory = file->data;

    uint64 data_size;
    if (!memory || !model_binary_header_read(data, size, file, &data_size)) {
        file->data = memory;

        return false;
    }

    file->data = memory;
    file->data_size = data_size;
    memcpy(file->data, data + MODEL_BINARY_DATA_OFFSET, data_size);

    return model_binary_pointers_fix(file);
}

/**
 * Uses binary data in place (no copy)
 *
 * The ModelFile points into data afterwards -> data must stay alive and must be MODEL_MEMORY_ALIGNMENT aligned
 *
 * @param byte*         data    Binary data
 * @param uint64        size    Size of the binary data
 * @param ModelFile*    file    Model (output)
 *
 * @return bool false if the data is invalid
 */
inline
bool model_from_data_in_place(byte* data, uint64 size, ModelFile* file) NO_EXCEPT
{
    uint64 data_size;
    if (!model_binary_header_read(data, size, file, &data_size)) {
        return false;
    }

    file->data = data + MODEL_BINARY_DATA_OFFSET;
    file->data_size = data_size;

    return model_binary_pointers_fix(file);
}

#endif
