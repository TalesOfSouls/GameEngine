/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_OBJECT_MODEL_PARSER_H
#define COMS_OBJECT_MODEL_PARSER_H

#include "../stdlib/Stdlib.h"
#include "Model.h"

// Text format *.modeltxt, the binary format is in Model.h (model_to_data, model_from_data)
//
// Usage:
//      ModelFile file;
//      ModelError error;
//      if (!model_measure(text, &file, &error)) { ... }
//      byte* memory = <reserve model_memory_size(&file) bytes, aligned to MODEL_MEMORY_ALIGNMENT>
//      if (!model_parse(text, &file, memory, &error)) { ... }
//
// text must be null terminated and MUST be identical for both calls

struct ModelError {
    int32 line;
    const char* message;
};

enum ModelBlock : byte {
    MODEL_BLOCK_NONE,
    MODEL_BLOCK_MATERIAL,
    MODEL_BLOCK_SKELETON,
    MODEL_BLOCK_ANIMATION,
    MODEL_BLOCK_HITBOX,
    MODEL_BLOCK_PARTICLE,
};

struct ModelToken {
    const char* str;
    int32 length;
};

struct ModelParser {
    const char* pos; // current position inside of the line
    const char* eol; // end of the current line (exclusive)
    int32 line;

    ModelFile* file;
    ModelError* error;

    // false = measure pass (only counting), true = fill pass
    bool fill;

    ModelBlock block;
    int32 mesh;          // current mesh, -1 = none
    int32 channel;       // current animation channel (absolute index), -1 = none
    AnimationEaseType channel_ease;

    // Requested size of the per instance state (memory keyword)
    uint32 state_memory;
};

/////////////////////////////////////////////////////////////
// Tokenizer
/////////////////////////////////////////////////////////////

// Packs a keyword (max 8 characters) into an integer -> keywords can be used in switch statements
constexpr
uint64 model_key(const char* str) NO_EXCEPT
{
    uint64 key = 0;
    for (int32 i = 0; i < 8 && str[i] != '\0'; ++i) {
        key |= ((uint64) (byte) str[i]) << (i * 8);
    }

    return key;
}

static inline
uint64 model_token_key(const ModelToken* tok) NO_EXCEPT
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
bool model_token_equals(const ModelToken* tok, const char* str) NO_EXCEPT
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
bool model_error(ModelParser* p, const char* message) NO_EXCEPT
{
    if (p->error) {
        p->error->line = p->line;
        p->error->message = message;
    }

    return false;
}

static inline
bool model_next_token(ModelParser* p, ModelToken* tok) NO_EXCEPT
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
bool model_has_token(ModelParser* p) NO_EXCEPT
{
    const char* pos = p->pos;
    ModelToken tok;
    const bool has_token = model_next_token(p, &tok);
    p->pos = pos;

    return has_token;
}

static inline
int32 model_remaining_tokens(ModelParser* p) NO_EXCEPT
{
    const char* pos = p->pos;
    int32 count = 0;

    ModelToken tok;
    while (model_next_token(p, &tok)) {
        ++count;
    }

    p->pos = pos;

    return count;
}

// We don't use str_to_float since we need exponent support (e.g. 1e-05 from exporters) and more than 6 decimals
static
bool model_token_to_f32(const ModelToken* tok, f32* out) NO_EXCEPT
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
bool model_token_to_int64(const ModelToken* tok, int64* out) NO_EXCEPT
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
bool model_read_f32(ModelParser* p, f32* out) NO_EXCEPT
{
    ModelToken tok;
    if (!model_next_token(p, &tok) || !model_token_to_f32(&tok, out)) {
        return model_error(p, "Expected number");
    }

    return true;
}

static
bool model_read_f32_array(ModelParser* p, f32* out, int32 count) NO_EXCEPT
{
    for (int32 i = 0; i < count; ++i) {
        if (!model_read_f32(p, &out[i])) {
            return false;
        }
    }

    return true;
}

// Optional value at the end of a line
static
bool model_read_f32_optional(ModelParser* p, f32* out, f32 default_value) NO_EXCEPT
{
    if (!model_has_token(p)) {
        *out = default_value;

        return true;
    }

    return model_read_f32(p, out);
}

// '-' = MODEL_INVALID_ID
static
bool model_read_id(ModelParser* p, uint32* out) NO_EXCEPT
{
    ModelToken tok;
    if (!model_next_token(p, &tok)) {
        return model_error(p, "Expected id");
    }

    if (tok.length == 1 && *tok.str == '-') {
        *out = MODEL_INVALID_ID;

        return true;
    }

    int64 value;
    if (!model_token_to_int64(&tok, &value) || value < 0 || value >= MODEL_INVALID_ID) {
        return model_error(p, "Expected id (uint32)");
    }

    *out = (uint32) value;

    return true;
}

static
bool model_read_int(ModelParser* p, int64* out, int64 min, int64 max) NO_EXCEPT
{
    ModelToken tok;
    if (!model_next_token(p, &tok) || !model_token_to_int64(&tok, out)) {
        return model_error(p, "Expected integer");
    }

    if (*out < min || *out > max) {
        return model_error(p, "Integer out of range");
    }

    return true;
}

// Names are stored as hash, '-' = 0 (= no name)
static
bool model_read_name(ModelParser* p, uint32* out) NO_EXCEPT
{
    ModelToken tok;
    if (!model_next_token(p, &tok)) {
        return model_error(p, "Expected name");
    }

    *out = (tok.length == 1 && *tok.str == '-') ? 0 : model_hash(tok.str, tok.length);

    return true;
}

static
bool model_read_quaternion(ModelParser* p, quaternion* out) NO_EXCEPT
{
    if (!model_read_f32_array(p, out->vec, 4)) {
        return false;
    }

    const f32 length_sq = out->x * out->x + out->y * out->y + out->z * out->z + out->w * out->w;
    if (length_sq < 0.000001f) {
        return model_error(p, "Invalid quaternion");
    }

    const f32 inv_length = 1.0f / sqrtf(length_sq);
    out->x *= inv_length;
    out->y *= inv_length;
    out->z *= inv_length;
    out->w *= inv_length;

    return true;
}

struct ModelEaseName {
    const char* name;
    AnimationEaseType type;
};

static const ModelEaseName MODEL_EASE_NAMES[] = {
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
bool model_read_ease(ModelParser* p, AnimationEaseType* out) NO_EXCEPT
{
    ModelToken tok;
    if (!model_next_token(p, &tok)) {
        return model_error(p, "Expected ease type");
    }

    for (int32 i = 0; i < ARRAY_COUNT(MODEL_EASE_NAMES); ++i) {
        if (model_token_equals(&tok, MODEL_EASE_NAMES[i].name)) {
            *out = MODEL_EASE_NAMES[i].type;

            return true;
        }
    }

    return model_error(p, "Unknown ease type");
}

/////////////////////////////////////////////////////////////
// Memory
/////////////////////////////////////////////////////////////

FORCE_INLINE
uint64 model_align(uint64 size) NO_EXCEPT
{
    return (size + (MODEL_MEMORY_ALIGNMENT - 1)) & ~((uint64) MODEL_MEMORY_ALIGNMENT - 1);
}

// Memory required for the parsed file (call after model_measure)
uint64 model_memory_size(const ModelFile* file) NO_EXCEPT
{
    return model_align(sizeof(ModelAsset) * file->asset_count)
        + model_align(file->string_size)
        + model_align(sizeof(ModelMaterial) * file->material_count)
        + model_align(sizeof(ModelSkeleton) * file->skeleton_count)
        + model_align(sizeof(ModelBone) * file->bone_count)
        + model_align(sizeof(ModelAnimation) * file->animation_count)
        + model_align(sizeof(ModelChannel) * file->channel_count)
        + model_align(sizeof(ModelKeyframe) * file->key_count)
        + model_align(sizeof(ModelEvent) * file->event_count)
        + model_align(sizeof(ModelHitbox) * file->hitbox_count)
        + model_align(sizeof(ModelShape) * file->shape_count)
        + model_align(sizeof(ModelParticleSystem) * file->particle_system_count)
        + model_align((uint64) file->vertex_stride * file->vertex_count)
        + model_align(sizeof(uint32) * file->index_count)
        + model_align(sizeof(ModelMesh) * file->mesh_count)
        + model_align(sizeof(ModelAnimationSlot) * file->slot_count)
        + model_align(sizeof(ModelSocket) * file->socket_count)
        + model_align(sizeof(ModelEmitter) * file->emitter_count)
        + model_align(sizeof(ModelLight) * file->light_count)
        + model_align(sizeof(ModelStateField) * file->state_field_count);
}

static
byte* model_carve(byte** pos, uint64 size) NO_EXCEPT
{
    byte* ptr = *pos;
    *pos += model_align(size);

    return size ? ptr : NULL;
}

// Subdivides the memory block based on the measured counts
static
void model_assign_memory(ModelFile* file, byte* memory) NO_EXCEPT
{
    byte* pos = memory;

    file->data = memory;
    file->data_size = model_memory_size(file);

    file->assets = (ModelAsset *) model_carve(&pos, sizeof(ModelAsset) * file->asset_count);
    file->strings = (char *) model_carve(&pos, file->string_size);
    file->materials = (ModelMaterial *) model_carve(&pos, sizeof(ModelMaterial) * file->material_count);
    file->skeletons = (ModelSkeleton *) model_carve(&pos, sizeof(ModelSkeleton) * file->skeleton_count);
    file->bones = (ModelBone *) model_carve(&pos, sizeof(ModelBone) * file->bone_count);
    file->animations = (ModelAnimation *) model_carve(&pos, sizeof(ModelAnimation) * file->animation_count);
    file->channels = (ModelChannel *) model_carve(&pos, sizeof(ModelChannel) * file->channel_count);
    file->keys = (ModelKeyframe *) model_carve(&pos, sizeof(ModelKeyframe) * file->key_count);
    file->events = (ModelEvent *) model_carve(&pos, sizeof(ModelEvent) * file->event_count);
    file->hitboxes = (ModelHitbox *) model_carve(&pos, sizeof(ModelHitbox) * file->hitbox_count);
    file->shapes = (ModelShape *) model_carve(&pos, sizeof(ModelShape) * file->shape_count);
    file->particle_systems = (ModelParticleSystem *) model_carve(&pos, sizeof(ModelParticleSystem) * file->particle_system_count);
    file->vertices = model_carve(&pos, (uint64) file->vertex_stride * file->vertex_count);
    file->indices = (uint32 *) model_carve(&pos, sizeof(uint32) * file->index_count);
    file->meshes = (ModelMesh *) model_carve(&pos, sizeof(ModelMesh) * file->mesh_count);
    file->slots = (ModelAnimationSlot *) model_carve(&pos, sizeof(ModelAnimationSlot) * file->slot_count);
    file->sockets = (ModelSocket *) model_carve(&pos, sizeof(ModelSocket) * file->socket_count);
    file->emitters = (ModelEmitter *) model_carve(&pos, sizeof(ModelEmitter) * file->emitter_count);
    file->lights = (ModelLight *) model_carve(&pos, sizeof(ModelLight) * file->light_count);
    file->state_fields = (ModelStateField *) model_carve(&pos, sizeof(ModelStateField) * file->state_field_count);
}

static
void model_file_init(ModelFile* file) NO_EXCEPT
{
    memset(file, 0, sizeof(ModelFile));
    file->version = MODEL_VERSION;
    file->skeleton = MODEL_INVALID_ID;
    file->hitbox = MODEL_INVALID_ID;
}

/////////////////////////////////////////////////////////////
// Root keywords
/////////////////////////////////////////////////////////////

static
bool model_line_asset(ModelParser* p) NO_EXCEPT
{
    ModelFile* file = p->file;

    uint32 id;
    if (!model_read_id(p, &id)) {
        return false;
    }

    ModelToken type_tok;
    ModelToken path_tok;
    if (!model_next_token(p, &type_tok) || !model_next_token(p, &path_tok)) {
        return model_error(p, "Expected: asset <id> <type> <path>");
    }

    if (!p->fill) {
        ++file->asset_count;
        file->string_size += path_tok.length + 1;

        return true;
    }

    ModelAssetType type;
    switch (model_token_key(&type_tok)) {
        case model_key("model"): type = MODEL_ASSET_MODEL; break;
        case model_key("tex"): type = MODEL_ASSET_TEXTURE; break;
        case model_key("audio"): type = MODEL_ASSET_AUDIO; break;
        case model_key("shader"): type = MODEL_ASSET_SHADER; break;
        case model_key("font"): type = MODEL_ASSET_FONT; break;
        case model_key("data"): type = MODEL_ASSET_DATA; break;
        default: return model_error(p, "Unknown asset type");
    }

    ModelAsset* asset = &file->assets[file->asset_count++];
    asset->id = id;
    asset->type = type;
    asset->path_offset = file->string_size;

    memcpy(file->strings + file->string_size, path_tok.str, path_tok.length);
    file->strings[file->string_size + path_tok.length] = '\0';
    file->string_size += path_tok.length + 1;

    return true;
}

static
bool model_line_layout(ModelParser* p) NO_EXCEPT
{
    ModelFile* file = p->file;
    if (file->vertex_count) {
        return model_error(p, "layout must be defined before the first vertex");
    }

    uint32 layout = 0;
    uint32 last = 0;

    ModelToken tok;
    while (model_next_token(p, &tok)) {
        uint32 attribute;
        switch (model_token_key(&tok)) {
            case model_key("pos"): attribute = MODEL_VERTEX_POSITION; break;
            case model_key("nrm"): attribute = MODEL_VERTEX_NORMAL; break;
            case model_key("tan"): attribute = MODEL_VERTEX_TANGENT; break;
            case model_key("uv0"): attribute = MODEL_VERTEX_UV0; break;
            case model_key("uv1"): attribute = MODEL_VERTEX_UV1; break;
            case model_key("col"): attribute = MODEL_VERTEX_COLOR; break;
            case model_key("skin"): attribute = MODEL_VERTEX_SKIN; break;
            default: return model_error(p, "Unknown vertex attribute");
        }

        // Enforcing the canonical order makes the v lines unambiguous and memcpy friendly
        if (attribute <= last) {
            return model_error(p, "Vertex attributes must be in the order: pos nrm tan uv0 uv1 col skin");
        }

        layout |= attribute;
        last = attribute;
    }

    if (!(layout & MODEL_VERTEX_POSITION)) {
        return model_error(p, "Vertex layout requires pos");
    }

    file->vertex_layout = layout;
    file->vertex_stride = model_vertex_stride(layout);

    return true;
}

static
bool model_read_vertex_attribute(ModelParser* p, byte* dst, int32 floats) NO_EXCEPT
{
    f32 values[4];
    if (!model_read_f32_array(p, values, floats)) {
        return false;
    }

    memcpy(dst, values, floats * sizeof(f32));

    return true;
}

static
bool model_line_vertex(ModelParser* p) NO_EXCEPT
{
    ModelFile* file = p->file;
    if (!file->vertex_layout) {
        return model_error(p, "layout must be defined before the first vertex");
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
    if (!model_read_f32_array(p, position.vec, 3)) {
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

    if (layout & MODEL_VERTEX_NORMAL) {
        if (!model_read_vertex_attribute(p, dst, 3)) {
            return false;
        }

        dst += 12;
    }

    if (layout & MODEL_VERTEX_TANGENT) {
        if (!model_read_vertex_attribute(p, dst, 4)) {
            return false;
        }

        dst += 16;
    }

    if (layout & MODEL_VERTEX_UV0) {
        if (!model_read_vertex_attribute(p, dst, 2)) {
            return false;
        }

        dst += 8;
    }

    if (layout & MODEL_VERTEX_UV1) {
        if (!model_read_vertex_attribute(p, dst, 2)) {
            return false;
        }

        dst += 8;
    }

    if (layout & MODEL_VERTEX_COLOR) {
        f32 color[4];
        if (!model_read_f32_array(p, color, 4)) {
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

    if (layout & MODEL_VERTEX_SKIN) {
        // bone weight pairs until the end of the line
        // If more than 4 influences are defined we only keep the 4 strongest ones
        ModelVertexSkin skin = {};
        int32 influences = 0;

        while (model_has_token(p)) {
            int64 bone;
            f32 weight;
            if (!model_read_int(p, &bone, 0, MODEL_INVALID_BONE - 1) || !model_read_f32(p, &weight)) {
                return false;
            }

            if (weight <= 0.0f) {
                continue;
            }

            // Insertion sort (descending weight)
            int32 i = influences < MODEL_MAX_BONE_INFLUENCES ? influences++ : MODEL_MAX_BONE_INFLUENCES;
            while (i > 0 && skin.weights[i - 1] < weight) {
                if (i < MODEL_MAX_BONE_INFLUENCES) {
                    skin.weights[i] = skin.weights[i - 1];
                    skin.bones[i] = skin.bones[i - 1];
                }

                --i;
            }

            if (i < MODEL_MAX_BONE_INFLUENCES) {
                skin.weights[i] = weight;
                skin.bones[i] = (uint16) bone;
            }
        }

        if (influences == 0) {
            return model_error(p, "Skinned vertex requires at least one bone weight pair");
        }

        // Normalize: 0.7 + 0.3 = 1.0 but also e.g. 7 + 3 is allowed
        f32 sum = 0.0f;
        for (int32 i = 0; i < MODEL_MAX_BONE_INFLUENCES; ++i) {
            sum += skin.weights[i];
        }

        for (int32 i = 0; i < MODEL_MAX_BONE_INFLUENCES; ++i) {
            skin.weights[i] /= sum;
        }

        memcpy(dst, &skin, sizeof(skin));
        dst += sizeof(skin);
    }

    ++file->vertex_count;

    return true;
}

static
void model_begin_mesh(ModelParser* p, uint32 name, uint32 material) NO_EXCEPT
{
    ModelFile* file = p->file;
    p->mesh = (int32) file->mesh_count++;

    if (p->fill) {
        ModelMesh* mesh = &file->meshes[p->mesh];
        mesh->name = name;
        mesh->material = material;
        mesh->index_offset = file->index_count;
        mesh->index_count = 0;
    }
}

static
bool model_line_mesh(ModelParser* p) NO_EXCEPT
{
    uint32 name;
    uint32 material;
    if (!model_read_name(p, &name) || !model_read_id(p, &material)) {
        return false;
    }

    model_begin_mesh(p, name, material);

    return true;
}

static
bool model_read_index(ModelParser* p, uint32* out) NO_EXCEPT
{
    int64 index;
    if (!model_read_int(p, &index, -(int64) MODEL_INVALID_ID, MODEL_INVALID_ID)) {
        return false;
    }

    // Negative = relative to the last defined vertex (-1 = last vertex)
    if (index < 0) {
        index += p->file->vertex_count;
    }

    if (index < 0 || index >= p->file->vertex_count) {
        return model_error(p, "Vertex index out of range (vertices must be defined before the face)");
    }

    *out = (uint32) index;

    return true;
}

static
bool model_line_face(ModelParser* p) NO_EXCEPT
{
    ModelFile* file = p->file;

    // Faces without a mesh line end up in a default mesh
    if (p->mesh < 0) {
        model_begin_mesh(p, 0, MODEL_INVALID_ID);
    }

    const int32 corners = model_remaining_tokens(p);
    if (corners < 3) {
        return model_error(p, "Face requires at least 3 indices");
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
    if (!model_read_index(p, &first) || !model_read_index(p, &previous)) {
        return false;
    }

    for (int32 i = 2; i < corners; ++i) {
        uint32 current;
        if (!model_read_index(p, &current)) {
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
bool model_line_socket(ModelParser* p) NO_EXCEPT
{
    ModelFile* file = p->file;
    if (!p->fill) {
        ++file->socket_count;
        p->pos = p->eol;

        return true;
    }

    ModelSocket* socket = &file->sockets[file->socket_count++];
    if (!model_read_name(p, &socket->name)
        || !model_read_name(p, &socket->bone)
        || !model_read_f32_array(p, socket->position.vec, 3)
    ) {
        return false;
    }

    socket->rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (model_has_token(p)) {
        return model_read_quaternion(p, &socket->rotation);
    }

    return true;
}

static
bool model_line_emitter(ModelParser* p) NO_EXCEPT
{
    ModelFile* file = p->file;
    if (!p->fill) {
        ++file->emitter_count;
        p->pos = p->eol;

        return true;
    }

    ModelEmitter* emitter = &file->emitters[file->emitter_count++];
    if (!model_read_name(p, &emitter->name)
        || !model_read_id(p, &emitter->particle_system)
        || !model_read_name(p, &emitter->bone)
        || !model_read_f32_array(p, emitter->offset.vec, 3)
    ) {
        return false;
    }

    emitter->flags = 0;

    ModelToken tok;
    while (model_next_token(p, &tok)) {
        if (model_token_equals(&tok, "auto")) {
            emitter->flags |= MODEL_EMITTER_AUTO_START;
        } else {
            return model_error(p, "Unknown emitter flag");
        }
    }

    return true;
}

// light <name> <bone|-> <x> <y> <z> <r> <g> <b> <intensity> <range>
static
bool model_line_light(ModelParser* p) NO_EXCEPT
{
    ModelFile* file = p->file;
    if (!p->fill) {
        ++file->light_count;
        p->pos = p->eol;

        return true;
    }

    ModelLight* light = &file->lights[file->light_count++];
    if (!model_read_name(p, &light->name)
        || !model_read_name(p, &light->bone)
        || !model_read_f32_array(p, light->offset.vec, 3)
        || !model_read_f32_array(p, light->color.vec, 3)
        || !model_read_f32(p, &light->intensity)
        || !model_read_f32(p, &light->range)
    ) {
        return false;
    }

    if (light->range <= 0.0f) {
        return model_error(p, "Light range must be > 0");
    }

    return true;
}

static
bool model_line_state(ModelParser* p) NO_EXCEPT
{
    ModelFile* file = p->file;
    if (!p->fill) {
        ++file->state_field_count;
        p->pos = p->eol;

        return true;
    }

    ModelStateField* field = &file->state_fields[file->state_field_count++];
    if (!model_read_name(p, &field->name)) {
        return false;
    }

    ModelToken tok;
    if (!model_next_token(p, &tok)) {
        return model_error(p, "Expected state type");
    }

    switch (model_token_key(&tok)) {
        case model_key("byte"): field->type = MODEL_STATE_BYTE; break;
        case model_key("i32"): field->type = MODEL_STATE_INT32; break;
        case model_key("u32"): field->type = MODEL_STATE_UINT32; break;
        case model_key("f32"): field->type = MODEL_STATE_F32; break;
        case model_key("v2"): field->type = MODEL_STATE_V2; break;
        case model_key("v3"): field->type = MODEL_STATE_V3; break;
        case model_key("v4"): field->type = MODEL_STATE_V4; break;
        case model_key("mat4"): field->type = MODEL_STATE_MAT4; break;
        default: return model_error(p, "Unknown state type");
    }

    int64 count = 1;
    if (model_has_token(p) && !model_read_int(p, &count, 1, 0xFFFFFF)) {
        return false;
    }

    const uint32 alignment = MODEL_STATE_TYPE_ALIGNMENT[field->type];
    field->count = (uint32) count;
    field->offset = (file->state_size + alignment - 1) & ~(alignment - 1);
    file->state_size = field->offset + MODEL_STATE_TYPE_BYTES[field->type] * field->count;

    return true;
}

// Opens a block (mat, skel, anim, hit, pfx)
static
bool model_begin_block(ModelParser* p, ModelBlock block) NO_EXCEPT
{
    ModelFile* file = p->file;

    uint32 id;
    uint32 name;
    if (!model_read_id(p, &id) || !model_read_name(p, &name)) {
        return false;
    }

    if (id == MODEL_INVALID_ID) {
        return model_error(p, "Block requires an id");
    }

    p->block = block;

    switch (block) {
        case MODEL_BLOCK_MATERIAL: {
                if (!p->fill) {
                    ++file->material_count;
                    break;
                }

                ModelMaterial* mat = &file->materials[file->material_count++];
                memset(mat, 0, sizeof(ModelMaterial));

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
                mat->shader = MODEL_INVALID_ID;
                mat->alpha_mode = MODEL_ALPHA_OPAQUE;
                mat->flags = MODEL_MATERIAL_CAST_SHADOW | MODEL_MATERIAL_RECEIVE_SHADOW;

                for (int32 i = 0; i < MODEL_TEXTURE_SLOT_SIZE; ++i) {
                    mat->textures[i].asset = MODEL_INVALID_ID;
                    mat->textures[i].region = MODEL_INVALID_ID;
                    mat->textures[i].scale = 1.0f;
                    mat->textures[i].uv_scale = { 1.0f, 1.0f };
                }
            } break;
        case MODEL_BLOCK_SKELETON: {
                if (!p->fill) {
                    ++file->skeleton_count;
                    break;
                }

                ModelSkeleton* skel = &file->skeletons[file->skeleton_count++];
                skel->id = id;
                skel->name = name;
                skel->bone_offset = file->bone_count;
                skel->bone_count = 0;
                skel->height = 1.0f;
            } break;
        case MODEL_BLOCK_ANIMATION: {
                p->channel = -1;

                if (!p->fill) {
                    ++file->animation_count;
                    break;
                }

                ModelAnimation* anim = &file->animations[file->animation_count++];
                memset(anim, 0, sizeof(ModelAnimation));

                anim->id = id;
                anim->name = name;
                anim->rig = MODEL_INVALID_ID;
                anim->speed = 1.0f;
                anim->channel_offset = file->channel_count;
                anim->event_offset = file->event_count;
            } break;
        case MODEL_BLOCK_HITBOX: {
                if (!p->fill) {
                    ++file->hitbox_count;
                    break;
                }

                ModelHitbox* hitbox = &file->hitboxes[file->hitbox_count++];
                hitbox->id = id;
                hitbox->name = name;
                hitbox->shape_offset = file->shape_count;
                hitbox->shape_count = 0;
            } break;
        case MODEL_BLOCK_PARTICLE: {
                if (!p->fill) {
                    ++file->particle_system_count;
                    break;
                }

                ModelParticleSystem* pfx = &file->particle_systems[file->particle_system_count++];
                memset(pfx, 0, sizeof(ModelParticleSystem));

                pfx->id = id;
                pfx->name = name;
                pfx->material = MODEL_INVALID_ID;
                pfx->max_particles = 64;
                pfx->rate = 10.0f;
                pfx->life = { 1.0f, 1.0f };
                pfx->speed = { 1.0f, 1.0f };
                pfx->frames_x = 1;
                pfx->frames_y = 1;
                pfx->shape = MODEL_EMITTER_POINT;
                pfx->flags = MODEL_PARTICLE_WORLD_SPACE;
            } break;
        default:
            UNREACHABLE();
    }

    // Measure pass doesn't care about the rest
    p->pos = p->eol;

    return true;
}

static
bool model_line_root(ModelParser* p, uint64 key) NO_EXCEPT
{
    ModelFile* file = p->file;

    switch (key) {
        case model_key("v"): return model_line_vertex(p);
        case model_key("f"): return model_line_face(p);
        case model_key("layout"): return model_line_layout(p);
        case model_key("mesh"): return model_line_mesh(p);
        case model_key("asset"): return model_line_asset(p);
        case model_key("socket"): return model_line_socket(p);
        case model_key("emitter"): return model_line_emitter(p);
        case model_key("light"): return model_line_light(p);
        case model_key("state"): return model_line_state(p);
        case model_key("mat"): return model_begin_block(p, MODEL_BLOCK_MATERIAL);
        case model_key("skel"): return model_begin_block(p, MODEL_BLOCK_SKELETON);
        case model_key("anim"): return model_begin_block(p, MODEL_BLOCK_ANIMATION);
        case model_key("hit"): return model_begin_block(p, MODEL_BLOCK_HITBOX);
        case model_key("pfx"): return model_begin_block(p, MODEL_BLOCK_PARTICLE);
        case model_key("model"): return model_read_name(p, &file->name);
        case model_key("useskel"): return model_read_id(p, &file->skeleton);
        case model_key("usehit"): return model_read_id(p, &file->hitbox);
        case model_key("memory"): {
                // Min. bytes of the per instance state (applied after the file, @see model_run)
                int64 bytes;
                if (!model_read_int(p, &bytes, 0, 0xFFFFFF)) {
                    return false;
                }

                p->state_memory = OMS_MAX(p->state_memory, (uint32) bytes);

                return true;
            }
        case model_key("layers"): {
                int64 layers;
                if (!model_read_int(p, &layers, 1, 64)) {
                    return false;
                }

                file->anim_layers = (uint32) layers;

                return true;
            }
        case model_key("slot"): {
                if (!p->fill) {
                    ++file->slot_count;
                    p->pos = p->eol;

                    return true;
                }

                ModelAnimationSlot* slot = &file->slots[file->slot_count++];

                return model_read_name(p, &slot->name) && model_read_id(p, &slot->animation);
            }
        default:
            return model_error(p, "Unknown keyword");
    }
}

/////////////////////////////////////////////////////////////
// Material keywords
/////////////////////////////////////////////////////////////

static
bool model_line_texture(ModelParser* p, ModelMaterial* mat, bool tiling) NO_EXCEPT
{
    ModelToken tok;
    if (!model_next_token(p, &tok)) {
        return model_error(p, "Expected texture slot");
    }

    ModelTextureSlot slot;
    switch (model_token_key(&tok)) {
        case model_key("base"): slot = MODEL_TEXTURE_BASE; break;
        case model_key("nrm"): slot = MODEL_TEXTURE_NORMAL; break;
        case model_key("orm"): slot = MODEL_TEXTURE_ORM; break;
        case model_key("emit"): slot = MODEL_TEXTURE_EMISSIVE; break;
        case model_key("trans"): slot = MODEL_TEXTURE_TRANSMISSION; break;
        case model_key("sss"): slot = MODEL_TEXTURE_SUBSURFACE; break;
        case model_key("coat"): slot = MODEL_TEXTURE_CLEARCOAT; break;
        case model_key("coatnrm"): slot = MODEL_TEXTURE_CLEARCOAT_NORMAL; break;
        case model_key("sheen"): slot = MODEL_TEXTURE_SHEEN; break;
        case model_key("aniso"): slot = MODEL_TEXTURE_ANISOTROPY; break;
        case model_key("spec"): slot = MODEL_TEXTURE_SPECULAR; break;
        case model_key("height"): slot = MODEL_TEXTURE_HEIGHT; break;
        default: return model_error(p, "Unknown texture slot");
    }

    ModelTextureRef* tex = &mat->textures[slot];

    // tile <slot> <su> <sv> [<ou> <ov>]
    if (tiling) {
        if (!model_read_f32_array(p, tex->uv_scale.vec, 2)) {
            return false;
        }

        if (model_has_token(p)) {
            return model_read_f32_array(p, tex->uv_offset.vec, 2);
        }

        return true;
    }

    // tex <slot> <asset_id> [<atlas_region>|-] [<uv_set>] [<scale>]
    if (!model_read_id(p, &tex->asset)) {
        return false;
    }

    if (model_has_token(p) && !model_read_id(p, &tex->region)) {
        return false;
    }

    if (model_has_token(p)) {
        int64 uv_set;
        if (!model_read_int(p, &uv_set, 0, 1)) {
            return false;
        }

        tex->uv_set = (byte) uv_set;
    }

    return model_read_f32_optional(p, &tex->scale, 1.0f);
}

static
bool model_line_material(ModelParser* p, uint64 key) NO_EXCEPT
{
    if (!p->fill) {
        p->pos = p->eol;

        return true;
    }

    ModelMaterial* mat = &p->file->materials[p->file->material_count - 1];

    switch (key) {
        case model_key("base"):
            return model_read_f32_array(p, mat->base_color.vec, 3)
                && model_read_f32_optional(p, &mat->base_color.a, 1.0f);
        case model_key("metal"): return model_read_f32(p, &mat->metallic);
        case model_key("rough"): return model_read_f32(p, &mat->roughness);
        case model_key("spec"): return model_read_f32(p, &mat->specular);
        case model_key("ior"): return model_read_f32(p, &mat->ior);
        case model_key("emit"):
            return model_read_f32_array(p, mat->emissive.vec, 3)
                && model_read_f32_optional(p, &mat->emissive_strength, 1.0f);
        case model_key("trans"): return model_read_f32(p, &mat->transmission);
        case model_key("volume"):
            return model_read_f32(p, &mat->thickness)
                && model_read_f32_array(p, mat->attenuation_color.vec, 3)
                && model_read_f32_optional(p, &mat->attenuation_distance, 0.0f);
        case model_key("sss"):
            return model_read_f32(p, &mat->subsurface)
                && model_read_f32_array(p, mat->subsurface_color.vec, 3)
                && model_read_f32_optional(p, &mat->subsurface_radius, 1.0f);
        case model_key("coat"):
            return model_read_f32(p, &mat->clearcoat)
                && model_read_f32_optional(p, &mat->clearcoat_roughness, 0.0f);
        case model_key("sheen"):
            return model_read_f32_array(p, mat->sheen_color.vec, 3)
                && model_read_f32_optional(p, &mat->sheen_roughness, 0.5f);
        case model_key("aniso"):
            return model_read_f32(p, &mat->anisotropy)
                && model_read_f32_optional(p, &mat->anisotropy_rotation, 0.0f);
        case model_key("irid"):
            return model_read_f32(p, &mat->iridescence)
                && model_read_f32_optional(p, &mat->iridescence_ior, 1.3f);
        case model_key("shader"): return model_read_id(p, &mat->shader);
        case model_key("tex"): return model_line_texture(p, mat, false);
        case model_key("tile"): return model_line_texture(p, mat, true);
        case model_key("alpha"): {
                ModelToken tok;
                if (!model_next_token(p, &tok)) {
                    return model_error(p, "Expected alpha mode");
                }

                switch (model_token_key(&tok)) {
                    case model_key("opaque"): mat->alpha_mode = MODEL_ALPHA_OPAQUE; return true;
                    case model_key("mask"):
                        mat->alpha_mode = MODEL_ALPHA_MASK;
                        return model_read_f32_optional(p, &mat->alpha_cutoff, 0.5f);
                    case model_key("blend"): mat->alpha_mode = MODEL_ALPHA_BLEND; return true;
                    case model_key("add"): mat->alpha_mode = MODEL_ALPHA_ADD; return true;
                    default: return model_error(p, "Unknown alpha mode");
                }
            }
        case model_key("flags"): {
                ModelToken tok;
                while (model_next_token(p, &tok)) {
                    switch (model_token_key(&tok)) {
                        case model_key("double"): mat->flags |= MODEL_MATERIAL_DOUBLE_SIDED; break;
                        case model_key("unlit"): mat->flags |= MODEL_MATERIAL_UNLIT; break;
                        case model_key("vcolor"): mat->flags |= MODEL_MATERIAL_VERTEX_COLOR; break;
                        case model_key("thin"): mat->flags |= MODEL_MATERIAL_THIN_WALLED; break;
                        case model_key("nocast"): mat->flags &= ~MODEL_MATERIAL_CAST_SHADOW; break;
                        case model_key("norecv"): mat->flags &= ~MODEL_MATERIAL_RECEIVE_SHADOW; break;
                        default: return model_error(p, "Unknown material flag");
                    }
                }

                return true;
            }
        default:
            return model_error(p, "Unknown material keyword");
    }
}

/////////////////////////////////////////////////////////////
// Skeleton keywords
/////////////////////////////////////////////////////////////

static
bool model_line_skeleton(ModelParser* p, uint64 key) NO_EXCEPT
{
    ModelFile* file = p->file;

    if (key == model_key("bone")) {
        if (!p->fill) {
            ++file->bone_count;
            p->pos = p->eol;

            return true;
        }

        ModelSkeleton* skel = &file->skeletons[file->skeleton_count - 1];
        if (skel->bone_count >= MODEL_INVALID_BONE) {
            return model_error(p, "Too many bones");
        }

        ModelBone* bone = &file->bones[file->bone_count];

        uint32 parent_name;
        if (!model_read_name(p, &bone->name) || !model_read_name(p, &parent_name)) {
            return false;
        }

        if (bone->name == 0) {
            return model_error(p, "Bone requires a name");
        }

        if (model_skeleton_find_bone(file, skel, bone->name) != MODEL_INVALID_BONE) {
            return model_error(p, "Duplicate bone name");
        }

        // Parents must be defined before their children
        // -> a linear loop over the bones is always a valid hierarchy traversal
        bone->parent = MODEL_INVALID_BONE;
        if (parent_name) {
            bone->parent = model_skeleton_find_bone(file, skel, parent_name);
            if (bone->parent == MODEL_INVALID_BONE) {
                return model_error(p, "Parent bone must be defined before the child");
            }
        }

        bone->flags = 0;
        bone->rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        bone->scale = { 1.0f, 1.0f, 1.0f };

        if (!model_read_f32_array(p, bone->position.vec, 3)) {
            return false;
        }

        if (model_has_token(p) && !model_read_quaternion(p, &bone->rotation)) {
            return false;
        }

        if (model_has_token(p) && !model_read_f32_array(p, bone->scale.vec, 3)) {
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

    if (key == model_key("height")) {
        return model_read_f32(p, &file->skeletons[file->skeleton_count - 1].height);
    }

    return model_error(p, "Unknown skeleton keyword");
}

/////////////////////////////////////////////////////////////
// Animation keywords
/////////////////////////////////////////////////////////////

static
bool model_line_animation(ModelParser* p, uint64 key) NO_EXCEPT
{
    ModelFile* file = p->file;

    // Counting
    if (!p->fill) {
        switch (key) {
            case model_key("ch"): ++file->channel_count; p->channel = 0; break;
            case model_key("k"): ++file->key_count; break;
            case model_key("ev"): ++file->event_count; break;
            default: break;
        }

        p->pos = p->eol;

        return true;
    }

    ModelAnimation* anim = &file->animations[file->animation_count - 1];

    switch (key) {
        case model_key("dur"): return model_read_f32(p, &anim->duration);
        case model_key("speed"): return model_read_f32(p, &anim->speed);
        case model_key("rig"): return model_read_id(p, &anim->rig);
        case model_key("loop"): anim->flags |= MODEL_ANIMATION_LOOP; return true;
        case model_key("additive"): anim->flags |= MODEL_ANIMATION_ADDITIVE; return true;
        case model_key("root"): anim->flags |= MODEL_ANIMATION_ROOT_MOTION; return true;
        case model_key("fulltran"): anim->flags |= MODEL_ANIMATION_FULL_TRANSLATION; return true;
        case model_key("ch"): {
                // ch <bone> <pos|rot|scl> [ease]
                p->channel = (int32) file->channel_count++;
                ++anim->channel_count;

                ModelChannel* ch = &file->channels[p->channel];
                ch->key_offset = file->key_count;
                ch->key_count = 0;

                if (!model_read_name(p, &ch->bone)) {
                    return false;
                }

                ModelToken tok;
                if (!model_next_token(p, &tok)) {
                    return model_error(p, "Expected channel type (pos, rot, scl)");
                }

                switch (model_token_key(&tok)) {
                    case model_key("pos"): ch->path = MODEL_CHANNEL_POSITION; break;
                    case model_key("rot"): ch->path = MODEL_CHANNEL_ROTATION; break;
                    case model_key("scl"): ch->path = MODEL_CHANNEL_SCALE; break;
                    default: return model_error(p, "Unknown channel type (pos, rot, scl)");
                }

                p->channel_ease = ANIMATION_LINEAR;
                if (model_has_token(p)) {
                    return model_read_ease(p, &p->channel_ease);
                }

                return true;
            }
        case model_key("k"): {
                // k <time> <values...> [ease]
                if (p->channel < 0) {
                    return model_error(p, "Keyframe requires a channel (ch)");
                }

                ModelChannel* ch = &file->channels[p->channel];
                ModelKeyframe* key_frame = &file->keys[file->key_count];
                memset(key_frame, 0, sizeof(ModelKeyframe));

                if (!model_read_f32(p, &key_frame->time)) {
                    return false;
                }

                if (key_frame->time < 0.0f
                    || (ch->key_count > 0 && key_frame->time <= file->keys[file->key_count - 1].time)
                ) {
                    return model_error(p, "Keyframe times must be >= 0 and increasing");
                }

                if (ch->path == MODEL_CHANNEL_ROTATION) {
                    quaternion q;
                    if (!model_read_quaternion(p, &q)) {
                        return false;
                    }

                    memcpy(key_frame->value, q.vec, sizeof(f32) * 4);
                } else if (!model_read_f32_array(p, key_frame->value, 3)) {
                    return false;
                }

                key_frame->ease = p->channel_ease;
                if (model_has_token(p) && !model_read_ease(p, &key_frame->ease)) {
                    return false;
                }

                ++file->key_count;
                ++ch->key_count;

                return true;
            }
        case model_key("ev"): {
                // ev <time> <sound|pfx|tag> <ref> [bone]
                ModelEvent* ev = &file->events[file->event_count];
                if (!model_read_f32(p, &ev->time)) {
                    return false;
                }

                if (anim->event_count > 0 && ev->time < file->events[file->event_count - 1].time) {
                    return model_error(p, "Events must be sorted by time");
                }

                ModelToken tok;
                if (!model_next_token(p, &tok)) {
                    return model_error(p, "Expected event type (sound, pfx, tag)");
                }

                bool ok;
                switch (model_token_key(&tok)) {
                    case model_key("sound"): ev->type = MODEL_EVENT_SOUND; ok = model_read_id(p, &ev->ref); break;
                    case model_key("pfx"): ev->type = MODEL_EVENT_PARTICLE; ok = model_read_name(p, &ev->ref); break;
                    case model_key("tag"): ev->type = MODEL_EVENT_TAG; ok = model_read_name(p, &ev->ref); break;
                    default: return model_error(p, "Unknown event type (sound, pfx, tag)");
                }

                if (!ok) {
                    return false;
                }

                ev->bone = 0;
                if (model_has_token(p) && !model_read_name(p, &ev->bone)) {
                    return false;
                }

                ++file->event_count;
                ++anim->event_count;

                return true;
            }
        default:
            return model_error(p, "Unknown animation keyword");
    }
}

static
void model_end_animation(ModelParser* p) NO_EXCEPT
{
    ModelFile* file = p->file;
    ModelAnimation* anim = &file->animations[file->animation_count - 1];

    // Duration = last keyframe/event if not explicitly defined
    if (anim->duration > 0.0f) {
        return;
    }

    for (uint32 i = 0; i < anim->channel_count; ++i) {
        const ModelChannel* ch = &file->channels[anim->channel_offset + i];
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
bool model_line_hitbox(ModelParser* p, uint64 key) NO_EXCEPT
{
    ModelFile* file = p->file;

    ModelShapeType type;
    switch (key) {
        case model_key("sphere"): type = MODEL_SHAPE_SPHERE; break;
        case model_key("capsule"): type = MODEL_SHAPE_CAPSULE; break;
        case model_key("box"): type = MODEL_SHAPE_BOX; break;
        default: return model_error(p, "Unknown hitbox shape (sphere, capsule, box)");
    }

    if (!p->fill) {
        ++file->shape_count;
        p->pos = p->eol;

        return true;
    }

    ModelShape* shape = &file->shapes[file->shape_count];
    memset(shape, 0, sizeof(ModelShape));
    shape->type = type;
    shape->multiplier = 1.0f;
    shape->rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

    ModelToken tok;
    if (!model_next_token(p, &tok)) {
        return model_error(p, "Expected hitbox layer");
    }

    switch (model_token_key(&tok)) {
        case model_key("world"): shape->layer = MODEL_HIT_LAYER_WORLD; break;
        case model_key("body"): shape->layer = MODEL_HIT_LAYER_BODY; break;
        case model_key("head"): shape->layer = MODEL_HIT_LAYER_HEAD; break;
        case model_key("weapon"): shape->layer = MODEL_HIT_LAYER_WEAPON; break;
        case model_key("trigger"): shape->layer = MODEL_HIT_LAYER_TRIGGER; break;
        default: return model_error(p, "Unknown hitbox layer");
    }

    if (!model_read_name(p, &shape->bone)) {
        return false;
    }

    switch (type) {
        case MODEL_SHAPE_SPHERE: {
                if (!model_read_f32_array(p, shape->a.vec, 3) || !model_read_f32(p, &shape->radius)) {
                    return false;
                }
            } break;
        case MODEL_SHAPE_CAPSULE: {
                if (!model_read_f32_array(p, shape->a.vec, 3)
                    || !model_read_f32_array(p, shape->b.vec, 3)
                    || !model_read_f32(p, &shape->radius)
                ) {
                    return false;
                }
            } break;
        case MODEL_SHAPE_BOX: {
                if (!model_read_f32_array(p, shape->a.vec, 3) || !model_read_f32_array(p, shape->b.vec, 3)) {
                    return false;
                }

                // optional rotation (4 values) and/or multiplier (1 value)
                const int32 remaining = model_remaining_tokens(p);
                if (remaining >= 4 && !model_read_quaternion(p, &shape->rotation)) {
                    return false;
                }
            } break;
        default:
            UNREACHABLE();
    }

    if (!model_read_f32_optional(p, &shape->multiplier, 1.0f)) {
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
bool model_line_particle(ModelParser* p, uint64 key) NO_EXCEPT
{
    if (!p->fill) {
        p->pos = p->eol;

        return true;
    }

    ModelParticleSystem* pfx = &p->file->particle_systems[p->file->particle_system_count - 1];

    switch (key) {
        case model_key("max"): {
                int64 max;
                if (!model_read_int(p, &max, 1, 1 << 20)) {
                    return false;
                }

                pfx->max_particles = (uint32) max;

                return true;
            }
        case model_key("burst"): {
                int64 burst;
                if (!model_read_int(p, &burst, 0, 1 << 20)) {
                    return false;
                }

                pfx->burst = (uint32) burst;

                return true;
            }
        case model_key("mat"): return model_read_id(p, &pfx->material);
        case model_key("rate"): return model_read_f32(p, &pfx->rate);
        case model_key("dur"): return model_read_f32(p, &pfx->duration);
        case model_key("life"): return model_read_f32_array(p, pfx->life.vec, 2);
        case model_key("speed"): return model_read_f32_array(p, pfx->speed.vec, 2);
        case model_key("spin"): return model_read_f32_array(p, pfx->spin.vec, 2);
        case model_key("gravity"): return model_read_f32_array(p, pfx->gravity.vec, 3);
        case model_key("drag"): return model_read_f32(p, &pfx->drag);
        case model_key("loop"): pfx->flags |= MODEL_PARTICLE_LOOP; return true;
        case model_key("stretch"): pfx->flags |= MODEL_PARTICLE_STRETCH; return true;
        case model_key("collide"): pfx->flags |= MODEL_PARTICLE_COLLIDE; return true;
        case model_key("local"): pfx->flags &= ~MODEL_PARTICLE_WORLD_SPACE; return true;
        case model_key("flipbook"): {
                int64 x;
                int64 y;
                if (!model_read_int(p, &x, 1, 0xFFFF) || !model_read_int(p, &y, 1, 0xFFFF)) {
                    return false;
                }

                pfx->frames_x = (uint16) x;
                pfx->frames_y = (uint16) y;

                return model_read_f32_optional(p, &pfx->fps, 0.0f);
            }
        case model_key("shape"): {
                ModelToken tok;
                if (!model_next_token(p, &tok)) {
                    return model_error(p, "Expected emitter shape");
                }

                pfx->shape_param = {};
                switch (model_token_key(&tok)) {
                    case model_key("point"): pfx->shape = MODEL_EMITTER_POINT; return true;
                    case model_key("sphere"): pfx->shape = MODEL_EMITTER_SPHERE; return model_read_f32(p, &pfx->shape_param.x);
                    case model_key("cone"):
                        pfx->shape = MODEL_EMITTER_CONE;
                        return model_read_f32(p, &pfx->shape_param.x)
                            && model_read_f32_optional(p, &pfx->shape_param.y, 0.0f);
                    case model_key("box"): pfx->shape = MODEL_EMITTER_BOX; return model_read_f32_array(p, pfx->shape_param.vec, 3);
                    default: return model_error(p, "Unknown emitter shape (point, sphere, cone, box)");
                }
            }
        case model_key("color"): {
                // color <t> <r> <g> <b> <a>
                if (pfx->color_key_count >= MODEL_MAX_CURVE_KEYS) {
                    return model_error(p, "Too many color keys");
                }

                ModelColorKey* ck = &pfx->color[pfx->color_key_count];
                if (!model_read_f32(p, &ck->time) || !model_read_f32_array(p, ck->color.vec, 4)) {
                    return false;
                }

                if (pfx->color_key_count > 0 && ck->time <= pfx->color[pfx->color_key_count - 1].time) {
                    return model_error(p, "Color key times must be increasing");
                }

                ++pfx->color_key_count;

                return true;
            }
        case model_key("size"): {
                // size <t> <value>
                if (pfx->size_key_count >= MODEL_MAX_CURVE_KEYS) {
                    return model_error(p, "Too many size keys");
                }

                ModelFloatKey* sk = &pfx->size[pfx->size_key_count];
                if (!model_read_f32(p, &sk->time) || !model_read_f32(p, &sk->value)) {
                    return false;
                }

                if (pfx->size_key_count > 0 && sk->time <= pfx->size[pfx->size_key_count - 1].time) {
                    return model_error(p, "Size key times must be increasing");
                }

                ++pfx->size_key_count;

                return true;
            }
        default:
            return model_error(p, "Unknown particle keyword");
    }
}

/////////////////////////////////////////////////////////////
// Driver
/////////////////////////////////////////////////////////////

static
bool model_line(ModelParser* p, const ModelToken* keyword) NO_EXCEPT
{
    const uint64 key = model_token_key(keyword);

    if (key == model_key("end")) {
        if (p->block == MODEL_BLOCK_NONE) {
            return model_error(p, "end without block");
        }

        if (p->fill && p->block == MODEL_BLOCK_ANIMATION) {
            model_end_animation(p);
        }

        p->block = MODEL_BLOCK_NONE;

        return true;
    }

    switch (p->block) {
        case MODEL_BLOCK_NONE: return model_line_root(p, key);
        case MODEL_BLOCK_MATERIAL: return model_line_material(p, key);
        case MODEL_BLOCK_SKELETON: return model_line_skeleton(p, key);
        case MODEL_BLOCK_ANIMATION: return model_line_animation(p, key);
        case MODEL_BLOCK_HITBOX: return model_line_hitbox(p, key);
        case MODEL_BLOCK_PARTICLE: return model_line_particle(p, key);
        default: UNREACHABLE();
    }
}

static
bool model_run(const char* text, ModelFile* file, bool fill, ModelError* error) NO_EXCEPT
{
    ModelParser p = {};
    p.file = file;
    p.error = error;
    p.fill = fill;
    p.block = MODEL_BLOCK_NONE;
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

        ModelToken keyword;
        if (model_next_token(&p, &keyword)) {
            if (!has_header) {
                // version <version>
                int64 version;
                if (!model_token_equals(&keyword, "version") || !model_read_int(&p, &version, 1, 0xFFFF)) {
                    return model_error(&p, "File must start with: version <version>");
                }

                if (version > MODEL_VERSION) {
                    return model_error(&p, "Unsupported model file version");
                }

                file->version = (uint32) version;
                has_header = true;
            } else if (!model_line(&p, &keyword)) {
                return false;
            }

            if (model_has_token(&p)) {
                return model_error(&p, "Unexpected token");
            }
        }

        line = *line_end != '\0' ? line_end + 1 : line_end;
    }

    if (!has_header) {
        return model_error(&p, "File must start with: version <version>");
    }

    if (p.block != MODEL_BLOCK_NONE) {
        return model_error(&p, "Missing end");
    }

    if (file->skeleton != MODEL_INVALID_ID && file->anim_layers == 0) {
        file->anim_layers = 1;
    }

    // The state fields are placed at the start of the state block, the memory keyword only enlarges it
    file->state_size = OMS_MAX(file->state_size, p.state_memory);

    return true;
}

/////////////////////////////////////////////////////////////
// Public api
/////////////////////////////////////////////////////////////

// Pass 1: counts all elements, afterwards model_memory_size() returns the required memory
bool model_measure(const char* text, ModelFile* file, ModelError* error = NULL) NO_EXCEPT
{
    model_file_init(file);

    return model_run(text, file, false, error);
}

// Pass 2: fills the file. memory must be at least model_memory_size() bytes and MODEL_MEMORY_ALIGNMENT aligned
bool model_parse(const char* text, ModelFile* file, byte* memory, ModelError* error = NULL) NO_EXCEPT
{
    const ModelFile measured = *file;

    model_assign_memory(file, memory);

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
    file->light_count = 0;
    file->state_field_count = 0;
    file->state_size = 0;

    if (!model_run(text, file, true, error)) {
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
            error->message = "Text changed between model_measure and model_parse";
        }

        return false;
    }

    return true;
}

#endif
