/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_OBJECT_OMX_H
#define COMS_OBJECT_OMX_H

#include "../stdlib/Stdlib.h"
#include "../animation/AnimationEaseType.h"

// OMX = OMS Model eXchange
// Line based text format (see Design-Documentation/File Formats/OMX Format.md)
// The same grammar is used for models, material libraries, animation libraries, hitbox libraries and particle libraries.
// A file is parsed in 2 passes:
//      1. omx_measure() counts all elements -> omx_memory_size() tells you how much memory to reserve
//      2. omx_parse() fills the reserved memory
// All arrays live in ONE memory block (OmxFile::data) which also makes the binary release format trivial (see OmxBinary.h)

#define OMX_VERSION 1
#define OMX_INVALID_ID 0xFFFFFFFF
#define OMX_INVALID_BONE 0xFFFF
#define OMX_MAX_BONE_INFLUENCES 4
#define OMX_MAX_CURVE_KEYS 4
#define OMX_MEMORY_ALIGNMENT 16

/////////////////////////////////////////////////////////////
// Vertex
/////////////////////////////////////////////////////////////

// The memory order of the attributes is always the order below (= canonical order)
// The text layout line MUST use the same order
enum OmxVertexAttribute : uint32 {
    OMX_VERTEX_POSITION = 1 << 0, // 3 x f32
    OMX_VERTEX_NORMAL   = 1 << 1, // 3 x f32
    OMX_VERTEX_TANGENT  = 1 << 2, // 4 x f32 (w = bitangent sign)
    OMX_VERTEX_UV0      = 1 << 3, // 2 x f32
    OMX_VERTEX_UV1      = 1 << 4, // 2 x f32 (e.g. lightmap, detail map)
    OMX_VERTEX_COLOR    = 1 << 5, // 1 x uint32 (rgba8, r = lowest byte)
    OMX_VERTEX_SKIN     = 1 << 6, // 4 x uint16 bone index + 4 x f32 weight
};

#define OMX_VERTEX_ATTRIBUTE_COUNT 7

// Byte size of every attribute in canonical order
static const uint32 OMX_VERTEX_ATTRIBUTE_SIZE[OMX_VERTEX_ATTRIBUTE_COUNT] = {
    12, 12, 16, 8, 8, 4, 24
};

FORCE_INLINE
uint32 omx_vertex_stride(uint32 layout) NO_EXCEPT
{
    uint32 stride = 0;
    for (int32 i = 0; i < OMX_VERTEX_ATTRIBUTE_COUNT; ++i) {
        if (layout & (1 << i)) {
            stride += OMX_VERTEX_ATTRIBUTE_SIZE[i];
        }
    }

    return stride;
}

// Byte offset of an attribute inside of a vertex, -1 if the layout doesn't contain the attribute
FORCE_INLINE
int32 omx_vertex_offset(uint32 layout, OmxVertexAttribute attribute) NO_EXCEPT
{
    if (!(layout & attribute)) {
        return -1;
    }

    int32 offset = 0;
    for (int32 i = 0; i < OMX_VERTEX_ATTRIBUTE_COUNT && (uint32) (1 << i) != (uint32) attribute; ++i) {
        if (layout & (1 << i)) {
            offset += OMX_VERTEX_ATTRIBUTE_SIZE[i];
        }
    }

    return offset;
}

struct OmxVertexSkin {
    uint16 bones[OMX_MAX_BONE_INFLUENCES];
    f32 weights[OMX_MAX_BONE_INFLUENCES];
};

/////////////////////////////////////////////////////////////
// Assets
/////////////////////////////////////////////////////////////

enum OmxAssetType : byte {
    OMX_ASSET_OMX,      // another omx file (material lib, animation lib, hitbox lib, ...)
    OMX_ASSET_TEXTURE,  // texture or texture atlas
    OMX_ASSET_AUDIO,
    OMX_ASSET_SHADER,
    OMX_ASSET_FONT,
    OMX_ASSET_DATA,     // anything else
};

// Dependency declaration. At runtime only the id matters, the path is for the build tools / hot reloading
struct OmxAsset {
    uint32 id;
    uint32 path_offset; // offset into OmxFile::strings
    OmxAssetType type;
};

/////////////////////////////////////////////////////////////
// Material (PBR metallic/roughness + common extensions)
/////////////////////////////////////////////////////////////

enum OmxAlphaMode : byte {
    OMX_ALPHA_OPAQUE,
    OMX_ALPHA_MASK,  // alpha test with alpha_cutoff
    OMX_ALPHA_BLEND,
    OMX_ALPHA_ADD,   // additive (fire, magic, ...)
};

enum OmxMaterialFlag : uint16 {
    OMX_MATERIAL_DOUBLE_SIDED   = 1 << 0,
    OMX_MATERIAL_UNLIT          = 1 << 1,
    OMX_MATERIAL_CAST_SHADOW    = 1 << 2,
    OMX_MATERIAL_RECEIVE_SHADOW = 1 << 3,
    OMX_MATERIAL_VERTEX_COLOR   = 1 << 4, // multiply base color with vertex color
    OMX_MATERIAL_THIN_WALLED    = 1 << 5, // transmission without refraction (leaves, paper, ...)
};

enum OmxTextureSlot : byte {
    OMX_TEXTURE_BASE,             // rgba
    OMX_TEXTURE_NORMAL,           // tangent space, scale = normal strength
    OMX_TEXTURE_ORM,              // r = occlusion, g = roughness, b = metallic, scale = occlusion strength
    OMX_TEXTURE_EMISSIVE,
    OMX_TEXTURE_TRANSMISSION,
    OMX_TEXTURE_SUBSURFACE,
    OMX_TEXTURE_CLEARCOAT,
    OMX_TEXTURE_CLEARCOAT_NORMAL,
    OMX_TEXTURE_SHEEN,
    OMX_TEXTURE_ANISOTROPY,
    OMX_TEXTURE_SPECULAR,
    OMX_TEXTURE_HEIGHT,           // parallax, scale = height scale
    OMX_TEXTURE_SLOT_SIZE,
};

struct OmxTextureRef {
    uint32 asset;      // texture/atlas asset id, OMX_INVALID_ID = unused
    uint32 region;     // texture atlas element, OMX_INVALID_ID = whole texture
    f32 scale;         // meaning depends on slot (see OmxTextureSlot)
    v2_f32 uv_offset;
    v2_f32 uv_scale;   // tiling
    byte uv_set;       // 0 = uv0, 1 = uv1
};

struct OmxMaterial {
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

    uint32 shader;      // optional custom shader asset id, OMX_INVALID_ID = default pbr

    OmxAlphaMode alpha_mode;
    uint16 flags;       // OmxMaterialFlag

    OmxTextureRef textures[OMX_TEXTURE_SLOT_SIZE];
};

/////////////////////////////////////////////////////////////
// Skeleton
/////////////////////////////////////////////////////////////

// Bind pose in local space (relative to the parent)
struct OmxBone {
    uint32 name;      // hash, animations target bones by name -> animations are shareable
    uint16 parent;    // index inside of the skeleton, OMX_INVALID_BONE = root
    uint16 flags;
    v3_f32 position;
    v3_f32 scale;
    quaternion rotation;
};

struct OmxSkeleton {
    uint32 id;
    uint32 name;
    uint32 bone_offset; // into OmxFile::bones
    uint32 bone_count;
    f32 height;         // reference height (e.g. hip height) used to scale root motion between different models
};

/////////////////////////////////////////////////////////////
// Animation
/////////////////////////////////////////////////////////////

enum OmxChannelPath : byte {
    OMX_CHANNEL_POSITION,
    OMX_CHANNEL_ROTATION,
    OMX_CHANNEL_SCALE,
};

enum OmxAnimationFlag : uint32 {
    OMX_ANIMATION_LOOP             = 1 << 0,
    OMX_ANIMATION_ADDITIVE         = 1 << 1, // applied on top of another animation (e.g. breathing, aiming)
    OMX_ANIMATION_ROOT_MOTION      = 1 << 2, // root translation moves the entity
    OMX_ANIMATION_FULL_TRANSLATION = 1 << 3, // also apply non-root translations (only correct on the authoring skeleton)
};

// Value from this key to the next key is interpolated with ease
struct OmxKeyframe {
    f32 time;
    f32 value[4]; // position/scale = xyz, rotation = quaternion xyzw
    AnimationEaseType ease;
};

struct OmxChannel {
    uint32 bone;        // bone name hash
    uint32 key_offset;  // into OmxFile::keys
    uint32 key_count;
    OmxChannelPath path;
};

enum OmxEventType : byte {
    OMX_EVENT_SOUND,    // ref = audio asset id
    OMX_EVENT_PARTICLE, // ref = emitter name hash (defined on the model -> the animation doesn't need to know the effect)
    OMX_EVENT_TAG,      // ref = tag name hash (e.g. footstep_l, hit_start, hit_end, combo_window)
};

struct OmxEvent {
    f32 time;
    uint32 ref;
    uint32 bone;        // bone name hash, 0 = model origin
    OmxEventType type;
};

struct OmxAnimation {
    uint32 id;
    uint32 name;
    uint32 rig;         // skeleton id the animation was authored for (optional, OMX_INVALID_ID)
    uint32 flags;       // OmxAnimationFlag
    f32 duration;
    f32 speed;          // default playback speed

    uint32 channel_offset; // into OmxFile::channels
    uint32 channel_count;

    uint32 event_offset;   // into OmxFile::events (sorted by time)
    uint32 event_count;
};

/////////////////////////////////////////////////////////////
// Hitbox
/////////////////////////////////////////////////////////////

enum OmxShapeType : byte {
    OMX_SHAPE_SPHERE,  // a = center, radius
    OMX_SHAPE_CAPSULE, // a = start, b = end, radius
    OMX_SHAPE_BOX,     // a = center, b = half extents, rotation
};

enum OmxHitLayer : byte {
    OMX_HIT_LAYER_WORLD,  // world collision
    OMX_HIT_LAYER_BODY,   // receives damage
    OMX_HIT_LAYER_HEAD,   // receives damage (usually with multiplier)
    OMX_HIT_LAYER_WEAPON, // deals damage (usually only active during hit_start - hit_end tag events)
    OMX_HIT_LAYER_TRIGGER,
};

// Shapes are defined in bone space and follow the bone
struct OmxShape {
    uint32 bone;        // bone name hash, 0 = model origin
    v3_f32 a;
    v3_f32 b;
    f32 radius;
    f32 multiplier;     // e.g. damage multiplier
    quaternion rotation;
    OmxShapeType type;
    OmxHitLayer layer;
};

struct OmxHitbox {
    uint32 id;
    uint32 name;
    uint32 shape_offset; // into OmxFile::shapes
    uint32 shape_count;
};

/////////////////////////////////////////////////////////////
// Particles
/////////////////////////////////////////////////////////////

enum OmxEmitterShape : byte {
    OMX_EMITTER_POINT,
    OMX_EMITTER_SPHERE, // shape_param.x = radius
    OMX_EMITTER_CONE,   // shape_param.x = angle (deg), shape_param.y = base radius
    OMX_EMITTER_BOX,    // shape_param = half extents
};

enum OmxParticleFlag : uint16 {
    OMX_PARTICLE_LOOP        = 1 << 0,
    OMX_PARTICLE_WORLD_SPACE = 1 << 1, // particles stay in the world when the emitter moves (smoke trails)
    OMX_PARTICLE_STRETCH     = 1 << 2, // velocity aligned billboards (sparks, rain)
    OMX_PARTICLE_COLLIDE     = 1 << 3,
};

struct OmxColorKey {
    f32 time; // 0-1 normalized life time
    v4_f32 color;
};

struct OmxFloatKey {
    f32 time;
    f32 value;
};

struct OmxParticleSystem {
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
    OmxColorKey color[OMX_MAX_CURVE_KEYS];
    OmxFloatKey size[OMX_MAX_CURVE_KEYS];

    OmxEmitterShape shape;
    uint16 flags;         // OmxParticleFlag
};

// Runtime particle (used to calculate the per instance memory)
struct OmxParticle {
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
struct OmxMesh {
    uint32 name;
    uint32 material;     // material id, OMX_INVALID_ID = default material
    uint32 index_offset; // into OmxFile::indices
    uint32 index_count;
};

// Maps a gameplay animation slot (e.g. "run") to an animation id
// -> different models can use different animations for the same slot
struct OmxAnimationSlot {
    uint32 name;
    uint32 animation;
};

// Attachment point (weapons, hats, ...)
struct OmxSocket {
    uint32 name;
    uint32 bone;
    v3_f32 position;
    quaternion rotation;
};

enum OmxEmitterFlag : uint16 {
    OMX_EMITTER_AUTO_START = 1 << 0, // e.g. torch fire, otherwise started by animation events
};

// Particle system instance attached to the model
struct OmxEmitter {
    uint32 name;
    uint32 particle_system;
    uint32 bone;
    v3_f32 offset;
    uint16 flags;
};

enum OmxStateType : byte {
    OMX_STATE_BYTE,
    OMX_STATE_INT32,
    OMX_STATE_UINT32,
    OMX_STATE_F32,
    OMX_STATE_V2,
    OMX_STATE_V3,
    OMX_STATE_V4,
    OMX_STATE_MAT4,
    OMX_STATE_TYPE_SIZE,
};

static const uint32 OMX_STATE_TYPE_BYTES[OMX_STATE_TYPE_SIZE] = { 1, 4, 4, 4, 8, 12, 16, 64 };
static const uint32 OMX_STATE_TYPE_ALIGNMENT[OMX_STATE_TYPE_SIZE] = { 1, 4, 4, 4, 4, 4, 16, 16 };

// Custom per instance memory the model needs (e.g. door open state, health, cooldowns)
struct OmxStateField {
    uint32 name;
    uint32 offset;  // inside of the state block
    uint32 count;
    OmxStateType type;
};

/////////////////////////////////////////////////////////////
// File
/////////////////////////////////////////////////////////////

struct OmxFile {
    byte* data; // memory owner that subdivides into the pointers below
    uint64 data_size;

    uint32 version;
    uint32 name; // model name hash, 0 = file is only a library

    // Libraries (everything in here can be referenced by other files through its id)
    uint32 asset_count;
    OmxAsset* assets;

    uint32 string_size;
    char* strings;

    uint32 material_count;
    OmxMaterial* materials;

    uint32 skeleton_count;
    OmxSkeleton* skeletons;

    uint32 bone_count;
    OmxBone* bones;

    uint32 animation_count;
    OmxAnimation* animations;

    uint32 channel_count;
    OmxChannel* channels;

    uint32 key_count;
    OmxKeyframe* keys;

    uint32 event_count;
    OmxEvent* events;

    uint32 hitbox_count;
    OmxHitbox* hitboxes;

    uint32 shape_count;
    OmxShape* shapes;

    uint32 particle_system_count;
    OmxParticleSystem* particle_systems;

    // Model
    uint32 vertex_layout; // OmxVertexAttribute
    uint32 vertex_stride;
    uint32 vertex_count;
    byte* vertices;

    uint32 index_count;
    uint32* indices;

    uint32 mesh_count;
    OmxMesh* meshes;

    uint32 slot_count;
    OmxAnimationSlot* slots;

    uint32 socket_count;
    OmxSocket* sockets;

    uint32 emitter_count;
    OmxEmitter* emitters;

    uint32 state_field_count;
    OmxStateField* state_fields;

    uint32 state_size;     // bytes of all state fields
    uint32 anim_layers;    // max. simultaneously blended animations per instance
    uint32 skeleton;       // skeleton id, OMX_INVALID_ID = static model
    uint32 hitbox;         // hitbox id, OMX_INVALID_ID = none

    v3_f32 bounds_min;
    v3_f32 bounds_max;
};

/////////////////////////////////////////////////////////////
// Hash
/////////////////////////////////////////////////////////////

// FNV-1a 32 bit. 0 is reserved for "no name"
FORCE_INLINE
uint32 omx_hash(const char* str, int32 length) NO_EXCEPT
{
    uint32 hash = 2166136261u;
    for (int32 i = 0; i < length; ++i) {
        hash ^= (byte) str[i];
        hash *= 16777619u;
    }

    return hash ? hash : 1;
}

FORCE_INLINE
uint32 omx_hash(const char* str) NO_EXCEPT
{
    int32 length = 0;
    while (str[length] != '\0') {
        ++length;
    }

    return omx_hash(str, length);
}

/////////////////////////////////////////////////////////////
// Lookup helpers
/////////////////////////////////////////////////////////////

FORCE_INLINE
const OmxBone* omx_skeleton_bones(const OmxFile* file, const OmxSkeleton* skeleton) NO_EXCEPT
{
    return file->bones + skeleton->bone_offset;
}

FORCE_INLINE
const OmxChannel* omx_animation_channels(const OmxFile* file, const OmxAnimation* anim) NO_EXCEPT
{
    return file->channels + anim->channel_offset;
}

FORCE_INLINE
const OmxEvent* omx_animation_events(const OmxFile* file, const OmxAnimation* anim) NO_EXCEPT
{
    return file->events + anim->event_offset;
}

FORCE_INLINE
const OmxShape* omx_hitbox_shapes(const OmxFile* file, const OmxHitbox* hitbox) NO_EXCEPT
{
    return file->shapes + hitbox->shape_offset;
}

FORCE_INLINE
const char* omx_asset_path(const OmxFile* file, const OmxAsset* asset) NO_EXCEPT
{
    return file->strings + asset->path_offset;
}

// Bone index inside of the skeleton, OMX_INVALID_BONE if not found
inline
uint16 omx_skeleton_find_bone(const OmxFile* file, const OmxSkeleton* skeleton, uint32 name) NO_EXCEPT
{
    const OmxBone* bones = omx_skeleton_bones(file, skeleton);
    for (uint32 i = 0; i < skeleton->bone_count; ++i) {
        if (bones[i].name == name) {
            return (uint16) i;
        }
    }

    return OMX_INVALID_BONE;
}

inline
const OmxStateField* omx_find_state_field(const OmxFile* file, uint32 name) NO_EXCEPT
{
    for (uint32 i = 0; i < file->state_field_count; ++i) {
        if (file->state_fields[i].name == name) {
            return &file->state_fields[i];
        }
    }

    return NULL;
}

#endif
