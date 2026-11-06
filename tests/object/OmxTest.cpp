#include <stdio.h>
#include "../../object/OmxParser.h"
#include "../TestFramework.h"
#include "../../object/OmxAnimation.h"
#include "../../object/OmxLibrary.h"
#include "../../object/OmxBinary.h"

#ifndef OMX_EXAMPLE_DIR
    // Relative to the cOMS directory (tests.bat runs the tests from there)
    #define OMX_EXAMPLE_DIR "tests/object/omx"
#endif

alignas(64) static byte _omx_test_memory[1 << 20];
static uint64 _omx_test_memory_pos = 0;

static byte* omx_test_alloc(uint64 size)
{
    byte* ptr = _omx_test_memory + _omx_test_memory_pos;
    _omx_test_memory_pos += (size + 63) & ~63ULL;

    return ptr;
}

static bool omx_test_parse_text(const char* text, OmxFile* file, OmxError* error)
{
    if (!omx_measure(text, file, error)) {
        return false;
    }

    return omx_parse(text, file, omx_test_alloc(omx_memory_size(file)), error);
}

static bool omx_test_load(const char* name, OmxFile* file)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", OMX_EXAMPLE_DIR, name);

    FILE* fp = fopen(path, "rb");
    if (!fp) {
        printf("Couldn't open %s\n", path);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    const long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* text = (char *) omx_test_alloc(size + 1);
    fread(text, 1, size, fp);
    text[size] = '\0';
    fclose(fp);

    OmxError error = {};
    if (!omx_test_parse_text(text, file, &error)) {
        printf("%s:%d %s\n", name, error.line, error.message);
        return false;
    }

    return true;
}

static OmxFile _omx_rig;
static OmxFile _omx_metals;
static OmxFile _omx_locomotion;
static OmxFile _omx_hit;
static OmxFile _omx_fx;
static OmxFile _omx_guard;

static void test_omx_load_examples()
{
    TEST_TRUE(omx_test_load("humanoid_rig.omx", &_omx_rig));
    TEST_TRUE(omx_test_load("metals.omx", &_omx_metals));
    TEST_TRUE(omx_test_load("humanoid_locomotion.omx", &_omx_locomotion));
    TEST_TRUE(omx_test_load("humanoid_hit.omx", &_omx_hit));
    TEST_TRUE(omx_test_load("fx.omx", &_omx_fx));
    TEST_TRUE(omx_test_load("guard.omx", &_omx_guard));
}

static void test_omx_number_parsing()
{
    f32 value;
    OmxToken tok;

    tok = { "1e-05", 5 };
    TEST_TRUE(omx_token_to_f32(&tok, &value));
    TEST_EQUALS_WITH_DELTA(value, 0.00001f, 0.0000001f);

    tok = { "-0.123456789", 12 };
    TEST_TRUE(omx_token_to_f32(&tok, &value));
    TEST_EQUALS_WITH_DELTA(value, -0.123456789f, 0.0000001f);

    tok = { "2.5E2", 5 };
    TEST_TRUE(omx_token_to_f32(&tok, &value));
    TEST_EQUALS_WITH_DELTA(value, 250.0f, 0.0001f);

    tok = { "1.2.3", 5 };
    TEST_FALSE(omx_token_to_f32(&tok, &value));

    tok = { "abc", 3 };
    TEST_FALSE(omx_token_to_f32(&tok, &value));
}

static void test_omx_model()
{
    const OmxFile* f = &_omx_guard;

    TEST_EQUALS(f->name, omx_hash("guard"));
    TEST_EQUALS(f->asset_count, 6);
    TEST_TRUE(strcmp(omx_asset_path(f, &f->assets[1]), "metals.omx") == 0);
    TEST_EQUALS(f->assets[5].type, OMX_ASSET_TEXTURE);

    TEST_EQUALS(f->skeleton, 10);
    TEST_EQUALS(f->hitbox, 300);
    TEST_EQUALS(f->anim_layers, 2);
    TEST_EQUALS(f->material_count, 1);
    TEST_EQUALS(f->slot_count, 2);
    TEST_EQUALS(omx_find_slot(f, omx_hash("wave")), 2002);
    TEST_EQUALS(omx_find_slot(f, omx_hash("swim")), OMX_INVALID_ID);
    TEST_EQUALS(f->socket_count, 2);
    TEST_EQUALS(f->sockets[1].bone, omx_hash("head"));
    TEST_EQUALS(f->emitter_count, 2);
    TEST_EQUALS(f->emitters[1].flags & OMX_EMITTER_AUTO_START, OMX_EMITTER_AUTO_START);

    // pos nrm uv0 col skin = 12 + 12 + 8 + 4 + 24
    TEST_EQUALS(f->vertex_layout, OMX_VERTEX_POSITION | OMX_VERTEX_NORMAL | OMX_VERTEX_UV0 | OMX_VERTEX_COLOR | OMX_VERTEX_SKIN);
    TEST_EQUALS(f->vertex_stride, 60);
    TEST_EQUALS(f->vertex_count, 6);
    TEST_EQUALS(omx_vertex_offset(f->vertex_layout, OMX_VERTEX_COLOR), 32);
    TEST_EQUALS(omx_vertex_offset(f->vertex_layout, OMX_VERTEX_SKIN), 36);
    TEST_EQUALS(omx_vertex_offset(f->vertex_layout, OMX_VERTEX_TANGENT), -1);

    // 70% forearm (5), 30% upper arm (4) -> sorted by weight
    OmxVertexSkin skin;
    memcpy(&skin, f->vertices + f->vertex_stride * 1 + 36, sizeof(skin));
    TEST_EQUALS(skin.bones[0], 5);
    TEST_EQUALS(skin.bones[1], 4);
    TEST_EQUALS_WITH_DELTA(skin.weights[0], 0.7f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(skin.weights[1], 0.3f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(skin.weights[2], 0.0f, 0.0001f);

    // "5 1 6 1" gets normalized to 0.5 0.5
    memcpy(&skin, f->vertices + f->vertex_stride * 4 + 36, sizeof(skin));
    TEST_EQUALS_WITH_DELTA(skin.weights[0] + skin.weights[1], 1.0f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(skin.weights[0], 0.5f, 0.0001f);

    uint32 color;
    memcpy(&color, f->vertices + f->vertex_stride * 4 + 32, sizeof(color));
    TEST_EQUALS(color & 0xFF, 230);
    TEST_EQUALS(color >> 24, 255);

    // quads -> 2 triangles each, -4 = relative index
    TEST_EQUALS(f->mesh_count, 2);
    TEST_EQUALS(f->index_count, 12);
    const uint32 expected[12] = { 0, 1, 2, 0, 2, 3, 1, 4, 5, 1, 5, 2 };
    TEST_MEMORY_EQUALS(f->indices, expected, sizeof(expected));
    TEST_EQUALS(f->meshes[1].material, 200);
    TEST_EQUALS(f->meshes[1].index_offset, 6);
    TEST_EQUALS(f->meshes[1].index_count, 6);

    TEST_EQUALS_WITH_DELTA(f->bounds_min.x, 0.2f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(f->bounds_max.x, 0.78f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(f->bounds_min.y, 1.25f, 0.0001f);

    // health f32 @0, cooldown f32[4] @4, flags u32 @20, target v3 @24
    TEST_EQUALS(f->state_field_count, 4);
    TEST_EQUALS(omx_find_state_field(f, omx_hash("cooldown"))->offset, 4);
    TEST_EQUALS(omx_find_state_field(f, omx_hash("flags"))->offset, 20);
    TEST_EQUALS(omx_find_state_field(f, omx_hash("target"))->offset, 24);
    TEST_EQUALS(f->state_size, 36);
}

static void test_omx_material()
{
    OmxFile* files[] = { &_omx_guard, &_omx_metals };
    const OmxLibrary lib = { 2, (const OmxFile **) files };

    const OmxMaterial* skin = omx_find_material(&lib, 100);
    TEST_TRUE(skin != NULL);
    TEST_EQUALS_WITH_DELTA(skin->subsurface, 0.4f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(skin->subsurface_radius, 0.01f, 0.0001f);
    TEST_EQUALS(skin->textures[OMX_TEXTURE_BASE].asset, 7010);
    TEST_EQUALS(skin->textures[OMX_TEXTURE_BASE].region, OMX_INVALID_ID);

    const OmxMaterial* steel = omx_find_material(&lib, 200);
    TEST_TRUE(steel != NULL);
    TEST_EQUALS_WITH_DELTA(steel->metallic, 1.0f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(steel->anisotropy_rotation, 0.25f, 0.0001f);
    TEST_EQUALS(steel->textures[OMX_TEXTURE_NORMAL].asset, 7002);
    TEST_EQUALS(steel->textures[OMX_TEXTURE_NORMAL].region, 12);
    TEST_EQUALS_WITH_DELTA(steel->textures[OMX_TEXTURE_NORMAL].scale, 0.8f, 0.0001f);

    const OmxMaterial* glass = omx_find_material(&lib, 202);
    TEST_EQUALS(glass->alpha_mode, OMX_ALPHA_BLEND);
    TEST_EQUALS_WITH_DELTA(glass->transmission, 1.0f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(glass->attenuation_distance, 0.5f, 0.0001f);
    TEST_TRUE(glass->flags & OMX_MATERIAL_DOUBLE_SIDED);
    TEST_FALSE(glass->flags & OMX_MATERIAL_CAST_SHADOW);
    TEST_TRUE(glass->flags & OMX_MATERIAL_RECEIVE_SHADOW);

    const OmxMaterial* fire = omx_find_material(&lib, 203);
    TEST_EQUALS(fire->alpha_mode, OMX_ALPHA_ADD);
    TEST_EQUALS_WITH_DELTA(fire->emissive_strength, 4.0f, 0.0001f);

    TEST_TRUE(omx_find_material(&lib, 999) == NULL);
}

static void test_omx_animation_retarget()
{
    const OmxSkeleton* rig = &_omx_rig.skeletons[0];
    TEST_EQUALS(rig->bone_count, 14);

    const OmxAnimation* walk = &_omx_locomotion.animations[0];
    TEST_EQUALS(walk->channel_count, 4);
    TEST_EQUALS(walk->event_count, 4);
    TEST_TRUE(walk->flags & OMX_ANIMATION_LOOP);

    // "tail" doesn't exist on the humanoid rig
    uint16 channel_to_bone[4];
    TEST_EQUALS(omx_animation_bind(&_omx_locomotion, walk, &_omx_rig, rig, channel_to_bone), 3);
    TEST_EQUALS(channel_to_bone[3], OMX_INVALID_BONE);

    // A completely different skeleton (other bone order, other proportions, extra bones)
    // can still use the same animation
    const char* other_text =
        "omx 1\n"
        "skel 11 dwarf\n"
        "  height 0.5\n"
        "  bone hips    -     0 0.5 0\n"
        "  bone thigh.r hips -0.15 0 0\n"
        "  bone thigh.l hips  0.15 0 0\n"
        "  bone beard   hips  0 0.6 0.1\n"
        "end\n";

    OmxFile other;
    OmxError error = {};
    TEST_TRUE(omx_test_parse_text(other_text, &other, &error));

    const OmxSkeleton* dwarf = &other.skeletons[0];
    uint16 dwarf_bones[4];
    TEST_EQUALS(omx_animation_bind(&_omx_locomotion, walk, &other, dwarf, dwarf_bones), 3);
    TEST_EQUALS(dwarf_bones[0], 0); // hips
    TEST_EQUALS(dwarf_bones[1], 2); // thigh.l
    TEST_EQUALS(dwarf_bones[2], 1); // thigh.r

    OmxTransform pose[4];
    omx_pose_from_bind(&other, dwarf, pose);

    // Root motion scaled by the height ratio of target / authoring skeleton
    const f32 scale = dwarf->height / rig->height;
    omx_animation_sample(&_omx_locomotion, walk, &other, dwarf, dwarf_bones, 0.5f, scale, pose);

    TEST_EQUALS_WITH_DELTA(pose[0].position.y, 0.475f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(pose[0].position.z, 0.35f, 0.0001f);

    // Non root translations keep the target proportions
    TEST_EQUALS_WITH_DELTA(pose[2].position.x, 0.15f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(pose[3].position.y, 0.6f, 0.0001f);

    // thigh.l at 0.5 is exactly on a key
    TEST_EQUALS_WITH_DELTA(pose[2].rotation.x, -0.2588f, 0.0001f);

    // in_out_sine at 50% between +x and -x -> identity
    omx_animation_sample(&_omx_locomotion, walk, &other, dwarf, dwarf_bones, 0.25f, scale, pose);
    TEST_EQUALS_WITH_DELTA(pose[2].rotation.x, 0.0f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(pose[2].rotation.w, 1.0f, 0.0001f);

    // looping: 1.75 == 0.75
    OmxTransform pose_a[4];
    OmxTransform pose_b[4];
    omx_pose_from_bind(&other, dwarf, pose_a);
    omx_pose_from_bind(&other, dwarf, pose_b);
    omx_animation_sample(&_omx_locomotion, walk, &other, dwarf, dwarf_bones, 1.75f, scale, pose_a);
    omx_animation_sample(&_omx_locomotion, walk, &other, dwarf, dwarf_bones, 0.75f, scale, pose_b);
    TEST_EQUALS_WITH_DELTA(pose_a[1].rotation.x, pose_b[1].rotation.x, 0.0001f);

    // Duration is derived from the last key if not defined
    TEST_EQUALS_WITH_DELTA(_omx_locomotion.animations[1].duration, 0.8f, 0.0001f);
    TEST_TRUE(_omx_locomotion.animations[1].flags & OMX_ANIMATION_ADDITIVE);
}

static void test_omx_animation_events()
{
    const OmxAnimation* walk = &_omx_locomotion.animations[0];
    const OmxEvent* events[8];

    TEST_EQUALS(omx_animation_collect_events(&_omx_locomotion, walk, 0.2f, 0.8f, events, 8), 4);

    // Wrap around: 0.9 -> 1.3 == (0.9, 1.0] + [0, 0.3]
    TEST_EQUALS(omx_animation_collect_events(&_omx_locomotion, walk, 0.9f, 1.3f, events, 8), 2);
    TEST_EQUALS(events[0]->type, OMX_EVENT_SOUND);
    TEST_EQUALS(events[0]->ref, 501);
    TEST_EQUALS(events[0]->bone, omx_hash("shin.l"));
    TEST_EQUALS(events[1]->type, OMX_EVENT_TAG);
    TEST_EQUALS(events[1]->ref, omx_hash("footstep_l"));

    TEST_EQUALS(omx_animation_collect_events(&_omx_locomotion, walk, 0.3f, 0.5f, events, 8), 0);

    // Particle events reference the emitter NAME of the model
    omx_animation_collect_events(&_omx_locomotion, walk, 0.7f, 0.8f, events, 8);
    TEST_EQUALS(events[1]->type, OMX_EVENT_PARTICLE);
    TEST_EQUALS(events[1]->ref, _omx_guard.emitters[0].name);
}

static void test_omx_hitbox_particles()
{
    const OmxHitbox* hit = &_omx_hit.hitboxes[0];
    TEST_EQUALS(hit->id, 300);
    TEST_EQUALS(hit->shape_count, 6);

    const OmxShape* shapes = omx_hitbox_shapes(&_omx_hit, hit);
    TEST_EQUALS(shapes[0].type, OMX_SHAPE_BOX);
    TEST_EQUALS(shapes[0].layer, OMX_HIT_LAYER_WORLD);
    TEST_EQUALS(shapes[1].type, OMX_SHAPE_SPHERE);
    TEST_EQUALS(shapes[1].bone, omx_hash("head"));
    TEST_EQUALS_WITH_DELTA(shapes[1].multiplier, 2.0f, 0.0001f);
    TEST_EQUALS(shapes[2].type, OMX_SHAPE_CAPSULE);
    TEST_EQUALS_WITH_DELTA(shapes[2].radius, 0.2f, 0.0001f);
    TEST_EQUALS(shapes[5].layer, OMX_HIT_LAYER_WEAPON);
    TEST_EQUALS_WITH_DELTA(shapes[5].multiplier, 1.5f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(shapes[5].rotation.w, 1.0f, 0.0001f);

    const OmxParticleSystem* torch = &_omx_fx.particle_systems[1];
    TEST_EQUALS(torch->max_particles, 128);
    TEST_TRUE(torch->flags & OMX_PARTICLE_LOOP);
    TEST_FALSE(torch->flags & OMX_PARTICLE_WORLD_SPACE);
    TEST_EQUALS(torch->frames_x, 4);
    TEST_EQUALS_WITH_DELTA(torch->fps, 24.0f, 0.0001f);
    TEST_EQUALS(torch->color_key_count, 3);
    TEST_EQUALS_WITH_DELTA(torch->color[1].color.g, 0.4f, 0.0001f);
    TEST_EQUALS(torch->shape, OMX_EMITTER_SPHERE);

    const OmxParticleSystem* dust = &_omx_fx.particle_systems[0];
    TEST_EQUALS(dust->burst, 12);
    TEST_EQUALS(dust->shape, OMX_EMITTER_CONE);
    TEST_EQUALS_WITH_DELTA(dust->shape_param.x, 70.0f, 0.0001f);
}

static void test_omx_instance()
{
    OmxFile* files[] = { &_omx_guard, &_omx_rig, &_omx_metals, &_omx_locomotion, &_omx_hit, &_omx_fx };
    const OmxLibrary lib = { ARRAY_COUNT(files), (const OmxFile **) files };

    OmxInstanceLayout layout;
    TEST_TRUE(omx_instance_layout(&_omx_guard, &lib, &layout));
    TEST_EQUALS(layout.bone_count, 14);
    TEST_EQUALS(layout.layer_count, 2);
    TEST_EQUALS(layout.particle_count, 32 + 128);
    TEST_EQUALS(layout.matrices_offset % 64, 0);
    TEST_EQUALS(layout.size % 64, 0);
    TEST_TRUE(layout.state_offset + _omx_guard.state_size <= layout.size);

    byte* instance = omx_test_alloc(layout.size);
    omx_instance_init(&_omx_guard, &lib, &layout, instance);

    const OmxEmitterState* emitters = (const OmxEmitterState *) (instance + layout.emitters_offset);
    TEST_EQUALS(emitters[0].active, 0);
    TEST_EQUALS(emitters[1].active, 1);
    TEST_EQUALS(emitters[1].particle_offset, 32);

    // Missing dependency -> layout can't be calculated
    OmxFile* partial[] = { &_omx_guard };
    const OmxLibrary partial_lib = { 1, (const OmxFile **) partial };
    TEST_FALSE(omx_instance_layout(&_omx_guard, &partial_lib, &layout));
}

static void test_omx_skinning()
{
    const OmxSkeleton* rig = &_omx_rig.skeletons[0];

    f32* inverse_bind = (f32 *) omx_test_alloc(sizeof(f32) * 16 * rig->bone_count);
    f32* temp = (f32 *) omx_test_alloc(sizeof(f32) * 16 * rig->bone_count);
    f32* matrices = (f32 *) omx_test_alloc(sizeof(f32) * 16 * rig->bone_count);
    OmxTransform* pose = (OmxTransform *) omx_test_alloc(sizeof(OmxTransform) * rig->bone_count);

    omx_skeleton_inverse_bind(&_omx_rig, rig, inverse_bind, temp);

    // forearm.l bind position in model space = spine(0, 1.25) + arm(0.2, 0.35) + forearm(0.3, 0)
    const uint16 forearm = omx_skeleton_find_bone(&_omx_rig, rig, omx_hash("forearm.l"));
    TEST_EQUALS_WITH_DELTA(temp[forearm * 16 + 12], 0.5f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(temp[forearm * 16 + 13], 1.6f, 0.0001f);

    // Bind pose -> skinning matrices must be identity
    omx_pose_from_bind(&_omx_rig, rig, pose);
    omx_pose_to_model_space(&_omx_rig, rig, pose, matrices);
    omx_skin_matrices(rig, inverse_bind, matrices);

    f32 max_error = 0.0f;
    for (uint32 i = 0; i < rig->bone_count; ++i) {
        for (int32 j = 0; j < 16; ++j) {
            const f32 identity = (j % 5 == 0) ? 1.0f : 0.0f;
            const f32 err = fabsf(matrices[i * 16 + j] - identity);
            max_error = err > max_error ? err : max_error;
        }
    }

    TEST_TRUE(max_error < 0.0001f);
}

static void test_omx_binary()
{
    const uint64 size = omx_binary_size(&_omx_guard);
    byte* binary = omx_test_alloc(size);
    TEST_EQUALS(omx_to_binary(&_omx_guard, binary), size);

    OmxFile loaded;
    TEST_TRUE(omx_from_binary(binary, size, &loaded));
    TEST_EQUALS(loaded.vertex_count, _omx_guard.vertex_count);
    TEST_MEMORY_EQUALS(loaded.vertices, _omx_guard.vertices, _omx_guard.vertex_count * _omx_guard.vertex_stride);
    TEST_MEMORY_EQUALS(loaded.indices, _omx_guard.indices, _omx_guard.index_count * sizeof(uint32));
    TEST_EQUALS(loaded.materials[0].id, 100);
    TEST_TRUE(strcmp(omx_asset_path(&loaded, &loaded.assets[0]), "humanoid_rig.omx") == 0);
    TEST_TRUE(loaded.skeletons == NULL);
    TEST_TRUE(loaded.data != _omx_guard.data);

    // Corrupted
    TEST_FALSE(omx_from_binary(binary, 8, &loaded));
    binary[0] = 0;
    TEST_FALSE(omx_from_binary(binary, size, &loaded));
}

static void test_omx_errors()
{
    OmxFile file;
    OmxError error;

    TEST_FALSE(omx_test_parse_text("omx 1\nv 0 0 0\n", &file, &error));
    TEST_EQUALS(error.line, 2);

    TEST_FALSE(omx_test_parse_text("v 0 0 0\n", &file, &error));
    TEST_EQUALS(error.line, 1);

    TEST_FALSE(omx_test_parse_text("omx 1\n\n# comment\nfoo 1\n", &file, &error));
    TEST_EQUALS(error.line, 4);

    TEST_FALSE(omx_test_parse_text("omx 1\nmat 1 a\nbase 1 1 1\n", &file, &error));
    TEST_TRUE(strcmp(error.message, "Missing end") == 0);

    TEST_FALSE(omx_test_parse_text("omx 1\nlayout pos\nv 0 0 0\nv 1 0 0\nf 0 1 2\n", &file, &error));
    TEST_EQUALS(error.line, 5);

    TEST_FALSE(omx_test_parse_text("omx 1\nlayout nrm pos\n", &file, &error));
    TEST_EQUALS(error.line, 2);

    TEST_FALSE(omx_test_parse_text("omx 1\nskel 1 s\nbone b a 0 0 0\nend\n", &file, &error));
    TEST_TRUE(strcmp(error.message, "Parent bone must be defined before the child") == 0);

    TEST_FALSE(omx_test_parse_text("omx 1\nanim 1 a\nch b rot\nk 0.5 0 0 0 1\nk 0.2 0 0 0 1\nend\n", &file, &error));
    TEST_EQUALS(error.line, 5);

    TEST_FALSE(omx_test_parse_text("omx 1\nlayout pos\nv 0 0 0 1\n", &file, &error));
    TEST_TRUE(strcmp(error.message, "Unexpected token") == 0);

    // Windows line endings + trailing comments
    TEST_TRUE(omx_test_parse_text("omx 1\r\nlayout pos # comment\r\nv 0 0 0\r\nv 1 0 0\r\nv 0 1 0\r\nf 0 1 2\r\n", &file, &error));
    TEST_EQUALS(file.index_count, 3);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main ObjectOmxTest
#endif

int main() {
    TEST_INIT(250);

    TEST_RUN(test_omx_load_examples);
    TEST_RUN(test_omx_number_parsing);
    TEST_RUN(test_omx_model);
    TEST_RUN(test_omx_material);
    TEST_RUN(test_omx_animation_retarget);
    TEST_RUN(test_omx_animation_events);
    TEST_RUN(test_omx_hitbox_particles);
    TEST_RUN(test_omx_instance);
    TEST_RUN(test_omx_skinning);
    TEST_RUN(test_omx_binary);
    TEST_RUN(test_omx_errors);

    TEST_FINALIZE();

    return 0;
}
