/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_OBJECT_OMX_PARSER_H
#define COMS_OBJECT_OMX_PARSER_H

#include "../stdlib/Stdlib.h"
#include "Omx.h"

// Usage:
//      OmxFile file;
//      OmxError error;
//      if (!omx_measure(text, &file, &error)) { ... }
//      byte* memory = <reserve omx_memory_size(&file) bytes, aligned to OMX_MEMORY_ALIGNMENT>
//      if (!omx_parse(text, &file, memory, &error)) { ... }
//
// text must be null terminated and MUST be identical for both calls

struct OmxError {
    int32 line;
    const char* message;
};

enum OmxBlock : byte {
    OMX_BLOCK_NONE,
    OMX_BLOCK_MATERIAL,
    OMX_BLOCK_SKELETON,
    OMX_BLOCK_ANIMATION,
    OMX_BLOCK_HITBOX,
    OMX_BLOCK_PARTICLE,
};

struct OmxToken {
    const char* str;
    int32 length;
};

struct OmxParser {
    const char* pos; // current position inside of the line
    const char* eol; // end of the current line (exclusive)
    int32 line;

    OmxFile* file;
    OmxError* error;

    // false = measure pass (only counting), true = fill pass
    bool fill;

    OmxBlock block;
    int32 mesh;          // current mesh, -1 = none
    int32 channel;       // current animation channel (absolute index), -1 = none
    AnimationEaseType channel_ease;
};

/////////////////////////////////////////////////////////////
// Tokenizer
/////////////////////////////////////////////////////////////

// Packs a keyword (max 8 characters) into an integer -> keywords can be used in switch statements
constexpr
uint64 omx_key(const char* str) NO_EXCEPT
{
    uint64 key = 0;
    for (int32 i = 0; i < 8 && str[i] != '\0'; ++i) {
        key |= ((uint64) (byte) str[i]) << (i * 8);
    }

    return key;
}

static inline
uint64 omx_token_key(const OmxToken* tok) NO_EXCEPT
{
    if (tok->length > 8) {
        return 0;
    }

    uint64 key = 0;
    for (int32 i = 0; i < tok->length; ++i) {
        key |= ((uint64) (byte) tok->str[i]) << (i * 8);
    }

    return key;
}

static inline
bool omx_token_equals(const OmxToken* tok, const char* str) NO_EXCEPT
{
    int32 i = 0;
    for (; i < tok->length; ++i) {
        if (str[i] != tok->str[i]) {
            return false;
        }
    }

    return str[i] == '\0';
}

static inline
bool omx_error(OmxParser* p, const char* message) NO_EXCEPT
{
    if (p->error) {
        p->error->line = p->line;
        p->error->message = message;
    }

    return false;
}

static inline
bool omx_next_token(OmxParser* p, OmxToken* tok) NO_EXCEPT
{
    const char* pos = p->pos;
    while (pos < p->eol && (*pos == ' ' || *pos == '\t')) {
        ++pos;
    }

    // '#' starts a comment (also at the end of a line)
    if (pos >= p->eol || *pos == '#') {
        p->pos = p->eol;

        return false;
    }

    tok->str = pos;
    while (pos < p->eol && *pos != ' ' && *pos != '\t') {
        ++pos;
    }

    tok->length = (int32) (pos - tok->str);
    p->pos = pos;

    return true;
}

static inline
bool omx_has_token(OmxParser* p) NO_EXCEPT
{
    const char* pos = p->pos;
    OmxToken tok;
    const bool has_token = omx_next_token(p, &tok);
    p->pos = pos;

    return has_token;
}

static inline
int32 omx_remaining_tokens(OmxParser* p) NO_EXCEPT
{
    const char* pos = p->pos;
    int32 count = 0;

    OmxToken tok;
    while (omx_next_token(p, &tok)) {
        ++count;
    }

    p->pos = pos;

    return count;
}

// We don't use str_to_float since we need exponent support (e.g. 1e-05 from exporters) and more than 6 decimals
static
bool omx_token_to_f32(const OmxToken* tok, f32* out) NO_EXCEPT
{
    const char* str = tok->str;
    const char* end = str + tok->length;

    f64 sign = 1.0;
    if (str < end && (*str == '-' || *str == '+')) {
        sign = *str == '-' ? -1.0 : 1.0;
        ++str;
    }

    f64 value = 0.0;
    int32 digits = 0;
    int32 decimals = 0;

    while (str < end && *str >= '0' && *str <= '9') {
        value = value * 10.0 + (*str - '0');
        ++digits;
        ++str;
    }

    if (str < end && *str == '.') {
        ++str;
        while (str < end && *str >= '0' && *str <= '9') {
            value = value * 10.0 + (*str - '0');
            ++digits;
            ++decimals;
            ++str;
        }
    }

    if (digits == 0) {
        return false;
    }

    int32 exponent = -decimals;
    if (str < end && (*str == 'e' || *str == 'E')) {
        ++str;

        int32 exp_sign = 1;
        if (str < end && (*str == '-' || *str == '+')) {
            exp_sign = *str == '-' ? -1 : 1;
            ++str;
        }

        int32 exp = 0;
        int32 exp_digits = 0;
        while (str < end && *str >= '0' && *str <= '9' && exp < 1000) {
            exp = exp * 10 + (*str - '0');
            ++exp_digits;
            ++str;
        }

        if (exp_digits == 0) {
            return false;
        }

        exponent += exp_sign * exp;
    }

    if (str != end) {
        return false;
    }

    f64 power = 1.0;
    const int32 abs_exponent = exponent < 0 ? -exponent : exponent;
    for (int32 i = 0; i < abs_exponent && i < 400; ++i) {
        power *= 10.0;
    }

    value = exponent < 0 ? value / power : value * power;
    *out = (f32) (sign * value);

    return true;
}

static
bool omx_token_to_int64(const OmxToken* tok, int64* out) NO_EXCEPT
{
    const char* str = tok->str;
    const char* end = str + tok->length;

    int64 sign = 1;
    if (str < end && (*str == '-' || *str == '+')) {
        sign = *str == '-' ? -1 : 1;
        ++str;
    }

    if (str == end) {
        return false;
    }

    int64 value = 0;
    while (str < end) {
        if (*str < '0' || *str > '9' || value > 0xFFFFFFFFFFLL) {
            return false;
        }

        value = value * 10 + (*str - '0');
        ++str;
    }

    *out = sign * value;

    return true;
}

/////////////////////////////////////////////////////////////
// Typed readers (they set the error on failure)
/////////////////////////////////////////////////////////////

static
bool omx_read_f32(OmxParser* p, f32* out) NO_EXCEPT
{
    OmxToken tok;
    if (!omx_next_token(p, &tok) || !omx_token_to_f32(&tok, out)) {
        return omx_error(p, "Expected number");
    }

    return true;
}

static
bool omx_read_f32_array(OmxParser* p, f32* out, int32 count) NO_EXCEPT
{
    for (int32 i = 0; i < count; ++i) {
        if (!omx_read_f32(p, &out[i])) {
            return false;
        }
    }

    return true;
}

// Optional value at the end of a line
static
bool omx_read_f32_optional(OmxParser* p, f32* out, f32 default_value) NO_EXCEPT
{
    if (!omx_has_token(p)) {
        *out = default_value;

        return true;
    }

    return omx_read_f32(p, out);
}

// '-' = OMX_INVALID_ID
static
bool omx_read_id(OmxParser* p, uint32* out) NO_EXCEPT
{
    OmxToken tok;
    if (!omx_next_token(p, &tok)) {
        return omx_error(p, "Expected id");
    }

    if (tok.length == 1 && *tok.str == '-') {
        *out = OMX_INVALID_ID;

        return true;
    }

    int64 value;
    if (!omx_token_to_int64(&tok, &value) || value < 0 || value >= OMX_INVALID_ID) {
        return omx_error(p, "Expected id (uint32)");
    }

    *out = (uint32) value;

    return true;
}

static
bool omx_read_int(OmxParser* p, int64* out, int64 min, int64 max) NO_EXCEPT
{
    OmxToken tok;
    if (!omx_next_token(p, &tok) || !omx_token_to_int64(&tok, out)) {
        return omx_error(p, "Expected integer");
    }

    if (*out < min || *out > max) {
        return omx_error(p, "Integer out of range");
    }

    return true;
}

// Names are stored as hash, '-' = 0 (= no name)
static
bool omx_read_name(OmxParser* p, uint32* out) NO_EXCEPT
{
    OmxToken tok;
    if (!omx_next_token(p, &tok)) {
        return omx_error(p, "Expected name");
    }

    *out = (tok.length == 1 && *tok.str == '-') ? 0 : omx_hash(tok.str, tok.length);

    return true;
}

static
bool omx_read_quaternion(OmxParser* p, quaternion* out) NO_EXCEPT
{
    if (!omx_read_f32_array(p, out->vec, 4)) {
        return false;
    }

    const f32 length_sq = out->x * out->x + out->y * out->y + out->z * out->z + out->w * out->w;
    if (length_sq < 0.000001f) {
        return omx_error(p, "Invalid quaternion");
    }

    const f32 inv_length = 1.0f / sqrtf(length_sq);
    out->x *= inv_length;
    out->y *= inv_length;
    out->z *= inv_length;
    out->w *= inv_length;

    return true;
}

struct OmxEaseName {
    const char* name;
    AnimationEaseType type;
};

static const OmxEaseName OMX_EASE_NAMES[] = {
    { "linear", ANIMATION_LINEAR },
    { "step", ANIMATION_EASE_DISCRETE },
    { "in_sine", ANIMATION_EASE_IN_SINE },
    { "out_sine", ANIMATION_EASE_OUT_SINE },
    { "in_out_sine", ANIMATION_EASE_IN_OUT_SINE },
    { "in_quad", ANIMATION_EASE_IN_QUAD },
    { "out_quad", ANIMATION_EASE_OUT_QUAD },
    { "in_out_quad", ANIMATION_EASE_IN_OUT_QUAD },
    { "in_cubic", ANIMATION_EASE_IN_CUBIC },
    { "out_cubic", ANIMATION_EASE_OUT_CUBIC },
    { "in_out_cubic", ANIMATION_EASE_IN_OUT_CUBIC },
    { "in_quart", ANIMATION_EASE_IN_QUART },
    { "out_quart", ANIMATION_EASE_OUT_QUART },
    { "in_out_quart", ANIMATION_EASE_IN_OUT_QUART },
    { "in_quint", ANIMATION_EASE_IN_QUINT },
    { "out_quint", ANIMATION_EASE_OUT_QUINT },
    { "in_out_quint", ANIMATION_EASE_IN_OUT_QUINT },
    { "in_expo", ANIMATION_EASE_IN_EXPO },
    { "out_expo", ANIMATION_EASE_OUT_EXPO },
    { "in_out_expo", ANIMATION_EASE_IN_OUT_EXPO },
    { "in_circ", ANIMATION_EASE_IN_CIRC },
    { "out_circ", ANIMATION_EASE_OUT_CIRC },
    { "in_out_circ", ANIMATION_EASE_IN_OUT_CIRC },
    { "in_back", ANIMATION_EASE_IN_BACK },
    { "out_back", ANIMATION_EASE_OUT_BACK },
    { "in_out_back", ANIMATION_EASE_IN_OUT_BACK },
    { "in_elastic", ANIMATION_EASE_IN_ELASTIC },
    { "out_elastic", ANIMATION_EASE_OUT_ELASTIC },
    { "in_out_elastic", ANIMATION_EASE_IN_OUT_ELASTIC },
    { "in_bounce", ANIMATION_EASE_IN_BOUNCE },
    { "out_bounce", ANIMATION_EASE_OUT_BOUNCE },
    { "in_out_bounce", ANIMATION_EASE_IN_OUT_BOUNCE },
};

static
bool omx_read_ease(OmxParser* p, AnimationEaseType* out) NO_EXCEPT
{
    OmxToken tok;
    if (!omx_next_token(p, &tok)) {
        return omx_error(p, "Expected ease type");
    }

    for (int32 i = 0; i < ARRAY_COUNT(OMX_EASE_NAMES); ++i) {
        if (omx_token_equals(&tok, OMX_EASE_NAMES[i].name)) {
            *out = OMX_EASE_NAMES[i].type;

            return true;
        }
    }

    return omx_error(p, "Unknown ease type");
}

/////////////////////////////////////////////////////////////
// Memory
/////////////////////////////////////////////////////////////

FORCE_INLINE
uint64 omx_align(uint64 size) NO_EXCEPT
{
    return (size + (OMX_MEMORY_ALIGNMENT - 1)) & ~((uint64) OMX_MEMORY_ALIGNMENT - 1);
}

// Memory required for the parsed file (call after omx_measure)
uint64 omx_memory_size(const OmxFile* file) NO_EXCEPT
{
    return omx_align(sizeof(OmxAsset) * file->asset_count)
        + omx_align(file->string_size)
        + omx_align(sizeof(OmxMaterial) * file->material_count)
        + omx_align(sizeof(OmxSkeleton) * file->skeleton_count)
        + omx_align(sizeof(OmxBone) * file->bone_count)
        + omx_align(sizeof(OmxAnimation) * file->animation_count)
        + omx_align(sizeof(OmxChannel) * file->channel_count)
        + omx_align(sizeof(OmxKeyframe) * file->key_count)
        + omx_align(sizeof(OmxEvent) * file->event_count)
        + omx_align(sizeof(OmxHitbox) * file->hitbox_count)
        + omx_align(sizeof(OmxShape) * file->shape_count)
        + omx_align(sizeof(OmxParticleSystem) * file->particle_system_count)
        + omx_align((uint64) file->vertex_stride * file->vertex_count)
        + omx_align(sizeof(uint32) * file->index_count)
        + omx_align(sizeof(OmxMesh) * file->mesh_count)
        + omx_align(sizeof(OmxAnimationSlot) * file->slot_count)
        + omx_align(sizeof(OmxSocket) * file->socket_count)
        + omx_align(sizeof(OmxEmitter) * file->emitter_count)
        + omx_align(sizeof(OmxStateField) * file->state_field_count);
}

static
byte* omx_carve(byte** pos, uint64 size) NO_EXCEPT
{
    byte* ptr = *pos;
    *pos += omx_align(size);

    return size ? ptr : NULL;
}

// Subdivides the memory block based on the measured counts
static
void omx_assign_memory(OmxFile* file, byte* memory) NO_EXCEPT
{
    byte* pos = memory;

    file->data = memory;
    file->data_size = omx_memory_size(file);

    file->assets = (OmxAsset *) omx_carve(&pos, sizeof(OmxAsset) * file->asset_count);
    file->strings = (char *) omx_carve(&pos, file->string_size);
    file->materials = (OmxMaterial *) omx_carve(&pos, sizeof(OmxMaterial) * file->material_count);
    file->skeletons = (OmxSkeleton *) omx_carve(&pos, sizeof(OmxSkeleton) * file->skeleton_count);
    file->bones = (OmxBone *) omx_carve(&pos, sizeof(OmxBone) * file->bone_count);
    file->animations = (OmxAnimation *) omx_carve(&pos, sizeof(OmxAnimation) * file->animation_count);
    file->channels = (OmxChannel *) omx_carve(&pos, sizeof(OmxChannel) * file->channel_count);
    file->keys = (OmxKeyframe *) omx_carve(&pos, sizeof(OmxKeyframe) * file->key_count);
    file->events = (OmxEvent *) omx_carve(&pos, sizeof(OmxEvent) * file->event_count);
    file->hitboxes = (OmxHitbox *) omx_carve(&pos, sizeof(OmxHitbox) * file->hitbox_count);
    file->shapes = (OmxShape *) omx_carve(&pos, sizeof(OmxShape) * file->shape_count);
    file->particle_systems = (OmxParticleSystem *) omx_carve(&pos, sizeof(OmxParticleSystem) * file->particle_system_count);
    file->vertices = omx_carve(&pos, (uint64) file->vertex_stride * file->vertex_count);
    file->indices = (uint32 *) omx_carve(&pos, sizeof(uint32) * file->index_count);
    file->meshes = (OmxMesh *) omx_carve(&pos, sizeof(OmxMesh) * file->mesh_count);
    file->slots = (OmxAnimationSlot *) omx_carve(&pos, sizeof(OmxAnimationSlot) * file->slot_count);
    file->sockets = (OmxSocket *) omx_carve(&pos, sizeof(OmxSocket) * file->socket_count);
    file->emitters = (OmxEmitter *) omx_carve(&pos, sizeof(OmxEmitter) * file->emitter_count);
    file->state_fields = (OmxStateField *) omx_carve(&pos, sizeof(OmxStateField) * file->state_field_count);
}

static
void omx_file_init(OmxFile* file) NO_EXCEPT
{
    memset(file, 0, sizeof(OmxFile));
    file->version = OMX_VERSION;
    file->skeleton = OMX_INVALID_ID;
    file->hitbox = OMX_INVALID_ID;
}

/////////////////////////////////////////////////////////////
// Root keywords
/////////////////////////////////////////////////////////////

static
bool omx_line_asset(OmxParser* p) NO_EXCEPT
{
    OmxFile* file = p->file;

    uint32 id;
    if (!omx_read_id(p, &id)) {
        return false;
    }

    OmxToken type_tok;
    OmxToken path_tok;
    if (!omx_next_token(p, &type_tok) || !omx_next_token(p, &path_tok)) {
        return omx_error(p, "Expected: asset <id> <type> <path>");
    }

    if (!p->fill) {
        ++file->asset_count;
        file->string_size += path_tok.length + 1;

        return true;
    }

    OmxAssetType type;
    switch (omx_token_key(&type_tok)) {
        case omx_key("omx"): type = OMX_ASSET_OMX; break;
        case omx_key("tex"): type = OMX_ASSET_TEXTURE; break;
        case omx_key("audio"): type = OMX_ASSET_AUDIO; break;
        case omx_key("shader"): type = OMX_ASSET_SHADER; break;
        case omx_key("font"): type = OMX_ASSET_FONT; break;
        case omx_key("data"): type = OMX_ASSET_DATA; break;
        default: return omx_error(p, "Unknown asset type");
    }

    OmxAsset* asset = &file->assets[file->asset_count++];
    asset->id = id;
    asset->type = type;
    asset->path_offset = file->string_size;

    memcpy(file->strings + file->string_size, path_tok.str, path_tok.length);
    file->strings[file->string_size + path_tok.length] = '\0';
    file->string_size += path_tok.length + 1;

    return true;
}

static
bool omx_line_layout(OmxParser* p) NO_EXCEPT
{
    OmxFile* file = p->file;
    if (file->vertex_count) {
        return omx_error(p, "layout must be defined before the first vertex");
    }

    uint32 layout = 0;
    uint32 last = 0;

    OmxToken tok;
    while (omx_next_token(p, &tok)) {
        uint32 attribute;
        switch (omx_token_key(&tok)) {
            case omx_key("pos"): attribute = OMX_VERTEX_POSITION; break;
            case omx_key("nrm"): attribute = OMX_VERTEX_NORMAL; break;
            case omx_key("tan"): attribute = OMX_VERTEX_TANGENT; break;
            case omx_key("uv0"): attribute = OMX_VERTEX_UV0; break;
            case omx_key("uv1"): attribute = OMX_VERTEX_UV1; break;
            case omx_key("col"): attribute = OMX_VERTEX_COLOR; break;
            case omx_key("skin"): attribute = OMX_VERTEX_SKIN; break;
            default: return omx_error(p, "Unknown vertex attribute");
        }

        // Enforcing the canonical order makes the v lines unambiguous and memcpy friendly
        if (attribute <= last) {
            return omx_error(p, "Vertex attributes must be in the order: pos nrm tan uv0 uv1 col skin");
        }

        layout |= attribute;
        last = attribute;
    }

    if (!(layout & OMX_VERTEX_POSITION)) {
        return omx_error(p, "Vertex layout requires pos");
    }

    file->vertex_layout = layout;
    file->vertex_stride = omx_vertex_stride(layout);

    return true;
}

static
bool omx_read_vertex_attribute(OmxParser* p, byte* dst, int32 floats) NO_EXCEPT
{
    f32 values[4];
    if (!omx_read_f32_array(p, values, floats)) {
        return false;
    }

    memcpy(dst, values, floats * sizeof(f32));

    return true;
}

static
bool omx_line_vertex(OmxParser* p) NO_EXCEPT
{
    OmxFile* file = p->file;
    if (!file->vertex_layout) {
        return omx_error(p, "layout must be defined before the first vertex");
    }

    if (!p->fill) {
        ++file->vertex_count;
        p->pos = p->eol;

        return true;
    }

    const uint32 layout = file->vertex_layout;
    byte* dst = file->vertices + (uint64) file->vertex_count * file->vertex_stride;

    // Position
    v3_f32 position;
    if (!omx_read_f32_array(p, position.vec, 3)) {
        return false;
    }

    memcpy(dst, position.vec, sizeof(f32) * 3);
    dst += 12;

    if (file->vertex_count == 0) {
        file->bounds_min = position;
        file->bounds_max = position;
    } else {
        for (int32 i = 0; i < 3; ++i) {
            file->bounds_min.vec[i] = position.vec[i] < file->bounds_min.vec[i] ? position.vec[i] : file->bounds_min.vec[i];
            file->bounds_max.vec[i] = position.vec[i] > file->bounds_max.vec[i] ? position.vec[i] : file->bounds_max.vec[i];
        }
    }

    if (layout & OMX_VERTEX_NORMAL) {
        if (!omx_read_vertex_attribute(p, dst, 3)) {
            return false;
        }

        dst += 12;
    }

    if (layout & OMX_VERTEX_TANGENT) {
        if (!omx_read_vertex_attribute(p, dst, 4)) {
            return false;
        }

        dst += 16;
    }

    if (layout & OMX_VERTEX_UV0) {
        if (!omx_read_vertex_attribute(p, dst, 2)) {
            return false;
        }

        dst += 8;
    }

    if (layout & OMX_VERTEX_UV1) {
        if (!omx_read_vertex_attribute(p, dst, 2)) {
            return false;
        }

        dst += 8;
    }

    if (layout & OMX_VERTEX_COLOR) {
        f32 color[4];
        if (!omx_read_f32_array(p, color, 4)) {
            return false;
        }

        uint32 rgba = 0;
        for (int32 i = 0; i < 4; ++i) {
            const f32 c = color[i] < 0.0f ? 0.0f : (color[i] > 1.0f ? 1.0f : color[i]);
            rgba |= ((uint32) (c * 255.0f + 0.5f)) << (i * 8);
        }

        memcpy(dst, &rgba, sizeof(rgba));
        dst += 4;
    }

    if (layout & OMX_VERTEX_SKIN) {
        // bone weight pairs until the end of the line
        // If more than 4 influences are defined we only keep the 4 strongest ones
        OmxVertexSkin skin = {};
        int32 influences = 0;

        while (omx_has_token(p)) {
            int64 bone;
            f32 weight;
            if (!omx_read_int(p, &bone, 0, OMX_INVALID_BONE - 1) || !omx_read_f32(p, &weight)) {
                return false;
            }

            if (weight <= 0.0f) {
                continue;
            }

            // Insertion sort (descending weight)
            int32 i = influences < OMX_MAX_BONE_INFLUENCES ? influences++ : OMX_MAX_BONE_INFLUENCES;
            while (i > 0 && skin.weights[i - 1] < weight) {
                if (i < OMX_MAX_BONE_INFLUENCES) {
                    skin.weights[i] = skin.weights[i - 1];
                    skin.bones[i] = skin.bones[i - 1];
                }

                --i;
            }

            if (i < OMX_MAX_BONE_INFLUENCES) {
                skin.weights[i] = weight;
                skin.bones[i] = (uint16) bone;
            }
        }

        if (influences == 0) {
            return omx_error(p, "Skinned vertex requires at least one bone weight pair");
        }

        // Normalize: 0.7 + 0.3 = 1.0 but also e.g. 7 + 3 is allowed
        f32 sum = 0.0f;
        for (int32 i = 0; i < OMX_MAX_BONE_INFLUENCES; ++i) {
            sum += skin.weights[i];
        }

        for (int32 i = 0; i < OMX_MAX_BONE_INFLUENCES; ++i) {
            skin.weights[i] /= sum;
        }

        memcpy(dst, &skin, sizeof(skin));
        dst += sizeof(skin);
    }

    ++file->vertex_count;

    return true;
}

static
void omx_begin_mesh(OmxParser* p, uint32 name, uint32 material) NO_EXCEPT
{
    OmxFile* file = p->file;
    p->mesh = (int32) file->mesh_count++;

    if (p->fill) {
        OmxMesh* mesh = &file->meshes[p->mesh];
        mesh->name = name;
        mesh->material = material;
        mesh->index_offset = file->index_count;
        mesh->index_count = 0;
    }
}

static
bool omx_line_mesh(OmxParser* p) NO_EXCEPT
{
    uint32 name;
    uint32 material;
    if (!omx_read_name(p, &name) || !omx_read_id(p, &material)) {
        return false;
    }

    omx_begin_mesh(p, name, material);

    return true;
}

static
bool omx_read_index(OmxParser* p, uint32* out) NO_EXCEPT
{
    int64 index;
    if (!omx_read_int(p, &index, -(int64) OMX_INVALID_ID, OMX_INVALID_ID)) {
        return false;
    }

    // Negative = relative to the last defined vertex (-1 = last vertex)
    if (index < 0) {
        index += p->file->vertex_count;
    }

    if (index < 0 || index >= p->file->vertex_count) {
        return omx_error(p, "Vertex index out of range (vertices must be defined before the face)");
    }

    *out = (uint32) index;

    return true;
}

static
bool omx_line_face(OmxParser* p) NO_EXCEPT
{
    OmxFile* file = p->file;

    // Faces without a mesh line end up in a default mesh
    if (p->mesh < 0) {
        omx_begin_mesh(p, 0, OMX_INVALID_ID);
    }

    const int32 corners = omx_remaining_tokens(p);
    if (corners < 3) {
        return omx_error(p, "Face requires at least 3 indices");
    }

    const uint32 indices = (uint32) (corners - 2) * 3;

    if (!p->fill) {
        file->index_count += indices;
        p->pos = p->eol;

        return true;
    }

    // Polygons are triangulated as triangle fan (convex polygons only)
    uint32 first;
    uint32 previous;
    if (!omx_read_index(p, &first) || !omx_read_index(p, &previous)) {
        return false;
    }

    for (int32 i = 2; i < corners; ++i) {
        uint32 current;
        if (!omx_read_index(p, &current)) {
            return false;
        }

        file->indices[file->index_count++] = first;
        file->indices[file->index_count++] = previous;
        file->indices[file->index_count++] = current;

        previous = current;
    }

    file->meshes[p->mesh].index_count += indices;

    return true;
}

static
bool omx_line_socket(OmxParser* p) NO_EXCEPT
{
    OmxFile* file = p->file;
    if (!p->fill) {
        ++file->socket_count;
        p->pos = p->eol;

        return true;
    }

    OmxSocket* socket = &file->sockets[file->socket_count++];
    if (!omx_read_name(p, &socket->name)
        || !omx_read_name(p, &socket->bone)
        || !omx_read_f32_array(p, socket->position.vec, 3)
    ) {
        return false;
    }

    socket->rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (omx_has_token(p)) {
        return omx_read_quaternion(p, &socket->rotation);
    }

    return true;
}

static
bool omx_line_emitter(OmxParser* p) NO_EXCEPT
{
    OmxFile* file = p->file;
    if (!p->fill) {
        ++file->emitter_count;
        p->pos = p->eol;

        return true;
    }

    OmxEmitter* emitter = &file->emitters[file->emitter_count++];
    if (!omx_read_name(p, &emitter->name)
        || !omx_read_id(p, &emitter->particle_system)
        || !omx_read_name(p, &emitter->bone)
        || !omx_read_f32_array(p, emitter->offset.vec, 3)
    ) {
        return false;
    }

    emitter->flags = 0;

    OmxToken tok;
    while (omx_next_token(p, &tok)) {
        if (omx_token_equals(&tok, "auto")) {
            emitter->flags |= OMX_EMITTER_AUTO_START;
        } else {
            return omx_error(p, "Unknown emitter flag");
        }
    }

    return true;
}

static
bool omx_line_state(OmxParser* p) NO_EXCEPT
{
    OmxFile* file = p->file;
    if (!p->fill) {
        ++file->state_field_count;
        p->pos = p->eol;

        return true;
    }

    OmxStateField* field = &file->state_fields[file->state_field_count++];
    if (!omx_read_name(p, &field->name)) {
        return false;
    }

    OmxToken tok;
    if (!omx_next_token(p, &tok)) {
        return omx_error(p, "Expected state type");
    }

    switch (omx_token_key(&tok)) {
        case omx_key("byte"): field->type = OMX_STATE_BYTE; break;
        case omx_key("i32"): field->type = OMX_STATE_INT32; break;
        case omx_key("u32"): field->type = OMX_STATE_UINT32; break;
        case omx_key("f32"): field->type = OMX_STATE_F32; break;
        case omx_key("v2"): field->type = OMX_STATE_V2; break;
        case omx_key("v3"): field->type = OMX_STATE_V3; break;
        case omx_key("v4"): field->type = OMX_STATE_V4; break;
        case omx_key("mat4"): field->type = OMX_STATE_MAT4; break;
        default: return omx_error(p, "Unknown state type");
    }

    int64 count = 1;
    if (omx_has_token(p) && !omx_read_int(p, &count, 1, 0xFFFFFF)) {
        return false;
    }

    const uint32 alignment = OMX_STATE_TYPE_ALIGNMENT[field->type];
    field->count = (uint32) count;
    field->offset = (file->state_size + alignment - 1) & ~(alignment - 1);
    file->state_size = field->offset + OMX_STATE_TYPE_BYTES[field->type] * field->count;

    return true;
}

// Opens a block (mat, skel, anim, hit, pfx)
static
bool omx_begin_block(OmxParser* p, OmxBlock block) NO_EXCEPT
{
    OmxFile* file = p->file;

    uint32 id;
    uint32 name;
    if (!omx_read_id(p, &id) || !omx_read_name(p, &name)) {
        return false;
    }

    if (id == OMX_INVALID_ID) {
        return omx_error(p, "Block requires an id");
    }

    p->block = block;

    switch (block) {
        case OMX_BLOCK_MATERIAL: {
                if (!p->fill) {
                    ++file->material_count;
                    break;
                }

                OmxMaterial* mat = &file->materials[file->material_count++];
                memset(mat, 0, sizeof(OmxMaterial));

                mat->id = id;
                mat->name = name;
                mat->base_color = { 1.0f, 1.0f, 1.0f, 1.0f };
                mat->roughness = 0.5f;
                mat->specular = 0.5f;
                mat->ior = 1.5f;
                mat->emissive_strength = 1.0f;
                mat->attenuation_color = { 1.0f, 1.0f, 1.0f };
                mat->subsurface_color = { 1.0f, 1.0f, 1.0f };
                mat->subsurface_radius = 1.0f;
                mat->iridescence_ior = 1.3f;
                mat->alpha_cutoff = 0.5f;
                mat->shader = OMX_INVALID_ID;
                mat->alpha_mode = OMX_ALPHA_OPAQUE;
                mat->flags = OMX_MATERIAL_CAST_SHADOW | OMX_MATERIAL_RECEIVE_SHADOW;

                for (int32 i = 0; i < OMX_TEXTURE_SLOT_SIZE; ++i) {
                    mat->textures[i].asset = OMX_INVALID_ID;
                    mat->textures[i].region = OMX_INVALID_ID;
                    mat->textures[i].scale = 1.0f;
                    mat->textures[i].uv_scale = { 1.0f, 1.0f };
                }
            } break;
        case OMX_BLOCK_SKELETON: {
                if (!p->fill) {
                    ++file->skeleton_count;
                    break;
                }

                OmxSkeleton* skel = &file->skeletons[file->skeleton_count++];
                skel->id = id;
                skel->name = name;
                skel->bone_offset = file->bone_count;
                skel->bone_count = 0;
                skel->height = 1.0f;
            } break;
        case OMX_BLOCK_ANIMATION: {
                p->channel = -1;

                if (!p->fill) {
                    ++file->animation_count;
                    break;
                }

                OmxAnimation* anim = &file->animations[file->animation_count++];
                memset(anim, 0, sizeof(OmxAnimation));

                anim->id = id;
                anim->name = name;
                anim->rig = OMX_INVALID_ID;
                anim->speed = 1.0f;
                anim->channel_offset = file->channel_count;
                anim->event_offset = file->event_count;
            } break;
        case OMX_BLOCK_HITBOX: {
                if (!p->fill) {
                    ++file->hitbox_count;
                    break;
                }

                OmxHitbox* hitbox = &file->hitboxes[file->hitbox_count++];
                hitbox->id = id;
                hitbox->name = name;
                hitbox->shape_offset = file->shape_count;
                hitbox->shape_count = 0;
            } break;
        case OMX_BLOCK_PARTICLE: {
                if (!p->fill) {
                    ++file->particle_system_count;
                    break;
                }

                OmxParticleSystem* pfx = &file->particle_systems[file->particle_system_count++];
                memset(pfx, 0, sizeof(OmxParticleSystem));

                pfx->id = id;
                pfx->name = name;
                pfx->material = OMX_INVALID_ID;
                pfx->max_particles = 64;
                pfx->rate = 10.0f;
                pfx->life = { 1.0f, 1.0f };
                pfx->speed = { 1.0f, 1.0f };
                pfx->frames_x = 1;
                pfx->frames_y = 1;
                pfx->shape = OMX_EMITTER_POINT;
                pfx->flags = OMX_PARTICLE_WORLD_SPACE;
            } break;
        default:
            UNREACHABLE();
    }

    // Measure pass doesn't care about the rest
    p->pos = p->eol;

    return true;
}

static
bool omx_line_root(OmxParser* p, uint64 key) NO_EXCEPT
{
    OmxFile* file = p->file;

    switch (key) {
        case omx_key("v"): return omx_line_vertex(p);
        case omx_key("f"): return omx_line_face(p);
        case omx_key("layout"): return omx_line_layout(p);
        case omx_key("mesh"): return omx_line_mesh(p);
        case omx_key("asset"): return omx_line_asset(p);
        case omx_key("socket"): return omx_line_socket(p);
        case omx_key("emitter"): return omx_line_emitter(p);
        case omx_key("state"): return omx_line_state(p);
        case omx_key("mat"): return omx_begin_block(p, OMX_BLOCK_MATERIAL);
        case omx_key("skel"): return omx_begin_block(p, OMX_BLOCK_SKELETON);
        case omx_key("anim"): return omx_begin_block(p, OMX_BLOCK_ANIMATION);
        case omx_key("hit"): return omx_begin_block(p, OMX_BLOCK_HITBOX);
        case omx_key("pfx"): return omx_begin_block(p, OMX_BLOCK_PARTICLE);
        case omx_key("model"): return omx_read_name(p, &file->name);
        case omx_key("useskel"): return omx_read_id(p, &file->skeleton);
        case omx_key("usehit"): return omx_read_id(p, &file->hitbox);
        case omx_key("layers"): {
                int64 layers;
                if (!omx_read_int(p, &layers, 1, 64)) {
                    return false;
                }

                file->anim_layers = (uint32) layers;

                return true;
            }
        case omx_key("slot"): {
                if (!p->fill) {
                    ++file->slot_count;
                    p->pos = p->eol;

                    return true;
                }

                OmxAnimationSlot* slot = &file->slots[file->slot_count++];

                return omx_read_name(p, &slot->name) && omx_read_id(p, &slot->animation);
            }
        default:
            return omx_error(p, "Unknown keyword");
    }
}

/////////////////////////////////////////////////////////////
// Material keywords
/////////////////////////////////////////////////////////////

static
bool omx_line_texture(OmxParser* p, OmxMaterial* mat, bool tiling) NO_EXCEPT
{
    OmxToken tok;
    if (!omx_next_token(p, &tok)) {
        return omx_error(p, "Expected texture slot");
    }

    OmxTextureSlot slot;
    switch (omx_token_key(&tok)) {
        case omx_key("base"): slot = OMX_TEXTURE_BASE; break;
        case omx_key("nrm"): slot = OMX_TEXTURE_NORMAL; break;
        case omx_key("orm"): slot = OMX_TEXTURE_ORM; break;
        case omx_key("emit"): slot = OMX_TEXTURE_EMISSIVE; break;
        case omx_key("trans"): slot = OMX_TEXTURE_TRANSMISSION; break;
        case omx_key("sss"): slot = OMX_TEXTURE_SUBSURFACE; break;
        case omx_key("coat"): slot = OMX_TEXTURE_CLEARCOAT; break;
        case omx_key("coatnrm"): slot = OMX_TEXTURE_CLEARCOAT_NORMAL; break;
        case omx_key("sheen"): slot = OMX_TEXTURE_SHEEN; break;
        case omx_key("aniso"): slot = OMX_TEXTURE_ANISOTROPY; break;
        case omx_key("spec"): slot = OMX_TEXTURE_SPECULAR; break;
        case omx_key("height"): slot = OMX_TEXTURE_HEIGHT; break;
        default: return omx_error(p, "Unknown texture slot");
    }

    OmxTextureRef* tex = &mat->textures[slot];

    // tile <slot> <su> <sv> [<ou> <ov>]
    if (tiling) {
        if (!omx_read_f32_array(p, tex->uv_scale.vec, 2)) {
            return false;
        }

        if (omx_has_token(p)) {
            return omx_read_f32_array(p, tex->uv_offset.vec, 2);
        }

        return true;
    }

    // tex <slot> <asset_id> [<atlas_region>|-] [<uv_set>] [<scale>]
    if (!omx_read_id(p, &tex->asset)) {
        return false;
    }

    if (omx_has_token(p) && !omx_read_id(p, &tex->region)) {
        return false;
    }

    if (omx_has_token(p)) {
        int64 uv_set;
        if (!omx_read_int(p, &uv_set, 0, 1)) {
            return false;
        }

        tex->uv_set = (byte) uv_set;
    }

    return omx_read_f32_optional(p, &tex->scale, 1.0f);
}

static
bool omx_line_material(OmxParser* p, uint64 key) NO_EXCEPT
{
    if (!p->fill) {
        p->pos = p->eol;

        return true;
    }

    OmxMaterial* mat = &p->file->materials[p->file->material_count - 1];

    switch (key) {
        case omx_key("base"):
            return omx_read_f32_array(p, mat->base_color.vec, 3)
                && omx_read_f32_optional(p, &mat->base_color.a, 1.0f);
        case omx_key("metal"): return omx_read_f32(p, &mat->metallic);
        case omx_key("rough"): return omx_read_f32(p, &mat->roughness);
        case omx_key("spec"): return omx_read_f32(p, &mat->specular);
        case omx_key("ior"): return omx_read_f32(p, &mat->ior);
        case omx_key("emit"):
            return omx_read_f32_array(p, mat->emissive.vec, 3)
                && omx_read_f32_optional(p, &mat->emissive_strength, 1.0f);
        case omx_key("trans"): return omx_read_f32(p, &mat->transmission);
        case omx_key("volume"):
            return omx_read_f32(p, &mat->thickness)
                && omx_read_f32_array(p, mat->attenuation_color.vec, 3)
                && omx_read_f32_optional(p, &mat->attenuation_distance, 0.0f);
        case omx_key("sss"):
            return omx_read_f32(p, &mat->subsurface)
                && omx_read_f32_array(p, mat->subsurface_color.vec, 3)
                && omx_read_f32_optional(p, &mat->subsurface_radius, 1.0f);
        case omx_key("coat"):
            return omx_read_f32(p, &mat->clearcoat)
                && omx_read_f32_optional(p, &mat->clearcoat_roughness, 0.0f);
        case omx_key("sheen"):
            return omx_read_f32_array(p, mat->sheen_color.vec, 3)
                && omx_read_f32_optional(p, &mat->sheen_roughness, 0.5f);
        case omx_key("aniso"):
            return omx_read_f32(p, &mat->anisotropy)
                && omx_read_f32_optional(p, &mat->anisotropy_rotation, 0.0f);
        case omx_key("irid"):
            return omx_read_f32(p, &mat->iridescence)
                && omx_read_f32_optional(p, &mat->iridescence_ior, 1.3f);
        case omx_key("shader"): return omx_read_id(p, &mat->shader);
        case omx_key("tex"): return omx_line_texture(p, mat, false);
        case omx_key("tile"): return omx_line_texture(p, mat, true);
        case omx_key("alpha"): {
                OmxToken tok;
                if (!omx_next_token(p, &tok)) {
                    return omx_error(p, "Expected alpha mode");
                }

                switch (omx_token_key(&tok)) {
                    case omx_key("opaque"): mat->alpha_mode = OMX_ALPHA_OPAQUE; return true;
                    case omx_key("mask"):
                        mat->alpha_mode = OMX_ALPHA_MASK;
                        return omx_read_f32_optional(p, &mat->alpha_cutoff, 0.5f);
                    case omx_key("blend"): mat->alpha_mode = OMX_ALPHA_BLEND; return true;
                    case omx_key("add"): mat->alpha_mode = OMX_ALPHA_ADD; return true;
                    default: return omx_error(p, "Unknown alpha mode");
                }
            }
        case omx_key("flags"): {
                OmxToken tok;
                while (omx_next_token(p, &tok)) {
                    switch (omx_token_key(&tok)) {
                        case omx_key("double"): mat->flags |= OMX_MATERIAL_DOUBLE_SIDED; break;
                        case omx_key("unlit"): mat->flags |= OMX_MATERIAL_UNLIT; break;
                        case omx_key("vcolor"): mat->flags |= OMX_MATERIAL_VERTEX_COLOR; break;
                        case omx_key("thin"): mat->flags |= OMX_MATERIAL_THIN_WALLED; break;
                        case omx_key("nocast"): mat->flags &= ~OMX_MATERIAL_CAST_SHADOW; break;
                        case omx_key("norecv"): mat->flags &= ~OMX_MATERIAL_RECEIVE_SHADOW; break;
                        default: return omx_error(p, "Unknown material flag");
                    }
                }

                return true;
            }
        default:
            return omx_error(p, "Unknown material keyword");
    }
}

/////////////////////////////////////////////////////////////
// Skeleton keywords
/////////////////////////////////////////////////////////////

static
bool omx_line_skeleton(OmxParser* p, uint64 key) NO_EXCEPT
{
    OmxFile* file = p->file;

    if (key == omx_key("bone")) {
        if (!p->fill) {
            ++file->bone_count;
            p->pos = p->eol;

            return true;
        }

        OmxSkeleton* skel = &file->skeletons[file->skeleton_count - 1];
        if (skel->bone_count >= OMX_INVALID_BONE) {
            return omx_error(p, "Too many bones");
        }

        OmxBone* bone = &file->bones[file->bone_count];

        uint32 parent_name;
        if (!omx_read_name(p, &bone->name) || !omx_read_name(p, &parent_name)) {
            return false;
        }

        if (bone->name == 0) {
            return omx_error(p, "Bone requires a name");
        }

        if (omx_skeleton_find_bone(file, skel, bone->name) != OMX_INVALID_BONE) {
            return omx_error(p, "Duplicate bone name");
        }

        // Parents must be defined before their children
        // -> a linear loop over the bones is always a valid hierarchy traversal
        bone->parent = OMX_INVALID_BONE;
        if (parent_name) {
            bone->parent = omx_skeleton_find_bone(file, skel, parent_name);
            if (bone->parent == OMX_INVALID_BONE) {
                return omx_error(p, "Parent bone must be defined before the child");
            }
        }

        bone->flags = 0;
        bone->rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        bone->scale = { 1.0f, 1.0f, 1.0f };

        if (!omx_read_f32_array(p, bone->position.vec, 3)) {
            return false;
        }

        if (omx_has_token(p) && !omx_read_quaternion(p, &bone->rotation)) {
            return false;
        }

        if (omx_has_token(p) && !omx_read_f32_array(p, bone->scale.vec, 3)) {
            return false;
        }

        ++file->bone_count;
        ++skel->bone_count;

        return true;
    }

    if (!p->fill) {
        p->pos = p->eol;

        return true;
    }

    if (key == omx_key("height")) {
        return omx_read_f32(p, &file->skeletons[file->skeleton_count - 1].height);
    }

    return omx_error(p, "Unknown skeleton keyword");
}

/////////////////////////////////////////////////////////////
// Animation keywords
/////////////////////////////////////////////////////////////

static
bool omx_line_animation(OmxParser* p, uint64 key) NO_EXCEPT
{
    OmxFile* file = p->file;

    // Counting
    if (!p->fill) {
        switch (key) {
            case omx_key("ch"): ++file->channel_count; p->channel = 0; break;
            case omx_key("k"): ++file->key_count; break;
            case omx_key("ev"): ++file->event_count; break;
            default: break;
        }

        p->pos = p->eol;

        return true;
    }

    OmxAnimation* anim = &file->animations[file->animation_count - 1];

    switch (key) {
        case omx_key("dur"): return omx_read_f32(p, &anim->duration);
        case omx_key("speed"): return omx_read_f32(p, &anim->speed);
        case omx_key("rig"): return omx_read_id(p, &anim->rig);
        case omx_key("loop"): anim->flags |= OMX_ANIMATION_LOOP; return true;
        case omx_key("additive"): anim->flags |= OMX_ANIMATION_ADDITIVE; return true;
        case omx_key("root"): anim->flags |= OMX_ANIMATION_ROOT_MOTION; return true;
        case omx_key("fulltran"): anim->flags |= OMX_ANIMATION_FULL_TRANSLATION; return true;
        case omx_key("ch"): {
                // ch <bone> <pos|rot|scl> [ease]
                p->channel = (int32) file->channel_count++;
                ++anim->channel_count;

                OmxChannel* ch = &file->channels[p->channel];
                ch->key_offset = file->key_count;
                ch->key_count = 0;

                if (!omx_read_name(p, &ch->bone)) {
                    return false;
                }

                OmxToken tok;
                if (!omx_next_token(p, &tok)) {
                    return omx_error(p, "Expected channel type (pos, rot, scl)");
                }

                switch (omx_token_key(&tok)) {
                    case omx_key("pos"): ch->path = OMX_CHANNEL_POSITION; break;
                    case omx_key("rot"): ch->path = OMX_CHANNEL_ROTATION; break;
                    case omx_key("scl"): ch->path = OMX_CHANNEL_SCALE; break;
                    default: return omx_error(p, "Unknown channel type (pos, rot, scl)");
                }

                p->channel_ease = ANIMATION_LINEAR;
                if (omx_has_token(p)) {
                    return omx_read_ease(p, &p->channel_ease);
                }

                return true;
            }
        case omx_key("k"): {
                // k <time> <values...> [ease]
                if (p->channel < 0) {
                    return omx_error(p, "Keyframe requires a channel (ch)");
                }

                OmxChannel* ch = &file->channels[p->channel];
                OmxKeyframe* key_frame = &file->keys[file->key_count];
                memset(key_frame, 0, sizeof(OmxKeyframe));

                if (!omx_read_f32(p, &key_frame->time)) {
                    return false;
                }

                if (key_frame->time < 0.0f
                    || (ch->key_count > 0 && key_frame->time <= file->keys[file->key_count - 1].time)
                ) {
                    return omx_error(p, "Keyframe times must be >= 0 and increasing");
                }

                if (ch->path == OMX_CHANNEL_ROTATION) {
                    quaternion q;
                    if (!omx_read_quaternion(p, &q)) {
                        return false;
                    }

                    memcpy(key_frame->value, q.vec, sizeof(f32) * 4);
                } else if (!omx_read_f32_array(p, key_frame->value, 3)) {
                    return false;
                }

                key_frame->ease = p->channel_ease;
                if (omx_has_token(p) && !omx_read_ease(p, &key_frame->ease)) {
                    return false;
                }

                ++file->key_count;
                ++ch->key_count;

                return true;
            }
        case omx_key("ev"): {
                // ev <time> <sound|pfx|tag> <ref> [bone]
                OmxEvent* ev = &file->events[file->event_count];
                if (!omx_read_f32(p, &ev->time)) {
                    return false;
                }

                if (anim->event_count > 0 && ev->time < file->events[file->event_count - 1].time) {
                    return omx_error(p, "Events must be sorted by time");
                }

                OmxToken tok;
                if (!omx_next_token(p, &tok)) {
                    return omx_error(p, "Expected event type (sound, pfx, tag)");
                }

                bool ok;
                switch (omx_token_key(&tok)) {
                    case omx_key("sound"): ev->type = OMX_EVENT_SOUND; ok = omx_read_id(p, &ev->ref); break;
                    case omx_key("pfx"): ev->type = OMX_EVENT_PARTICLE; ok = omx_read_name(p, &ev->ref); break;
                    case omx_key("tag"): ev->type = OMX_EVENT_TAG; ok = omx_read_name(p, &ev->ref); break;
                    default: return omx_error(p, "Unknown event type (sound, pfx, tag)");
                }

                if (!ok) {
                    return false;
                }

                ev->bone = 0;
                if (omx_has_token(p) && !omx_read_name(p, &ev->bone)) {
                    return false;
                }

                ++file->event_count;
                ++anim->event_count;

                return true;
            }
        default:
            return omx_error(p, "Unknown animation keyword");
    }
}

static
void omx_end_animation(OmxParser* p) NO_EXCEPT
{
    OmxFile* file = p->file;
    OmxAnimation* anim = &file->animations[file->animation_count - 1];

    // Duration = last keyframe/event if not explicitly defined
    if (anim->duration > 0.0f) {
        return;
    }

    for (uint32 i = 0; i < anim->channel_count; ++i) {
        const OmxChannel* ch = &file->channels[anim->channel_offset + i];
        if (ch->key_count) {
            const f32 last = file->keys[ch->key_offset + ch->key_count - 1].time;
            anim->duration = last > anim->duration ? last : anim->duration;
        }
    }

    if (anim->event_count) {
        const f32 last = file->events[anim->event_offset + anim->event_count - 1].time;
        anim->duration = last > anim->duration ? last : anim->duration;
    }
}

/////////////////////////////////////////////////////////////
// Hitbox keywords
/////////////////////////////////////////////////////////////

static
bool omx_line_hitbox(OmxParser* p, uint64 key) NO_EXCEPT
{
    OmxFile* file = p->file;

    OmxShapeType type;
    switch (key) {
        case omx_key("sphere"): type = OMX_SHAPE_SPHERE; break;
        case omx_key("capsule"): type = OMX_SHAPE_CAPSULE; break;
        case omx_key("box"): type = OMX_SHAPE_BOX; break;
        default: return omx_error(p, "Unknown hitbox shape (sphere, capsule, box)");
    }

    if (!p->fill) {
        ++file->shape_count;
        p->pos = p->eol;

        return true;
    }

    OmxShape* shape = &file->shapes[file->shape_count];
    memset(shape, 0, sizeof(OmxShape));
    shape->type = type;
    shape->multiplier = 1.0f;
    shape->rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

    OmxToken tok;
    if (!omx_next_token(p, &tok)) {
        return omx_error(p, "Expected hitbox layer");
    }

    switch (omx_token_key(&tok)) {
        case omx_key("world"): shape->layer = OMX_HIT_LAYER_WORLD; break;
        case omx_key("body"): shape->layer = OMX_HIT_LAYER_BODY; break;
        case omx_key("head"): shape->layer = OMX_HIT_LAYER_HEAD; break;
        case omx_key("weapon"): shape->layer = OMX_HIT_LAYER_WEAPON; break;
        case omx_key("trigger"): shape->layer = OMX_HIT_LAYER_TRIGGER; break;
        default: return omx_error(p, "Unknown hitbox layer");
    }

    if (!omx_read_name(p, &shape->bone)) {
        return false;
    }

    switch (type) {
        case OMX_SHAPE_SPHERE: {
                if (!omx_read_f32_array(p, shape->a.vec, 3) || !omx_read_f32(p, &shape->radius)) {
                    return false;
                }
            } break;
        case OMX_SHAPE_CAPSULE: {
                if (!omx_read_f32_array(p, shape->a.vec, 3)
                    || !omx_read_f32_array(p, shape->b.vec, 3)
                    || !omx_read_f32(p, &shape->radius)
                ) {
                    return false;
                }
            } break;
        case OMX_SHAPE_BOX: {
                if (!omx_read_f32_array(p, shape->a.vec, 3) || !omx_read_f32_array(p, shape->b.vec, 3)) {
                    return false;
                }

                // optional rotation (4 values) and/or multiplier (1 value)
                const int32 remaining = omx_remaining_tokens(p);
                if (remaining >= 4 && !omx_read_quaternion(p, &shape->rotation)) {
                    return false;
                }
            } break;
        default:
            UNREACHABLE();
    }

    if (!omx_read_f32_optional(p, &shape->multiplier, 1.0f)) {
        return false;
    }

    ++file->shape_count;
    ++file->hitboxes[file->hitbox_count - 1].shape_count;

    return true;
}

/////////////////////////////////////////////////////////////
// Particle keywords
/////////////////////////////////////////////////////////////

static
bool omx_line_particle(OmxParser* p, uint64 key) NO_EXCEPT
{
    if (!p->fill) {
        p->pos = p->eol;

        return true;
    }

    OmxParticleSystem* pfx = &p->file->particle_systems[p->file->particle_system_count - 1];

    switch (key) {
        case omx_key("max"): {
                int64 max;
                if (!omx_read_int(p, &max, 1, 1 << 20)) {
                    return false;
                }

                pfx->max_particles = (uint32) max;

                return true;
            }
        case omx_key("burst"): {
                int64 burst;
                if (!omx_read_int(p, &burst, 0, 1 << 20)) {
                    return false;
                }

                pfx->burst = (uint32) burst;

                return true;
            }
        case omx_key("mat"): return omx_read_id(p, &pfx->material);
        case omx_key("rate"): return omx_read_f32(p, &pfx->rate);
        case omx_key("dur"): return omx_read_f32(p, &pfx->duration);
        case omx_key("life"): return omx_read_f32_array(p, pfx->life.vec, 2);
        case omx_key("speed"): return omx_read_f32_array(p, pfx->speed.vec, 2);
        case omx_key("spin"): return omx_read_f32_array(p, pfx->spin.vec, 2);
        case omx_key("gravity"): return omx_read_f32_array(p, pfx->gravity.vec, 3);
        case omx_key("drag"): return omx_read_f32(p, &pfx->drag);
        case omx_key("loop"): pfx->flags |= OMX_PARTICLE_LOOP; return true;
        case omx_key("stretch"): pfx->flags |= OMX_PARTICLE_STRETCH; return true;
        case omx_key("collide"): pfx->flags |= OMX_PARTICLE_COLLIDE; return true;
        case omx_key("local"): pfx->flags &= ~OMX_PARTICLE_WORLD_SPACE; return true;
        case omx_key("flipbook"): {
                int64 x;
                int64 y;
                if (!omx_read_int(p, &x, 1, 0xFFFF) || !omx_read_int(p, &y, 1, 0xFFFF)) {
                    return false;
                }

                pfx->frames_x = (uint16) x;
                pfx->frames_y = (uint16) y;

                return omx_read_f32_optional(p, &pfx->fps, 0.0f);
            }
        case omx_key("shape"): {
                OmxToken tok;
                if (!omx_next_token(p, &tok)) {
                    return omx_error(p, "Expected emitter shape");
                }

                pfx->shape_param = {};
                switch (omx_token_key(&tok)) {
                    case omx_key("point"): pfx->shape = OMX_EMITTER_POINT; return true;
                    case omx_key("sphere"): pfx->shape = OMX_EMITTER_SPHERE; return omx_read_f32(p, &pfx->shape_param.x);
                    case omx_key("cone"):
                        pfx->shape = OMX_EMITTER_CONE;
                        return omx_read_f32(p, &pfx->shape_param.x)
                            && omx_read_f32_optional(p, &pfx->shape_param.y, 0.0f);
                    case omx_key("box"): pfx->shape = OMX_EMITTER_BOX; return omx_read_f32_array(p, pfx->shape_param.vec, 3);
                    default: return omx_error(p, "Unknown emitter shape (point, sphere, cone, box)");
                }
            }
        case omx_key("color"): {
                // color <t> <r> <g> <b> <a>
                if (pfx->color_key_count >= OMX_MAX_CURVE_KEYS) {
                    return omx_error(p, "Too many color keys");
                }

                OmxColorKey* ck = &pfx->color[pfx->color_key_count];
                if (!omx_read_f32(p, &ck->time) || !omx_read_f32_array(p, ck->color.vec, 4)) {
                    return false;
                }

                if (pfx->color_key_count > 0 && ck->time <= pfx->color[pfx->color_key_count - 1].time) {
                    return omx_error(p, "Color key times must be increasing");
                }

                ++pfx->color_key_count;

                return true;
            }
        case omx_key("size"): {
                // size <t> <value>
                if (pfx->size_key_count >= OMX_MAX_CURVE_KEYS) {
                    return omx_error(p, "Too many size keys");
                }

                OmxFloatKey* sk = &pfx->size[pfx->size_key_count];
                if (!omx_read_f32(p, &sk->time) || !omx_read_f32(p, &sk->value)) {
                    return false;
                }

                if (pfx->size_key_count > 0 && sk->time <= pfx->size[pfx->size_key_count - 1].time) {
                    return omx_error(p, "Size key times must be increasing");
                }

                ++pfx->size_key_count;

                return true;
            }
        default:
            return omx_error(p, "Unknown particle keyword");
    }
}

/////////////////////////////////////////////////////////////
// Driver
/////////////////////////////////////////////////////////////

static
bool omx_line(OmxParser* p, const OmxToken* keyword) NO_EXCEPT
{
    const uint64 key = omx_token_key(keyword);

    if (key == omx_key("end")) {
        if (p->block == OMX_BLOCK_NONE) {
            return omx_error(p, "end without block");
        }

        if (p->fill && p->block == OMX_BLOCK_ANIMATION) {
            omx_end_animation(p);
        }

        p->block = OMX_BLOCK_NONE;

        return true;
    }

    switch (p->block) {
        case OMX_BLOCK_NONE: return omx_line_root(p, key);
        case OMX_BLOCK_MATERIAL: return omx_line_material(p, key);
        case OMX_BLOCK_SKELETON: return omx_line_skeleton(p, key);
        case OMX_BLOCK_ANIMATION: return omx_line_animation(p, key);
        case OMX_BLOCK_HITBOX: return omx_line_hitbox(p, key);
        case OMX_BLOCK_PARTICLE: return omx_line_particle(p, key);
        default: UNREACHABLE();
    }
}

static
bool omx_run(const char* text, OmxFile* file, bool fill, OmxError* error) NO_EXCEPT
{
    OmxParser p = {};
    p.file = file;
    p.error = error;
    p.fill = fill;
    p.block = OMX_BLOCK_NONE;
    p.mesh = -1;
    p.channel = -1;

    bool has_header = false;
    const char* line = text;

    while (*line != '\0') {
        const char* line_end = line;
        while (*line_end != '\0' && *line_end != '\n') {
            ++line_end;
        }

        p.pos = line;
        p.eol = (line_end > line && line_end[-1] == '\r') ? line_end - 1 : line_end;
        ++p.line;

        OmxToken keyword;
        if (omx_next_token(&p, &keyword)) {
            if (!has_header) {
                // omx <version>
                int64 version;
                if (!omx_token_equals(&keyword, "omx") || !omx_read_int(&p, &version, 1, 0xFFFF)) {
                    return omx_error(&p, "File must start with: omx <version>");
                }

                if (version > OMX_VERSION) {
                    return omx_error(&p, "Unsupported omx version");
                }

                file->version = (uint32) version;
                has_header = true;
            } else if (!omx_line(&p, &keyword)) {
                return false;
            }

            if (omx_has_token(&p)) {
                return omx_error(&p, "Unexpected token");
            }
        }

        line = *line_end != '\0' ? line_end + 1 : line_end;
    }

    if (!has_header) {
        return omx_error(&p, "File must start with: omx <version>");
    }

    if (p.block != OMX_BLOCK_NONE) {
        return omx_error(&p, "Missing end");
    }

    if (file->skeleton != OMX_INVALID_ID && file->anim_layers == 0) {
        file->anim_layers = 1;
    }

    return true;
}

/////////////////////////////////////////////////////////////
// Public api
/////////////////////////////////////////////////////////////

// Pass 1: counts all elements, afterwards omx_memory_size() returns the required memory
bool omx_measure(const char* text, OmxFile* file, OmxError* error = NULL) NO_EXCEPT
{
    omx_file_init(file);

    return omx_run(text, file, false, error);
}

// Pass 2: fills the file. memory must be at least omx_memory_size() bytes and OMX_MEMORY_ALIGNMENT aligned
bool omx_parse(const char* text, OmxFile* file, byte* memory, OmxError* error = NULL) NO_EXCEPT
{
    const OmxFile measured = *file;

    omx_assign_memory(file, memory);

    // Reset all counts, they are incremented again during the fill pass
    file->asset_count = 0;
    file->string_size = 0;
    file->material_count = 0;
    file->skeleton_count = 0;
    file->bone_count = 0;
    file->animation_count = 0;
    file->channel_count = 0;
    file->key_count = 0;
    file->event_count = 0;
    file->hitbox_count = 0;
    file->shape_count = 0;
    file->particle_system_count = 0;
    file->vertex_count = 0;
    file->index_count = 0;
    file->mesh_count = 0;
    file->slot_count = 0;
    file->socket_count = 0;
    file->emitter_count = 0;
    file->state_field_count = 0;
    file->state_size = 0;

    if (!omx_run(text, file, true, error)) {
        return false;
    }

    // Both passes must see the same text
    if (file->vertex_count != measured.vertex_count
        || file->index_count != measured.index_count
        || file->key_count != measured.key_count
        || file->string_size != measured.string_size
    ) {
        if (error) {
            error->line = 0;
            error->message = "Text changed between omx_measure and omx_parse";
        }

        return false;
    }

    return true;
}

#endif
