#ifndef COMS_UI_THEME_C
#define COMS_UI_THEME_C

#include "../stdlib/Stdlib.h"
#include "../memory/RingMemory.cpp"
#include "../utils/StringUtils.h"
#include "../stdlib/HashMap.cpp"
#include "../font/Font.cpp"
#include "../system/FileUtils.cpp"
#include "../sort/Sort.h"
#include "UITheme.h"

#include "attribute/UIAttribute.h"
#include "UIElementType.h"

FORCE_INLINE
UIAttributeGroup* theme_style_group(UIThemeStyle* theme, const char* group_name)
{
    HashEntryInt32* entry = (HashEntryInt32 *) hashmap_get_entry(&theme->hash_map, group_name);
    if (!entry) {
        ASSERT_TRUE(false);
        return NULL;
    }

    ASSERT_STRICT(((uintptr_t) (theme->data + entry->value)) % 4 == 0);

    return (UIAttributeGroup *) (theme->data + entry->value);
}

static FORCE_INLINE
int compare_by_attribute_id(const void* __restrict a, const void* __restrict b) NO_EXCEPT
{
    UIAttribute* attr_a = (UIAttribute *) a;
    UIAttribute* attr_b = (UIAttribute *) b;

    return attr_a->attribute_id - attr_b->attribute_id;
}

// File layout - text
// version
// #group_name
//      attributes ...
//      attributes ...
//      attributes ...
// #group_name
//      attributes ...
//      attributes ...
//      attributes ...

// WARNING: theme needs to have memory already reserved and assigned to data
void theme_from_file_txt(
    UIThemeStyle* theme,
    const char* path,
    RingMemory* const ring
) {
    FileBody file = {0};
    file_read(path, &file, ring);
    ASSERT_TRUE(file.size);

    const char* pos = (char *) file.content;

    // move past the "version" string
    pos += 8;

    // Use version for different handling
    /*MAYBE_UNUSED int32 version = (int32) */str_to_int(pos, &pos); ++pos;

    // We have to find how many groups are defined in the theme file.
    // Therefore we have to do an initial iteration
    int32 temp_group_count = 0;
    while (*pos != '\0') {
        // Skip all white spaces
        str_skip_empty(&pos);

        // Is group name
        if (*pos == '#' || *pos == '.') {
            ++temp_group_count;
        }

        // Go to the next line
        str_move_past(&pos, '\n');
    }

    // @performance This is probably horrible since we are not using a perfect hashing function (1 hash -> 1 index)
    //      I wouldn't be surprised if we have a 50% hash overlap (2 hashes -> 1 index)
    hashmap_create(
        &theme->hash_map,
        temp_group_count,
        sizeof(HashEntryInt32),
        theme->data,
        align_up((int32) sizeof(HashEntryInt32), 32)
    );
    int32 data_offset = (int32) hashmap_size(&theme->hash_map);

    UIAttributeGroup* temp_group = NULL;

    pos = (char *) file.content;

    // move past "version" string
    str_move_past(&pos, '\n');

    char attribute_name[32];
    char block_name[128]; // HASH_MAP_MAX_KEY_LENGTH would probably result in a buffer overflow
    bool last_token_newline = false;
    bool block_open = false;
    while (*pos != '\0') {
        str_skip_whitespace(&pos);

        if (is_eol(pos)) {
            pos += is_eol(pos);

            // 2 new lines => closing block
            if (last_token_newline) {
                block_open = false;
            }
            last_token_newline = !last_token_newline;

            continue;
        }

        last_token_newline = false;

        // Handle new group
        /////////////////////////////////////////////////////////
        if (!block_open) {
            str_copy_move_until(block_name, &pos, " \r\n");
            // @bug handle block_name length >= HASH_MAP_MAX_KEY_LENGTH
            // Why? well because we only store HASH_MAP_MAX_KEY_LENGTH - 1 in the hash map key
            // That means we could overwrite existing keys e.g. #btn_SOME_LONGER_NAME vs #ipt_SAME_LONG_NAME
            // In the above case the key uses the last HASH_MAP_MAX_KEY_LENGTH - 1 characters therefore ignoring the prefix

            // All blocks need to start with #. In the past this wasn't the case and may not be in the future. This is why we keep this if here.
            if (*block_name == '#' || *block_name == '.') {
                // Named style
                block_open = true;

                if (temp_group) {
                    // Before we insert a new group we have to sort the attributes of the PREVIOUS temp_group
                    // since this makes searching them later on more efficient.
                    sort_introsort(temp_group + 1, temp_group->attribute_count, sizeof(UIAttribute), compare_by_attribute_id);
                    // @todo This is where we create the Eytzinger order if we want to use it
                }

                // Insert new group
                hashmap_insert(&theme->hash_map, block_name, data_offset);

                temp_group = (UIAttributeGroup *) (theme->data + data_offset);
                temp_group->attribute_count = 0;
                data_offset += sizeof(UIAttributeGroup);
            }

            if (strlen(block_name) + 1 > HASH_MAP_MAX_KEY_LENGTH) {
                LOG_1("Identifier %s too long", {DATA_TYPE_CHAR_STR, block_name});
            }

            continue;
        }

        // Handle new attribute for previously found group
        /////////////////////////////////////////////////////////
        str_copy_move_until(attribute_name, &pos, " :\n");

        // Skip any white spaces or other delimeter
        str_skip_list(&pos, " \t:", sizeof(" \t:") - 1);

        // Parse attribute value
        UIAttribute attribute = {0};
        ui_attribute_parse_value(&attribute, attribute_name, pos);

        // @bug How to handle "anim"
        // We currently just end the block if we encounter it
        if (attribute.attribute_id == UI_ATTRIBUTE_TYPE_ANIMATION) {
            block_open = false;
            continue;
        }

        // Again, currently this if check is redundant but it wasn't in the past and we may need it again in the future.
        if (block_name[0] == '#' || block_name[0] == '.') {
            // Named block
            UIAttribute* attribute_reference = (UIAttribute *) (temp_group + 1);
            // @question Why are we even doing this? couldn't we just pass this offset to the ui_attribute_parse_value() function?
            memcpy(
                attribute_reference + temp_group->attribute_count,
                &attribute,
                sizeof(attribute)
            );

            data_offset += sizeof(attribute);
            ++temp_group->attribute_count;
        }

        str_move_to(&pos, '\n');
    }

    theme->used_data_size = data_offset;

    // We still need to sort the last group
    sort_introsort(temp_group + 1, temp_group->attribute_count, sizeof(UIAttribute), compare_by_attribute_id);
    // @todo This is where we create the Eytzinger order if we want to use it
}

static inline
void ui_theme_parse_group(HashEntryInt32* entry, byte* data, const byte** in)
{
    UIAttributeGroup* group = (UIAttributeGroup *) (data + entry->value);
    ASSERT_STRICT(((uintptr_t) group) % 4 == 0);

    group->attribute_count = SWAP_ENDIAN_LITTLE(*((int32 *) *in));
    *in += sizeof(group->attribute_count);

    UIAttribute* attribute_reference = (UIAttribute *) (group + 1);
    ASSERT_STRICT(((uintptr_t) attribute_reference) % 4 == 0);

    for (int32 j = 0; j < group->attribute_count; ++j) {
        attribute_reference[j].attribute_id = (UIAttributeType) SWAP_ENDIAN_LITTLE(*((uint16 *) *in));
        *in += sizeof(attribute_reference[j].attribute_id);

        attribute_reference[j].datatype = *((UIAttributeDataType *) *in);
        *in += sizeof(attribute_reference[j].datatype);

        if (attribute_reference[j].datatype == UI_ATTRIBUTE_DATA_TYPE_INT) {
            attribute_reference[j].value_int = SWAP_ENDIAN_LITTLE(*((int32 *) *in));
            *in += sizeof(attribute_reference[j].value_int);
        } else if (attribute_reference[j].datatype == UI_ATTRIBUTE_DATA_TYPE_F32) {
            attribute_reference[j].value_float = SWAP_ENDIAN_LITTLE(*((f32 *) *in));
            *in += sizeof(attribute_reference[j].value_float);
        } else if (attribute_reference[j].datatype == UI_ATTRIBUTE_DATA_TYPE_STR) {
            memcpy(attribute_reference[j].value_str, *in, sizeof(attribute_reference[j].value_str));
            *in += sizeof(attribute_reference[j].value_str);
        } else if (attribute_reference[j].datatype == UI_ATTRIBUTE_DATA_TYPE_V4_F32) {
            attribute_reference[j].value_v4_f32.x = SWAP_ENDIAN_LITTLE(*((f32 *) *in));
            *in += sizeof(attribute_reference[j].value_v4_f32.x);

            attribute_reference[j].value_v4_f32.y = SWAP_ENDIAN_LITTLE(*((f32 *) *in));
            *in += sizeof(attribute_reference[j].value_v4_f32.y);

            attribute_reference[j].value_v4_f32.z = SWAP_ENDIAN_LITTLE(*((f32 *) *in));
            *in += sizeof(attribute_reference[j].value_v4_f32.z);

            attribute_reference[j].value_v4_f32.w = SWAP_ENDIAN_LITTLE(*((f32 *) *in));
            *in += sizeof(attribute_reference[j].value_v4_f32.w);
        }
    }
}

// Memory layout (data) - This is not the layout of the file itself, just how we represent it in memory
// Hashmap
// UIAttributeGroup - This is where the pointers point to (or what the offset represents)
//      size
//      Attributes ...
//      Attributes ...
//      Attributes ...
// UIAttributeGroup
//      size
//      Attributes ...
//      Attributes ...
//      Attributes ...

// The size of theme->data should be the file size.
// Yes, this means we have a little too much data but not by a lot
int32 theme_from_data(
    const byte* __restrict data,
    UIThemeStyle* __restrict theme
) {
    PROFILE(PROFILE_THEME_FROM_THEME, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("Load theme");

    const byte* in = data;

    int32 version;
    in = read_le(in, &version);
    in = read_le(in, &theme->used_data_size);

    int32 count;
    read_le(in, &count);
    SWAP_ENDIAN_LITTLE_SELF(count);

    // Prepare hashmap (incl. reserve memory) by initializing it the same way we originally did
    // Of course we still need to populate the data using hashmap_load()
    // The value is a int64 (because this is the value of the chunk buffer size but the hashmap only allows int32)
    hashmap_create(
        &theme->hash_map,
        count,
        sizeof(HashEntryInt32),
        theme->data,
        align_up((int32) sizeof(HashEntryInt32), 4)
    );

    in += hashmap_load(&theme->hash_map, in, MEMBER_SIZEOF(HashEntryInt32, value));

    // theme data
    // Layout: first load the size of the group, then load the individual attributes
    // @performance We are iterating the hashmap twice (hashmap_load and here)
    uint32 chunk_id = 0;
    chunk_iterate_start(&theme->hash_map.buf, chunk_id) {
        HashEntryInt32* entry = (HashEntryInt32 *) chunk_get_element((ChunkMemory *) &theme->hash_map.buf, chunk_id);
        ui_theme_parse_group(entry, theme->data, &in);
    } chunk_iterate_end;

    LOG_1("[INFO] Loaded theme");

    return (int32) (in - data);
}

// Calculates the maximum theme size
// Not every group has all the attributes (most likely only a small subset)
// However, an accurate calculation is probably too slow and not needed most of the time
FORCE_INLINE
int64 theme_data_size_max(const UIThemeStyle* theme)
{
    return hashmap_size(&theme->hash_map)
        + theme->hash_map.buf.capacity * UI_ATTRIBUTE_TYPE_SIZE * sizeof(UIAttribute);
}

static inline
void ui_theme_serialize_group(const HashEntryInt32* entry, const byte* data, byte** out)
{
    // @performance Are we sure the data is nicely aligned?
    // Probably depends on the from_txt function and the start of theme->data
    UIAttributeGroup* group = (UIAttributeGroup *) (data + entry->value);
    ASSERT_STRICT(((uintptr_t) group) % 4 == 0);

    // Careful this serializes the group to a different offset than it is currently in memory
    // This means the entry->value (which represents the offset) is no longer correct when we load it
    // As a solution when we load the theme we move it back to the memory position where it was originally located
    // @question alternatively we could do out + entry->value as the storage location?
    //  If we would do that the loading might be faster since we don't have to jump in memory and can just load it in order?
    *((int32 *) *out) = SWAP_ENDIAN_LITTLE(group->attribute_count);
    *out += sizeof(group->attribute_count);

    UIAttribute* attribute_reference = (UIAttribute *) (group + 1);
    ASSERT_STRICT(((uintptr_t) attribute_reference) % 4 == 0);

    f32 tempf32;

    for (int32 j = 0; j < group->attribute_count; ++j) {
        *((uint16 *) *out) = SWAP_ENDIAN_LITTLE(attribute_reference[j].attribute_id);
        *out += sizeof(attribute_reference[j].attribute_id);

        *((byte *) *out) = attribute_reference[j].datatype;
        *out += sizeof(attribute_reference[j].datatype);

        // Careful the sizeof() below returns the actual value size but the union size is actually larger
        // This is fine as long as we remember when loading the data to zero out the other bytes
        if (attribute_reference[j].datatype == UI_ATTRIBUTE_DATA_TYPE_INT) {
            *((int32 *) *out) = SWAP_ENDIAN_LITTLE(attribute_reference[j].value_int);
            *out += sizeof(attribute_reference[j].value_int);
        } else if (attribute_reference[j].datatype == UI_ATTRIBUTE_DATA_TYPE_F32) {
            *((f32 *) *out) = SWAP_ENDIAN_LITTLE(attribute_reference[j].value_float);
            *out += sizeof(attribute_reference[j].value_float);
        } else if (attribute_reference[j].datatype == UI_ATTRIBUTE_DATA_TYPE_STR) {
            memcpy(*out, attribute_reference[j].value_str, sizeof(attribute_reference[j].value_str));
            *out += sizeof(attribute_reference[j].value_str);
        } else if (attribute_reference[j].datatype == UI_ATTRIBUTE_DATA_TYPE_V4_F32) {
            tempf32 = SWAP_ENDIAN_LITTLE(attribute_reference[j].value_v4_f32.x);
            memcpy(*out, &tempf32, sizeof(tempf32));
            *out += sizeof(attribute_reference[j].value_v4_f32.x);

            tempf32 = SWAP_ENDIAN_LITTLE(attribute_reference[j].value_v4_f32.y);
            memcpy(*out, &tempf32, sizeof(tempf32));
            *out += sizeof(attribute_reference[j].value_v4_f32.y);

            tempf32 = SWAP_ENDIAN_LITTLE(attribute_reference[j].value_v4_f32.z);
            memcpy(*out, &tempf32, sizeof(tempf32));
            *out += sizeof(attribute_reference[j].value_v4_f32.z);

            tempf32 = SWAP_ENDIAN_LITTLE(attribute_reference[j].value_v4_f32.w);
            memcpy(*out, &tempf32, sizeof(tempf32));
            *out += sizeof(attribute_reference[j].value_v4_f32.w);
        }
    }
}

// File layout - binary
// version
// hashmap size
// Hashmap (includes offsets to the individual groups)
// #group_name (this line doesn't really exist in file, it's more like the pointer offset in the hashmap)
//      size
//      attributes ...
//      attributes ...
//      attributes ...
// #group_name
//      size
//      attributes ...
//      attributes ...
//      attributes ...

int32 theme_to_data(
    const UIThemeStyle* __restrict theme,
    byte* __restrict data
) {
    byte* out = data;

    // version
    out = write_le(out, UI_THEME_VERSION);
    out = write_le(out, theme->used_data_size);

    // hashmap
    out += hashmap_dump(&theme->hash_map, out, MEMBER_SIZEOF(HashEntryInt32, value));

    // theme data
    // Layout: first save the size of the group, then save the individual attributes
    uint32 chunk_id = 0;
    chunk_iterate_start(&theme->hash_map.buf, chunk_id) {
        const HashEntryInt32* entry = (HashEntryInt32 *) chunk_get_element((ChunkMemory *) &theme->hash_map.buf, chunk_id);
        ui_theme_serialize_group(entry, theme->data, &out);
    } chunk_iterate_end;

    return (int32) (out - data);
}

#endif